/******************************************************************************
* File Name: config.h
*
* Description: This header file defines the application configuration for the PMG1B1
*              MCU USBPD DRP Example for ModusToolBox.
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2021-2025, Cypress Semiconductor Corporation (an Infineon company) or
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
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "cybsp.h"
#include "cy_pdutils_sw_timer.h"
#include "timer_id.h"
#include "stdio.h"
#include "solution.h"

#define PMG1B1_CHARGER_SINK_APP                     (0u)

/*
 * Enable/Disable firmware active LED operation.
 *
 * The blinking LED is enabled by default but it is recommended to disable it
 * for production designs to save power.
 */
#define APP_FW_LED_ENABLE                       (1u)

/* Enable board temperature monitoring */
#define ENABLE_NTCP0_TEMP_MONITORING (1u)
#define ENABLE_NTCP1_TEMP_MONITORING (1u)

#define DEBUG_UART_ENABLE (0u)

#define VBUS_SNK_UVP_ENABLE                         (0u)
#define BAT_HW_UVP_ENABLE                           (0u)
#define BAT_HW_OVP_ENABLE                           (0u)
#define BAT_HW_OCP_ENABLE                           (0u)
#define VBUS_SRC_OCP_PIN_ENABLE                     (1u)

/* SRC VBUS OCP pin method retry count. Set 255 to infinite retry */
#define VBUS_SRC_OCP_PIN_RETRY                      (2u)

/* BB VOUT voltage below which VBAT UVP is triggered */
#define PRIMARY_VBATT_UVP_THRESHOLD                 (3500u)

/* BB VOUT voltage above which VBAT OVP is triggered */
#define PRIMARY_VBATT_OVP_THRESHOLD                 (21500u)

#if DEBUG_UART_ENABLE

#define ENABLE_BATT_CURR_MONITORING (1u)

#define DEBUG_BATT_INFO_ENABLE (1u)
#define DEBUG_BATT_CELL_INFO_ENABLE (1u)
#define DEBUG_PWR_INFO_ENABLE (0u)
#define DEBUG_TEMP_INFO_ENABLE (1u)
#define SIMULATE_ERROR (0u)

#define DEBUG_PRINT(string)    \
    {                          \
        sprintf(temp, string); \
        debug_print(temp);     \
    }
#define DEBUG_PRINT_VAR(string, value) \
    {                                  \
        sprintf(temp, string, value);  \
        debug_print(temp);             \
    }

#else
#define DEBUG_BATT_INFO_ENABLE (0u)
#define DEBUG_BATT_CELL_INFO_ENABLE (0u)
#define DEBUG_PWR_INFO_ENABLE (0u)
#define DEBUG_TEMP_INFO_ENABLE (0u)
#define DEBUG_PRINT(string)
#define DEBUG_PRINT_VAR(string, value)
#endif

#define BATT_TIMER_ID                               (CY_USBPD_USER_TIMERS_START_ID + 2u)
#define BATT_TIMER_PERIOD                           (900)

#define SOLN_BATT_MONITOR_TASK_TIMER_ID             (CY_USBPD_USER_TIMERS_START_ID + 3u)
#define SOLN_BATT_MONITOR_TASK_TIMER_PERIOD         (200u)

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#ifndef NO_OF_TYPEC_PORTS
#define NO_OF_TYPEC_PORTS                         (CY_IP_MXUSBPD_INSTANCES)
#endif /* NO_OF_TYPEC_PORTS */

/** VBus current defined in 10mA units. */
#define CY_USBPD_I_1A                           (100u)
#define CY_USBPD_I_1_5A                         (150u)
#define CY_USBPD_I_2A                           (200u)
#define CY_USBPD_I_3A                           (300u)
#define CY_USBPD_I_4A                           (400u)
#define CY_USBPD_I_5A                           (500u)

/*******************************************************************************
 * Enable PD spec Rev 3 support
 ******************************************************************************/
#if CY_PD_REV3_ENABLE
    #define CY_PD_FRS_RX_ENABLE                   (0u)
    #define CY_PD_FRS_TX_ENABLE                   (0u)
    #define CY_PD_PPS_SRC_ENABLE                  (0u)
#endif /* CY_PD_REV3_ENABLE */

#define TOTAL_VBATT_MAX_ALLOWED_VOLT                (20000u)

/* Total minimum VBUS Power required for charging the battery in mWatt */
#define MIN_USBC_VBUS_TOTAL_POWER                   (7500u)

/* Total allowed min current to charge the battery in 10mA */
#define MIN_IBAT_CHARGING_CURR                      (30u)

/* Total allowed BB MAX output current in 10mA (Rsense = 5 mOhm)*/
#define VBAT_INPUT_CURR_MAX_SETTING                 (650u)

/* Efficiency settings of Buck Boost */
#define INPUT_OUTPUT_EFFICIENCY_REDUCE_PERCENTAGE   (93u)

#define PRIMARY_NTCP0_OTP_THRESHOLD                 (85)
#define PRIMARY_NTCP0_HOT_THRESHOLD                 (70)
#define PRIMARY_NTCP0_HYSTERESIS                    (10)

#define PRIMARY_NTCP1_OTP_THRESHOLD                 (100)
#define PRIMARY_NTCP1_HOT_THRESHOLD                 (85)
#define PRIMARY_NTCP1_HYSTERESIS                    (10)

/* Retry Timer to check detach condition */
#define APP_BROWNOUT_CHECK_DETACH_DELAY_ID          (CY_USBPD_USER_TIMERS_START_ID + 7u)
#define APP_BROWNOUT_CHECK_DETACH_DELAY_PERIOD      (12u)

/*
 * Macro defines additional delay in milliseconds before the PD stack starts sending
 * SRC_CAP message. This may be required to work with some non-compliant sink devices
 * which require more start up time for PD.
 */
#define DELAY_SRC_CAP_START_MS                  (100u)

/* Use the default Source PDO selection algorithm. */
#define PD_PDO_SEL_ALGO                         (0u)

/*******************************************************************************
 * USB-PD SAR ADC Configurations
 ******************************************************************************/
#define APP_VBUS_POLL_ADC_ID                    (CY_USBPD_ADC_ID_0)
#define APP_GPIO_POLL_ADC_ID                    (CY_USBPD_ADC_ID_1)
#define APP_GPIO_POLL_ADC_INPUT                 (CY_USBPD_ADC_INPUT_AMUX_A)
#if defined(CY_DEVICE_CCG3)
#define APP_VBUS_POLL_ADC_INPUT                 (CY_USBPD_ADC_INPUT_AMUX_A)
#else
#define APP_VBUS_POLL_ADC_INPUT                 (CY_USBPD_ADC_INPUT_AMUX_B)
#endif /* defined(CY_DEVICE_CCG3) */

/*******************************************************************************
 * Power Sink (PSINK) controls
 ******************************************************************************/
/* 
 * Allow VBUS_IN discharge below 5V.
 * When VBUS_IN_DISCHARGE_EN macro is enabled, VBUS_IN discharge is enabled for all
 * VBUS downward transitions above 5V, but is disabled for transitions below 5V.
 * Because, for VBUS_IN powered solutions, VBUS_IN should not be accidently
 * brought to the low voltage where system behavior is undefined. 
 * VBUS_IN discharge below 5V may be required for solutions where regulator
 * requires higher discharge strength to set voltage below 5V.
 * It is recommended to enable this feature only for solution which are not
 * VBUS_IN powered.
 */
#define VBUS_IN_DISCH_BELOW_5V_EN               (0u)

/*******************************************************************************
 * Power Source (PSOURCE) Configuration.
 ******************************************************************************/
#define APP_VBUS_SET_VOLT_P1(context, volt_mV)      sol_batt_src_set_volt(context, volt_mV)

/* Enable PWM to control vPosSlewRate of SRC-mode Buck controller (for VBUS 5V -> 9V transfer) */
#define VBUS_SRC_HV_PWM_SLEW_RATE_ENABLE            (1u)

#if VBUS_SRC_HV_PWM_SLEW_RATE_ENABLE
/* Set increment value for PWM duty cycle in PWM clocks */
#define PWM_DUTY_CYCLE_INCREMENT                    (10u)
/* Set the PWM period in count value in the range: 0 - 65535. Maximum value 65535 corresponds to 1 full round of count. */
#define PWM_PERIOD                                  (uint16_t)(480)
#endif


/* Time (in ms) allowed for source voltage to become valid. */
#define APP_PSOURCE_EN_TIMER_PERIOD             (250u)

/* Period (in ms) of VBus validity checks after enabling the power source. */
#define APP_PSOURCE_EN_MONITOR_TIMER_PERIOD     (1u)

/* Time (in ms) between VBus valid and triggering of PS_RDY. */
#define APP_PSOURCE_EN_HYS_TIMER_PERIOD         (5u)

/* Time (in ms) for which the VBus_Discharge path will be enabled when turning power source OFF. */
#define APP_PSOURCE_DIS_TIMER_PERIOD            (600u)

/* Period (in ms) of VBus drop to VSAFE0 checks after power source is turned OFF. */
#define APP_PSOURCE_DIS_MONITOR_TIMER_PERIOD    (1u)

/* VBus Monitoring is done using internal resistor divider. */
#define VBUS_MON_INTERNAL                       (1u)

/* Period in ms for turning on VBus FET. */
#define APP_VBUS_FET_ON_TIMER_PERIOD           (5u)

/* Period in ms for turning off VBus FET. */
#define APP_VBUS_FET_OFF_TIMER_PERIOD           (1u)

/*******************************************************************************
 * VBus monitor configuration.
 ******************************************************************************/

/* Allowed VBus valid margin as percentage of expected voltage. */
#define VBUS_TURN_ON_MARGIN                     (-20)

/* Allowed VBus valid margin (as percentage of expected voltage) before detach detection is triggered. */
#define VBUS_TURN_OFF_MARGIN                    (-20)

/* Allowed margin over expected voltage (as percentage) for negative VBus voltage transitions. */
#define VBUS_DISCHARGE_MARGIN                   (20)

/* Allowed margin over 5V before the provider FET is turned OFF when discharging to VSAFE0. */
#define VBUS_DISCHARGE_TO_5V_MARGIN             (10)

/*******************************************************************************
 * System fault configuration features.
 ******************************************************************************/

/* 
 * Enable/Disable delay between fault retries for Type-C/PD faults.
 */
#define FAULT_RETRY_DELAY_EN                        (0u)

#if FAULT_RETRY_DELAY_EN
/* 
 * Delay between fault retries in mS.
 */
#define FAULT_RETRY_DELAY_MS                        (500u)

#endif /* FAULT_RETRY_DELAY_EN */

/* 
 * Enable/Disable delayed infinite fault recovery for Type-C/PD faults.
 * Fault recovery shall be tried with a fixed delay after configured 
 * fault retry count is elapsed. 
 */
#define FAULT_INFINITE_RECOVERY_EN                  (0u)

#if FAULT_INFINITE_RECOVERY_EN

/* 
 * Delayed fault recovery period in mS.
 */
#define FAULT_INFINITE_RECOVERY_DELAY_MS            (5000u)

#endif /* FAULT_INFINITE_RECOVERY_EN */


/*
 * Disable PMG1 device reset on error (watchdog expiry or hard fault).
 * NOTE: Enabling this feature can cause unexpected device reset during SWD debug sessions.
 */
#define RESET_ON_ERROR_ENABLE (0u)

/*
 * Enable watchdog hardware reset for CPU lock-up recovery. Note that watchdog reset can only be enabled if we have
 * any periodic timers running in the application.
 */
#if (RESET_ON_ERROR_ENABLE)
#define WATCHDOG_HARDWARE_RESET_ENABLE (1u)
#else
#define WATCHDOG_HARDWARE_RESET_ENABLE (0u)
#endif /* (RESET_ON_ERROR_ENABLE) */

/*
 * Watchdog reset period in ms. This should be set to a value greater than
 * 500 ms to avoid significant increase in power consumption.
 */
#define WATCHDOG_RESET_PERIOD_MS                    (750u)

/* Enable tracking of maximum stack usage. */
#define STACK_USAGE_CHECK_ENABLE                    (0u)

/*
 * Set this to 1 to Shutdown the SNK FET in the application layer in states where power consumption needs to be
 * reduced to standby level.
 */
#define SNK_STANDBY_FET_SHUTDOWN_ENABLE (0u)

/*
 * Activity indicator LED timer. The timer is used to indicate that the firmware
 * is functional. The feature is controlled by APP_FW_LED_ENABLE.
 */
#define LED_TIMER_ID                            (CY_PDUTILS_TIMER_USER_START_ID + 4u)

/*
 * The LED toggle period (ms) to be used when Type-C connection hasn't been detected.
 */
#define LED_TIMER_PERIOD_DETACHED               (1000u)

/*
 * The LED toggle period (ms) to be used when a Type-C power source is connected.
 */
#define LED_TIMER_PERIOD_TYPEC_SRC                  (1000u)

/*
 * The LED toggle period (ms) to be used when a USB-PD power source is connected.
 */
#define LED_TIMER_PERIOD_PD_SRC                 (100u)

/*
 * The LED toggle period (ms) to be used when a BC 1.2 DCP (Downstream Charging Port) source without PD support is connected.
 */
#define LED_TIMER_PERIOD_DCP_SRC                (3000u)

/*
 * The LED toggle period (ms) to be used when a BC 1.2 CDP (Charging Downstream Port) source without PD support is connected.
 */
#define LED_TIMER_PERIOD_CDP_SRC                (10000u)

/*******************************************************************************
 * Firmware feature configuration.
 ******************************************************************************/

/** Timer period in ms for providing delay for VConn Gate Pull Up enable. */
#define APP_VCONN_TURN_ON_DELAY_PERIOD              (1u)

/***********************************************************************************/
/* Enable selection of data/power role preference. */
#define ROLE_PREFERENCE_ENABLE                      (0u)
#define POWER_ROLE_PREFERENCE_ENABLE                (0u)

/*******************************************************************************
 * Power throttling specific Configuration.
 ******************************************************************************/
/*
 * Set this macro to 1 if the temperature based power throttling is
 * done with thermistor.
 */

#define TEMPERATURE_SENSOR_IS_THERMISTOR            (1u)

#if TEMPERATURE_SENSOR_IS_THERMISTOR

/**
@brief Temperature sensor count
*/
#define TEMPERATURE_SENSOR_COUNT                    (2u)

/*
 * Defines the temperature starting from which mapping is done
 */
#define BASE_MAP_TEMP                               (20u)

/*
 * Defines the resolution of the map function in deg(C)
 */
#define TEMP_MAP_RESOLUTION                         (5u)

/*
 * Defines the number of map data available
 */
#define VOLT_TO_TEMP_MAP_TABLE_COUNT                (20u)

/*
 * Defines the safe temperature value when the thermistor voltage is below
 * the first entry of the table
 */
#define SAFE_DEFAULT_TEMPERATURE                    (25u)

/*
 * Macro when set defines the Thermistor type as NTC.
 */
#define THERMISTOR_IS_NTC                           (1u)

/*
 * Defines the fault value returned by I2C/Thermistor.
 */
#define THROTTLE_SENSOR_FAULT                       (0xFF)

#endif /* TEMPERATURE_SENSOR_IS_THERMISTOR */



#endif /* _CONFIG_H_ */

/* End of file [] */
