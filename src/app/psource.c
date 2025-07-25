/******************************************************************************
* File Name: psource.c
*
* Description: This source file implements functions associated with the power
*              provider path control and fault detection which are part of the
*              PMG1 MCU USB-PD DRP Code Example for ModusToolBox.
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

#include "cybsp.h"
#include "config.h"
#include "psink.h"
#include "psource.h"
#include "app.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_usbpd_vbus_ctrl.h"

#if (!CY_PD_SINK_ONLY)
/* Type-C current levels in 10mA units. */
#define CUR_LEVEL_3A                        (300)
#define CUR_LEVEL_1_5A                      (150)
#define CUR_LEVEL_DEF                       (90)

/* VBUS absolute maximum voltage in mV units */
#define VBUS_MAX_VOLTAGE  (21500u)

/* Regulator turn on delay in ms */
#define REGULATOR_TURN_ON_DELAY                     (50)

extern cy_stc_pdstack_context_t *get_pdstack_context(uint8_t portIdx);

static void psrc_en_ovp(cy_stc_pdstack_context_t * context);
static void psrc_en_uvp(cy_stc_pdstack_context_t * context);
static void psrc_en_rcp(cy_stc_pdstack_context_t * context);

static void psrc_dis_ovp(cy_stc_pdstack_context_t * context);
static void psrc_dis_uvp(cy_stc_pdstack_context_t * context);
static void psrc_dis_ocp(cy_stc_pdstack_context_t * context);
static void psrc_dis_scp(cy_stc_pdstack_context_t * context);
static void psrc_dis_rcp(cy_stc_pdstack_context_t * context);

static void psrc_shutdown(cy_stc_pdstack_context_t * context, bool discharge_dis);

#if VBUS_SOFT_START_ENABLE

bool gl_fet_soft_start_en[NO_OF_TYPEC_PORTS] = {false};
void ocp_handler_wrapper(cy_timer_id_t id,  void *cbkContext);

#endif /* VBUS_SOFT_START_ENABLE */

void app_psrc_tmr_cbk(cy_timer_id_t id,  void * callbackCtx);
bool app_psrc_vbus_ovp_cbk(void *context, bool compOut);
bool app_psrc_vbus_ocp_cbk(void * cbkContext, bool comp_out);
void psrc_select_voltage(cy_stc_pdstack_context_t *context);

#if defined(CY_DEVICE_PMG1S3)
void vbus_fet_on_cbk (cy_timer_id_t id,  void * context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    /* Turn On the FET. */
    Cy_USBPD_Vbus_NgdoG1Ctrl (ptrPdStackContext->ptrUsbPdContext, true);
}
#endif /* defined(CY_DEVICE_PMG1S3) */

#if VBUS_SOFT_START_ENABLE
static void Vbus_NgdoSoftStartOn(cy_stc_pdstack_context_t *context)
{
    /* Disable the OCP fault detection module */
    Cy_USBPD_Fault_Vbus_OcpDisable(context->ptrUsbPdContext, true);

    /* Set the OCP threshold to 200 mA for soft start */
    Cy_USBPD_Fault_Vbus_OcpEnable(context->ptrUsbPdContext, 20, app_psrc_vbus_ocp_cbk);

    gl_fet_soft_start_en[context->port] = true;

    /* Set the drive strength of the NGDO to high (1.05uA). */
    Cy_USBPD_Vbus_NgdoSetDriveStrength(context->ptrUsbPdContext, (uint8_t)0x07);

    /* Start the timer which will change drive strength and OCP settings to default after a timeout. */
    Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_FET_SOFT_START_TIMER_ID), 50, ocp_handler_wrapper);
}
#endif /* VBUS_SOFT_START_ENABLE */

static void vbus_fet_on(cy_stc_pdstack_context_t *context)
{
    /* If fet is already On then no need to enable it again */    
    if(app_get_status(context->port)->is_vbus_on == false)
    {
        app_get_status(context->port)->is_vbus_on = true;

        /*
         * In case of REGULATOR_REQUIRE_STABLE_ON_TIME, the regulator is
         * already turned on. POWER_BANK implementation, uses a single
         * regulator and FET control for both source and sink operation.
         * Turning OFF sink FET here will cause the regulator to get wrongly
         * shut down. We should not disable sink here in this case.
         */
        Cy_USBPD_Vbus_GdrvPfetOff(context->ptrUsbPdContext, false);
        Cy_SysLib_DelayUs(10);
#if (PMG1B1_CHARGER_SINK_APP == 0)
        sol_batt_src_en(context);
#endif /* (PMG1B1_CHARGER_SINK_APP == 0) */

#if defined(CY_DEVICE_PMG1S3)
        Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, GET_APP_TIMER_ID(context, APP_VBUS_FET_OFF_TIMER));
#if VBUS_SOFT_START_ENABLE
        Vbus_NgdoSoftStartOn(context);
#endif /* VBUS_SOFT_START_ENABLE */
#endif /* defined(CY_DEVICE_PMG1S3) */

#if defined(CY_DEVICE_PMG1S3)
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, 
                GET_APP_TIMER_ID(context, APP_VBUS_FET_ON_TIMER),
                APP_VBUS_FET_ON_TIMER_PERIOD, vbus_fet_on_cbk);
#endif /* defined(CY_DEVICE_PMG1S3) */
    }
}

#if defined(CY_DEVICE_PMG1S3)
void vbus_fet_off_cbk (cy_timer_id_t id,  void * context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;

    Cy_USBPD_Vbus_NgdoEqCtrl (ptrPdStackContext->ptrUsbPdContext, false);

    Cy_USBPD_Vbus_GdrvPfetOff(ptrPdStackContext->ptrUsbPdContext, true);
}
#endif /* defined(CY_DEVICE_PMG1S3) */

static void vbus_fet_off(cy_stc_pdstack_context_t *context)
{
    app_get_status(context->port)->is_vbus_on = false;
#if defined(CY_DEVICE_PMG1S3)
    /* Stop the VBUS_FET_ON_TIMER. */
    Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, GET_APP_TIMER_ID(context, APP_VBUS_FET_ON_TIMER));

    Cy_USBPD_Vbus_NgdoG1Ctrl (context->ptrUsbPdContext, false);
    Cy_USBPD_Vbus_NgdoEqCtrl (context->ptrUsbPdContext, true);

    Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_VBUS_FET_OFF_TIMER),
            APP_VBUS_FET_OFF_TIMER_PERIOD, vbus_fet_off_cbk);
#else
        sol_batt_src_dis(context);
#endif /* defined(CY_DEVICE_PMG1S3) */
}

static void call_psrc_ready_cbk(cy_stc_pdstack_context_t * context)
{
    app_status_t* app_stat = app_get_status(context->port);

    if (app_stat->pwr_ready_cbk != NULL)
    {
        app_stat->pwr_ready_cbk (context);
        app_stat->pwr_ready_cbk = NULL;
    }
}

/*Timer Callback*/
void app_psrc_tmr_cbk(cy_timer_id_t id,  void * callbackCtx)
{
    cy_stc_pdstack_context_t* context = callbackCtx;
    app_status_t* app_stat = app_get_status(context->port);

    switch(id)
    {
        case APP_PSOURCE_EN_TIMER:
            /* Supply did not reach expected level. Turn off power and do error recovery. */
            Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_MONITOR_TIMER), GET_APP_TIMER_ID(context, APP_PSOURCE_EN_HYS_TIMER));
            app_stat->psrc_volt_old = CY_PD_VSAFE_0V;
            psrc_shutdown(context, true);

#if (VBUS_UVP_ENABLE)
            /*
             *If the VBUS does not reach VSAFE5V, then we need to treat this as an
             * under voltage condition. Since the UVP hardware detection cannot be
             * enabled when turning on the VBUS, this has to be manually triggered
             * from here by invoking the callback directly. Do this only if UVP is
             * enabled from the configuration table.
             */
            if (context->ptrUsbPdContext->usbpdConfig->vbusOvpConfig->enable)
            {
                app_psrc_vbus_ovp_cbk(context, false);
            }
#endif /* (VBUS_UVP_ENABLE) */
            break;

        case APP_PSOURCE_EN_MONITOR_TIMER:
            if (((app_stat->psrc_rising == true) &&
                        (vbus_is_present(context, app_stat->psrc_volt, VBUS_TURN_ON_MARGIN) == true)) ||
                    ((app_stat->psrc_rising == false) &&
                     (vbus_is_present(context, app_stat->psrc_volt, VBUS_DISCHARGE_MARGIN) == false))
                    )
            {
                /* Start Source enable hysteresis Timer */
                Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_HYS_TIMER),
                        APP_PSOURCE_EN_HYS_TIMER_PERIOD, app_psrc_tmr_cbk);
                break;
            }

            /* Start Monitor Timer again */
            Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_MONITOR_TIMER),
                    APP_PSOURCE_EN_MONITOR_TIMER_PERIOD, app_psrc_tmr_cbk);
            break;

        case APP_PSOURCE_EN_HYS_TIMER:

            Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_TIMER));
            app_stat->psrc_volt_old = app_stat->psrc_volt;
            vbus_discharge_off(context);

            if(app_stat->psrc_rising == false)
            {
                /* VBus voltage has stabilized at the new lower level. Update the OVP and RCP limits. */
                psrc_en_ovp (context);
                psrc_en_rcp (context);
            }
            else
            {
                psrc_en_uvp (context);
            }

#if VREG_BROWN_OUT_DET_ENABLE
            sol_brown_out_control(context, true);
#endif /* VREG_BROWN_OUT_DET_ENABLE*/

            call_psrc_ready_cbk(context);
            break;

        case APP_PSOURCE_DIS_TIMER:
            /* Discharge operation timed out. */
            Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, GET_APP_TIMER_ID(context, APP_PSOURCE_DIS_MONITOR_TIMER));
            psrc_shutdown(context, true);
            break;

        case APP_PSOURCE_DIS_MONITOR_TIMER:
            if (vbus_is_present(context, CY_PD_VSAFE_5V, VBUS_DISCHARGE_TO_5V_MARGIN) == false)
            {
                /* Voltage has dropped below 5 V. We can now turn off the FET and continue discharge. */
                psrc_shutdown(context, false);
            }

            if (vbus_is_present(context, CY_PD_VSAFE_0V, VBUS_TURN_ON_MARGIN) == false)
            {
                /* Start Extra discharge to allow proper discharge below Vsafe0V */
                Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_DIS_EXT_DIS_TIMER),
                        APP_PSOURCE_DIS_EXT_DIS_TIMER_PERIOD,
                        app_psrc_tmr_cbk);
            }
            else
            {
                /* Start Monitor Timer again */
                 Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_DIS_MONITOR_TIMER),
                        APP_PSOURCE_DIS_MONITOR_TIMER_PERIOD,
                        app_psrc_tmr_cbk);
            }
            break;

        case APP_PSOURCE_DIS_EXT_DIS_TIMER:
            Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, GET_APP_TIMER_ID(context, APP_PSOURCE_DIS_TIMER));
            vbus_discharge_off(context);

            /* Notify the caller that psrc_disable is complete. */
            call_psrc_ready_cbk(context);
            break;

        default:
            break;
    }
}

bool app_psrc_vbus_ocp_cbk(void * callbackCtx, bool compOut)
{
    (void)compOut;
    cy_stc_usbpd_context_t *usbpdcontext=callbackCtx;
    cy_stc_pdstack_context_t *context = (cy_stc_pdstack_context_t *)usbpdcontext->pdStackContext;

    /*
     * Stop all psource transition timers and notify stack about voltage
     * transition complete to process the SCP hard reset sequence.
     * Also disable all other fault detection by calling psource shutdown.
     */
    Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext, CY_USBPD_APP_PSOURCE_EN_TIMER, CY_USBPD_APP_PSOURCE_EN_HYS_TIMER);

    call_psrc_ready_cbk(context);

    /* OCP fault. */
    psrc_shutdown(context, true);

    /* Set alert message */
    cy_pd_pd_do_t alert;
    alert.val = 0;
    alert.ado_alert.ocp = 1u;
    context->dpmStat.alert = alert;

    /* Enqueue HPI OVP fault event. */
    app_event_handler(context, APP_EVT_VBUS_OCP_FAULT, NULL);
    return true;
}

#if VBUS_SCP_ENABLE
bool app_psrc_vbus_scp_cbk(void * cbkContext, bool comp_out)
{
    /* Get the PD-Stack context from the USBPD context */
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *) cbkContext;
    cy_stc_pdstack_context_t * pdstack_ctx = get_pdstack_context(context->port);
    /* 
     * Stop all psource transition timers and notify stack about voltage
     * transition complete to process the SCP hard reset sequence. 
     * Also disable all other fault detection by calling psource shutdown.
     */ 
    Cy_PdUtils_SwTimer_StopRange(pdstack_ctx->ptrTimerContext,
            GET_APP_TIMER_ID(pdstack_ctx, APP_PSOURCE_EN_TIMER),
            GET_APP_TIMER_ID(pdstack_ctx, APP_PSOURCE_EN_HYS_TIMER));
    call_psrc_ready_cbk(pdstack_ctx);

    /* SCP fault. */
    psrc_shutdown(pdstack_ctx, true);

    /* Set alert message */
    cy_pd_pd_do_t alert;
    alert.val = 0;
    alert.ado_alert.ocp = true;
    pdstack_ctx->dpmStat.alert = alert;

    /* Enqueue SCP fault event. */
    app_event_handler(pdstack_ctx, APP_EVT_VBUS_SCP_FAULT, NULL);

    return 0;
}
#endif /* VBUS_SCP_ENABLE */

#if VBUS_RCP_ENABLE
bool app_psrc_vbus_rcp_cbk(void * cbkContext, bool comp_out)
{
    /* Get the PD-Stack context from the USBPD context */
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *) cbkContext;
    cy_stc_pdstack_context_t * pdstack_ctx = get_pdstack_context(context->port);

    /* RCP fault. */
    psrc_shutdown(pdstack_ctx, true);

    cy_pd_pd_do_t alert;
    alert.val = 0;
    /* Treat RCP as equivalent to OVP and send an alert post fault recovery. */
    alert.ado_alert.ovp = true;
    pdstack_ctx->dpmStat.alert = alert;

    /* Notify the solution layer about the fault. */
    app_event_handler(pdstack_ctx, APP_EVT_VBUS_RCP_FAULT, NULL);

    return 0;
}
#endif /* VBUS_RCP_ENABLE */

#if ((VBUS_OVP_ENABLE) || (VBUS_UVP_ENABLE))

static void ovp_pwr_ready_cbk(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    /* Dummy callback to allow vbus discharge */
}

bool app_psrc_vbus_ovp_cbk(void *cbkContext, bool compOut)
{
    /* Get the PD-Stack context from the USBPD context */
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *) cbkContext;
    cy_stc_pdstack_context_t *ptrPdStackContext = get_pdstack_context(context->port);
    uint8_t port = ptrPdStackContext->port;
    app_status_t *app_stat = app_get_status(port);

    app_stat->psrc_volt = CY_PD_VSAFE_0V;
    psrc_select_voltage(ptrPdStackContext);

    /*OVP fault*/
    psrc_shutdown(ptrPdStackContext, true);

    if (compOut == true)
    {
        /* OVP fault condition. */

        /* Set alert message to be sent after fault recovery. */
        cy_pd_pd_do_t alert;
        alert.val = 0;
        alert.ado_alert.ovp = true;
        ptrPdStackContext->dpmStat.alert = alert;

        app_event_handler(ptrPdStackContext, APP_EVT_VBUS_OVP_FAULT, NULL);
        psrc_disable(ptrPdStackContext, ovp_pwr_ready_cbk);
    }
#if VBUS_UVP_ENABLE
    else
    {
        /* UVP fault condition. */

        /* 
         * UVP is a hardware cutoff in micro seconds and OCP is a software
         * debounce and cutoff in milli seconds. When there is a sudden change
         * in Vbus current, Vbus voltage dips and causes UVP to react first
         * rather than OCP. Sink expects an alert message for OCP, that will be
         * missed if UVP is received. Hence, mimic OCP alert in case of UVP as
         * well. This will be taken care only for non-PPS contracts.
         */
        /* Set alert message */
        cy_pd_pd_do_t alert;
        alert.val = 0;
        alert.ado_alert.ocp = true;
        ptrPdStackContext->dpmStat.alert = alert;

        app_event_handler(ptrPdStackContext, APP_EVT_VBUS_UVP_FAULT, NULL);
    }
#endif /* VBUS_UVP_ENABLE */

    return 0;
}
#endif /* ((VBUS_OVP_ENABLE) || (VBUS_UVP_ENABLE)) */

void psrc_select_voltage(cy_stc_pdstack_context_t *context)
{
    uint8_t port = context->port;
    app_status_t *app_stat = app_get_status(port);

    uint32_t select_volt = app_stat->psrc_volt;

    /* Don't drop voltage below 5 V. */
    if (app_stat->psrc_volt == CY_PD_VSAFE_0V)
    {
        app_stat->psrc_volt = CY_PD_VSAFE_5V;
    }

    /* 
     * Cap the selected voltage to the absolute maximum voltage that can be
     * applied to the cable. 
     */ 
    if (select_volt > VBUS_MAX_VOLTAGE)
    {
        select_volt = VBUS_MAX_VOLTAGE;
    }

    if(port == TYPEC_PORT_0_IDX)
    {
#if (PMG1B1_CHARGER_SINK_APP == 0)
        /* Fpr PMG1B1:Contract voltage 9V wiil be set by PWM in psrc_enable(). */
#if VBUS_SRC_HV_PWM_SLEW_RATE_ENABLE
        if(app_stat->psrc_volt != CY_PD_VSAFE_9V) 
#endif
        {
            APP_VBUS_SET_VOLT_P1(context, select_volt);
        }
#endif /* (PMG1B1_CHARGER_SINK_APP == 0) */
    }
}

void psrc_set_voltage(cy_stc_pdstack_context_t * context, uint16_t volt_mV)
{
    uint8_t port = context->port;
    app_status_t *app_stat = app_get_status(port);
    const cy_stc_pd_dpm_config_t *dpm_stat = &context->dpmConfig;
    app_stat->psrc_volt = volt_mV;

#if VBUS_OCP_ENABLE
    /* Leave OCP detection disabled while doing the voltage transition. */
    psrc_dis_ocp (context);
#endif /* VBUS_OCP_ENABLE */

    if ((app_stat->psrc_volt >= app_stat->psrc_volt_old) && (volt_mV != CY_PD_VSAFE_0V))
    {
        /* If voltage is being increased, make sure that OVP and RCP limits are moved up. */
        psrc_en_ovp (context);
        psrc_en_rcp (context);
    }
    else if ((app_stat->psrc_volt < app_stat->psrc_volt_old) && (volt_mV != CY_PD_VSAFE_0V))
    {
        /*
         * Enable UVP only if port partner is attached. We need to ensure that
         * UVP does not get enabled if VBUS is not applied, like in case of
         * HARD_RESET.
         */
        if ((dpm_stat->attach == true) && (app_stat->is_vbus_on == true))
        {
            psrc_en_uvp (context);
        }
    }

    psrc_select_voltage(context);
}

uint32_t psrc_get_voltage (cy_stc_pdstack_context_t *context)
{
    return app_get_status(context->port)->psrc_volt;
}

#if (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE)
static const uint32_t cc_rp_to_cur_map[] = {
    CUR_LEVEL_DEF,
    CUR_LEVEL_1_5A,
    CUR_LEVEL_3A
};
#endif /* (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE) */

void psrc_set_current (cy_stc_pdstack_context_t *context, uint16_t cur_10mA)
{
#if (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE)
    /* Update the OCP/SCP thresholds when required. */
    const cy_stc_pdstack_dpm_status_t *dpm_stat = &context->dpmStat;
    cy_stc_pd_dpm_config_t *dpm_config = &context->dpmConfig;
    uint32_t ocp_cur;

    if (dpm_config->contractExist)
    {
        ocp_cur = dpm_stat->srcSelPdo.src_gen.maxCurPower;
    }
    else
    {
        ocp_cur = cc_rp_to_cur_map[dpm_stat->srcCurLevel];
    }

#if VBUS_OCP_ENABLE
    if (context->ptrUsbPdContext->usbpdConfig->vbusOcpConfig->enable)
    {
        Cy_USBPD_Fault_Vbus_OcpEnable (context->ptrUsbPdContext, ocp_cur, app_psrc_vbus_ocp_cbk);
    }
#endif /* VBUS_OCP_ENABLE */

#if VBUS_SCP_ENABLE
    /* Enable SCP. */
    if (context->ptrUsbPdContext->usbpdConfig->vbusScpConfig->enable)
    {
        /* SCP Limit is set to 10A. */
        Cy_USBPD_Fault_Vbus_ScpEnable (context->ptrUsbPdContext, 1000, app_psrc_vbus_scp_cbk);
    }
#endif /* VBUS_SCP_ENABLE */

#endif /* (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE) */
}

void psrc_enable (cy_stc_pdstack_context_t * context,
        cy_pdstack_pwr_ready_cbk_t pwr_ready_handler)
{
    app_status_t* app_stat = app_get_status(context->port);
    const cy_stc_pdstack_dpm_status_t *dpm_stat = &(context->dpmStat);
    uint32_t intr_state;

    intr_state = Cy_SysLib_EnterCriticalSection();

    Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_TIMER), 
            GET_APP_TIMER_ID(context, APP_PSOURCE_DIS_EXT_DIS_TIMER));

    /* Turn on FETs only if dpm is enabled and there is no active fault condition. */
    if ((context->dpmConfig.dpmEnabled) && (dpm_stat->faultActive == false))
    {
#if (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE)
        /* Make sure OCP/SCP is enabled where required. The current parameter is not used. */
        psrc_set_current (context, CUR_LEVEL_3A);
#endif /* (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE) */

        /* Enable the regulator before turning on the FET */
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_MONITOR_TIMER), REGULATOR_TURN_ON_DELAY, app_psrc_tmr_cbk);

        /* Turn off VBus Discharge by default. */
        vbus_discharge_off(context);

        /* Turn on PSource FET */
        vbus_fet_on(context);

        if (pwr_ready_handler != NULL)
        {
            app_stat->psrc_rising = true;

            /* If the VBus voltage is dropping, turn the discharge path on. */
            if(app_stat->psrc_volt_old > app_stat->psrc_volt)
            {
                app_stat->psrc_rising = false;
                vbus_discharge_on(context);
            }
            app_stat->pwr_ready_cbk = pwr_ready_handler;

            /* Start Power source enable and monitor timers */
            Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_TIMER),
                                APP_PSOURCE_EN_TIMER_PERIOD, app_psrc_tmr_cbk);

            Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_MONITOR_TIMER),
                    APP_PSOURCE_EN_MONITOR_TIMER_PERIOD, app_psrc_tmr_cbk);
        }
#if ((PMG1B1_CHARGER_SINK_APP == 0) && VBUS_SRC_HV_PWM_SLEW_RATE_ENABLE)
        /*
         * Slew rate controlling PWM is started here (only for 5V->9V transfer)
         * to avoid PWM interrupt blocking by  vbus_get_value() procedure.
         */
        if((app_stat->psrc_volt == CY_PD_VSAFE_9V) && (app_stat->psrc_volt != app_stat->psrc_volt_old))
        {
            APP_VBUS_SET_VOLT_P1(context, CY_PD_VSAFE_9V);
        }
#endif /* (PMG1B1_CHARGER_SINK_APP == 0) */
    }
    Cy_SysLib_ExitCriticalSection(intr_state);
}

void psrc_disable(cy_stc_pdstack_context_t * context, cy_pdstack_pwr_ready_cbk_t pwr_ready_handler)
{
    uint8_t port = context->port;
    app_status_t* app_stat = app_get_status(port);
    uint32_t intr_state;

    intr_state = Cy_SysLib_EnterCriticalSection();

    Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext, GET_APP_TIMER_ID(context, APP_PSOURCE_EN_TIMER), 
            GET_APP_TIMER_ID(context, APP_PSOURCE_DIS_EXT_DIS_TIMER));

#if VBUS_UVP_ENABLE
    psrc_dis_uvp(context);
#endif /* VBUS_UVP_ENABLE */

    if(app_stat->psrc_volt_old <= CY_PD_VSAFE_5V)
    {
        psrc_shutdown(context, false);
        Cy_SysLib_DelayUs(20);
    }
    else
    {
        psrc_set_voltage(context, CY_PD_VSAFE_5V);
    }

    app_stat->psrc_volt_old = CY_PD_VSAFE_0V;

    if ((pwr_ready_handler != NULL) && (context->dpmConfig.dpmEnabled))
    {
        /* Turn on discharge to get the voltage to drop faster. */
        vbus_discharge_on(context);
        app_stat->pwr_ready_cbk = pwr_ready_handler;
        /*Start Power source enable and monitor timer*/
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_DIS_TIMER),
                APP_PSOURCE_DIS_TIMER_PERIOD, app_psrc_tmr_cbk);
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, GET_APP_TIMER_ID(context, APP_PSOURCE_DIS_MONITOR_TIMER),
                APP_PSOURCE_DIS_MONITOR_TIMER_PERIOD, app_psrc_tmr_cbk);
    }
    else
    {
        psrc_shutdown(context, true);
    }

    Cy_SysLib_ExitCriticalSection(intr_state);
}

static void psrc_dis_ovp(cy_stc_pdstack_context_t * context)
{
#if VBUS_OVP_ENABLE
    app_ovp_disable (context, true);
#endif /* VBUS_OVP_ENABLE */
}

static void psrc_dis_ocp(cy_stc_pdstack_context_t * context)
{
#if VBUS_OCP_ENABLE
    if (context->ptrUsbPdContext->usbpdConfig->vbusOcpConfig->enable)
    {
        /* Make sure the OCP debounce timer has been stopped. */
        Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, CY_PDSTACK_GET_PD_TIMER_ID(context, CY_PDSTACK_PD_OCP_DEBOUNCE_TIMER));
        Cy_USBPD_Fault_Vbus_OcpDisable (context->ptrUsbPdContext, false);
    }
#endif /* VBUS_OCP_ENABLE */
}

static void psrc_dis_scp(cy_stc_pdstack_context_t * context)
{
#if VBUS_SCP_ENABLE
    if (context->ptrUsbPdContext->usbpdConfig->vbusScpConfig->enable)
    {
        Cy_USBPD_Fault_Vbus_ScpDisable (context->ptrUsbPdContext);
    }
#endif /* VBUS_SCP_ENABLE */
}

static void psrc_dis_uvp(cy_stc_pdstack_context_t * context)
{
#if VBUS_UVP_ENABLE
    app_uvp_disable (context, true);
#endif /* VBUS_UVP_ENABLE */
}

static void psrc_dis_rcp(cy_stc_pdstack_context_t * context)
{
#if VBUS_RCP_ENABLE
    if (context->ptrUsbPdContext->usbpdConfig->vbusRcpConfig->enable)
    {
        Cy_USBPD_Fault_Vbus_RcpDisable (context->ptrUsbPdContext);
    }
#endif /* VBUS_RCP_ENABLE */
}

static void psrc_shutdown(cy_stc_pdstack_context_t * context, bool discharge_dis)
{
    uint8_t port = context->port;

    /* Turn Off Source FET */
    vbus_fet_off(context);

    if(discharge_dis == true)
    {
        vbus_discharge_off(context);
    }

    /* Disable OVP/OCP/UVP */
    psrc_dis_ovp(context);
    psrc_dis_uvp(context);
    psrc_dis_ocp(context);
    psrc_dis_scp(context);
    psrc_dis_rcp(context);

    (void) port;
}

static void psrc_en_ovp(cy_stc_pdstack_context_t * context)
{
#if (VBUS_OVP_ENABLE)
    app_ovp_enable(context, app_get_status(context->port)->psrc_volt,
            true, app_psrc_vbus_ovp_cbk);
#endif /* (VBUS_OVP_ENABLE) */
}

static void psrc_en_rcp(cy_stc_pdstack_context_t * context)
{
#if VBUS_RCP_ENABLE
    if (context->ptrUsbPdContext->usbpdConfig->vbusRcpConfig->enable)
    {
        Cy_USBPD_Fault_Vbus_RcpEnable(context->ptrUsbPdContext, app_get_status(context->port)->psrc_volt, app_psrc_vbus_rcp_cbk);
    }
#endif /* VBUS_RCP_ENABLE */
}

static void psrc_en_uvp(cy_stc_pdstack_context_t * context)
{
#if VBUS_UVP_ENABLE
    /* Using the same callback as OVP as behavior is same. */
    app_uvp_enable(context, app_get_status(context->port)->psrc_volt, true, app_psrc_vbus_ovp_cbk);
#endif /* VBUS_UVP_ENABLE */
}

#endif /* (!CY_PD_SINK_ONLY) */

/* End of File */
