/**
 * @file solution_tasks.c
 *
 * @brief @{Solution source file solution layer related tasks.@}
 *
 *******************************************************************************
 * Copyright 2021-2023, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 *******************************************************************************/

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"
#include "stdio.h"
#include "cy_gpio.h"

#include "config.h"
#include "thermistor.h"

#if DEBUG_UART_ENABLE
#include "debug.h"
char temp[80];
#endif

cy_stc_battery_charging_context_t battery_charging_p0_ctx;

cy_stc_battery_charging_context_t * battery_charging_contexts[NO_OF_TYPEC_PORTS] =
{
    &battery_charging_p0_ctx
};

cy_stc_battery_charging_context_t * get_battery_charging_context(uint8_t portIdx)
{
    return (battery_charging_contexts[portIdx]);
}

void reset_battery_status(cy_stc_battery_charging_context_t *ptrBatteryChargingContext)
{
    cy_stc_battery_status_t* batt_stat = &(ptrBatteryChargingContext->batteryStatus);

    batt_stat->curr_chrg_cycle_num = 1;
    batt_stat->cur_batt_charging_status = false;
    batt_stat->adc_pending = false;
}

void clear_flags_on_usbc_disconnect (cy_stc_battery_charging_context_t *ptrBatteryChargingContext)
{
    cy_stc_battery_status_t* batt_stat = &(ptrBatteryChargingContext->batteryStatus);
    batt_stat->curr_chrg_cycle_num = 1;
    batt_stat->cur_bb_pwr = 0;
    batt_stat->cur_bb_iout = MIN_IBAT_CHARGING_CURR;
    batt_stat->cur_bb_vout = TOTAL_VBATT_MAX_ALLOWED_VOLT;
    batt_stat->is_vbus_pwr_sufficient = false;
    batt_stat->batt_ocp_fault_active = false;
    batt_stat->batt_rcp_fault_active = false;
    batt_stat->batt_ovp_fault_active = false;
    batt_stat->batt_uvp_fault_active = false;
}

void reset_battery_faults(cy_stc_battery_charging_context_t *ptrBatteryChargingContext)
{
    cy_stc_battery_status_t* batt_stat = &(ptrBatteryChargingContext->batteryStatus);

    batt_stat->batt_uvp_fault_active = false;
    batt_stat->batt_ovp_fault_active = false;
    batt_stat->batt_ocp_fault_active = false;
    batt_stat->batt_rcp_fault_active = false;
    batt_stat->batt_otp_fault_active = false;
}

#if PRINT_CV
bool print_flag = false;
void print_cb (
        cy_timer_id_t id,           /**< Timer ID for which callback is being generated. */
        void *callbackContext)       /**< Timer module Context. */
{
    cy_stc_pdstack_context_t* ptrPdStackContext = (cy_stc_pdstack_context_t*) callbackContext;
    print_flag = true;
    Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext, MY_ID, MY_PERIOD, &print_cb);
}
#endif /* PRINT_CV */

/** VBus current usage = 1.0 A. */
#define CY_USBPD_I_1A                           (100u)
#define CY_USBPD_I_2A                           (200u)
#define CY_USBPD_I_4A                           (400u)

/* ADC reference voltage for ID Pin and Thermistor measurements */
#define INPUT_ADC_VDDD_REF_VOLT                     (5000u)

uint16_t get_adc_8bit(cy_stc_pdstack_context_t* ptrPdStackContext, uint32_t port, uint32_t pin)
{
    uint8_t level;
    uint16_t volt;
    uint16_t state = Cy_SysLib_EnterCriticalSection();

    Cy_GPIO_SetHSIOM(Cy_GPIO_PortToAddr(port), pin, HSIOM_SEL_AMUXA);
    Cy_SysLib_DelayUs(20);

    /* Take ADC sample. */
    level = Cy_USBPD_Adc_Sample(ptrPdStackContext->ptrUsbPdContext, APP_GPIO_POLL_ADC_ID, APP_GPIO_POLL_ADC_INPUT);
    volt = ((uint32_t)(level * INPUT_ADC_VDDD_REF_VOLT) / PD_ADC_NUM_LEVELS);
    Cy_GPIO_SetHSIOM(Cy_GPIO_PortToAddr(port), pin, HSIOM_SEL_GPIO);
    Cy_SysLib_DelayUs(10);

    Cy_SysLib_ExitCriticalSection(state);
    return volt;
}

void measure_onboard_ntcp0_sensor_data(cy_stc_battery_charging_context_t *ptrBatteryChargingContext)
{
    cy_stc_battery_status_t* batt_stat = &(ptrBatteryChargingContext->batteryStatus);
    static temp_state_t ntcp0_state = STATE_TEMP_NORMAL_CHARGE;

    int8_t ntcp0_temp = ccg_volt_temp_map(ptrBatteryChargingContext->ptrPdStack,
            get_adc_8bit(ptrBatteryChargingContext->ptrPdStack, NTCP0_PORT_NUM, NTCP0_PIN));

#if DEBUG_TEMP_INFO_ENABLE
    sprintf(temp, "\n NTCP0 Temp: %i C", ntcp0_temp);
    debug_print(temp);
#endif

    switch(ntcp0_state)
    {
        case STATE_TEMP_NORMAL_CHARGE:
            if(ntcp0_temp >= PRIMARY_NTCP0_HOT_THRESHOLD)
            {
                ntcp0_state = STATE_TEMP_LIMITED_CHARGE;
            }
            break;
        case STATE_TEMP_LIMITED_CHARGE:
            if(ntcp0_temp >= PRIMARY_NTCP0_OTP_THRESHOLD)
            {
                ntcp0_state = STATE_TEMP_OTP;
            }
            if(ntcp0_temp < (PRIMARY_NTCP0_HOT_THRESHOLD - PRIMARY_NTCP0_HYSTERESIS))
            {
                ntcp0_state = STATE_TEMP_NORMAL_CHARGE;
            }
            break;
        case STATE_TEMP_OTP:
            if(ntcp0_temp < (PRIMARY_NTCP0_OTP_THRESHOLD - PRIMARY_NTCP0_HYSTERESIS))
            {
                ntcp0_state = STATE_TEMP_LIMITED_CHARGE;
            }
            break;
        default:
            break;
    }
    /* set OTP and current limit based on the new state */
    switch(ntcp0_state)
    {
        case STATE_TEMP_LIMITED_CHARGE:
            batt_stat->ntcp0_otp_fault_active = false;
            batt_stat->batt_max_curr_rating = CY_USBPD_GET_MIN(batt_stat->batt_max_curr_rating / 2, CY_USBPD_I_2A);
#if DEBUG_UART_ENABLE
            sprintf(temp, "NTCP0 limits current to %d\n", batt_stat->batt_max_curr_rating * 10);
            debug_print(temp);
#endif
            break;
        case STATE_TEMP_NORMAL_CHARGE:
            batt_stat->ntcp0_otp_fault_active = false;
            break;
        case STATE_TEMP_OTP:
            batt_stat->ntcp0_otp_fault_active = true;
#if DEBUG_UART_ENABLE
            debug_print("NTCP0 OTP\r\n");
#endif
            break;
        default:
            break;
    }
}

void measure_onboard_ntcp1_sensor_data(cy_stc_battery_charging_context_t *ptrBatteryChargingContext)
{
    cy_stc_battery_status_t* batt_stat = &(ptrBatteryChargingContext->batteryStatus);
    static temp_state_t ntcp1_state = STATE_TEMP_NORMAL_CHARGE;

    int8_t ntcp1_temp = ccg_volt_temp_map(ptrBatteryChargingContext->ptrPdStack,
        get_adc_8bit(ptrBatteryChargingContext->ptrPdStack, NTCP1_PORT_NUM, NTCP1_PIN));
#if DEBUG_TEMP_INFO_ENABLE
    sprintf(temp, "\n NTCP1 Temp: %i C", ntcp1_temp);
    debug_print(temp);
#endif

    switch(ntcp1_state)
    {
        case STATE_TEMP_NORMAL_CHARGE:
            if(ntcp1_temp >= PRIMARY_NTCP1_HOT_THRESHOLD)
            {
                ntcp1_state = STATE_TEMP_LIMITED_CHARGE;
            }
            break;
        case STATE_TEMP_LIMITED_CHARGE:
            if(ntcp1_temp >= PRIMARY_NTCP1_OTP_THRESHOLD)
            {
                ntcp1_state = STATE_TEMP_OTP;
            }
            if(ntcp1_temp < (PRIMARY_NTCP1_HOT_THRESHOLD - PRIMARY_NTCP1_HYSTERESIS))
            {
                ntcp1_state = STATE_TEMP_NORMAL_CHARGE;
            }
            break;
        case STATE_TEMP_OTP:
            if(ntcp1_temp < (PRIMARY_NTCP1_OTP_THRESHOLD - PRIMARY_NTCP1_HYSTERESIS))
            {
                ntcp1_state = STATE_TEMP_LIMITED_CHARGE;
            }
            break;
        default:
            break;
    }
    /* set OTP and current limit based on the new state */
    switch(ntcp1_state)
    {
        case STATE_TEMP_LIMITED_CHARGE:
            batt_stat->ntcp1_otp_fault_active = false;
            batt_stat->batt_max_curr_rating = CY_USBPD_GET_MIN(batt_stat->batt_max_curr_rating / 2, CY_USBPD_I_2A);
#if DEBUG_UART_ENABLE
            sprintf(temp, "NTCP1 limits current to %d\n", batt_stat->batt_max_curr_rating * 10);
            debug_print(temp);
#endif
            break;
        case STATE_TEMP_NORMAL_CHARGE:
            batt_stat->ntcp1_otp_fault_active = false;
            break;
        case STATE_TEMP_OTP:
            batt_stat->ntcp1_otp_fault_active = true;
#if DEBUG_UART_ENABLE
            debug_print("NTCP1 OTP\r\n");
#endif
            break;
        default:
            break;
    }
}

/* [] END OF FILE */
