/*******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Airoha Technology Corp. (C) 2021
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AIROHA SOFTWARE")
*  RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE AIROHA SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. AIROHA SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY AIROHA SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT AIROHA'S OPTION, TO REVISE OR REPLACE THE AIROHA SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*******************************************************************************/

/* FILE NAME:   portSetting.c
 * PURPOSE:
 *      CGI and SSI function of port setting web page.
 * NOTES:
 */

/* INCLUDE FILE DECLARATIONS
 */
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "mw_error.h"
#include "db_api.h"
#include "httpd_queue.h"
#include "web.h"
#ifdef AIR_SUPPORT_SFP
#include "sfp_task.h"
#include "sfp_pin.h"
#include "sfp_db.h"
#include "sfp_util.h"
#endif /* AIR_SUPPORT_SFP */

/* NAMING CONSTANT DECLARATIONS
 */
#define MW_PORT_STATE_LNE       (9)
#define MW_PORT_STATE_NUM       (4)
#define MW_SGMII_PORT_START     (24)
#ifdef AIR_SUPPORT_SFP
#define MW_SFP_PORT_STATE       (MAX_PORT_NUM * 6 + 2)
#else
#define MW_SFP_PORT_STATE       (0)
#endif /* AIR_SUPPORT_SFP */

#define MW_STATE_RANGE          (5)
#define MW_STATE_STATUS         (0)
#define MW_STATE_SPEED          (1)
#define MW_STATE_DUPLEX         (2)
#define MW_STATE_FLOWCTRL       (3)
#define MW_STATE_ABILITY        (4)
#define MW_STATE_NO_CHANGE      (0xFF)

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* GLOBAL VARIABLE DECLARATIONS
 */
PORTSETTING_ERR_TYPE_T _portSetting_errType = PORTSETTING_NO_ERR;

/* LOCAL SUBPROGRAM DECLARATIONS
 */

/* STATIC VARIABLE DECLARATIONS
 */

/* LOCAL SUBPROMGRAM BODIES
*/

/* EXPORTED SUBPROGRAM BODIES
 */
MW_ERROR_NO_T
ssi_get_portCur_xmlHandle(
    I32_T *ptr_length,
    struct tcp_pcb *ptr_pcb,
    UI32_T apiflags)
{
    MW_ERROR_NO_T err = MW_E_OK;
    UI32_T total_len = 0, len = 0;
    C8_T tmpbuf[(MAX_PORT_NUM * (MW_PORT_STATE_NUM * 2)) + MW_PORT_STATE_NUM + MW_SFP_PORT_STATE] = {0};

    /* DB variable */
    DB_MSG_T *ptr_msg = NULL;
    UI8_T *ptr_data = NULL;
    UI16_T i = 0, num = 0, size = 0;
    UI8_T f_idx = 0;
#ifdef AIR_SUPPORT_SFP
    UI32_T                    unit = 0;
    AIR_ERROR_NO_T            rc = AIR_E_OK;
    SFP_DB_PORT_TYPE_T        port_type = SFP_DB_PORT_TYPE_LAST;
    SFP_DB_PORT_SERDES_MODE_T serdes_mode = SFP_DB_PORT_SERDES_MODE_UNKNOWN;
    BOOL_T                    i2c_flag = FALSE;
#endif /* AIR_SUPPORT_SFP */

    for(i = 0; i < MW_PORT_STATE_NUM; i++)
    {
        f_idx = (PORT_OPER_STATUS + i);
        err = httpd_queue_getData(PORT_OPER_INFO, f_idx, DB_ALL_ENTRIES, &ptr_msg, &size, (void**)&ptr_data);
        for(num = 0; num < PLAT_MAX_PORT_NUM; num++)
        {
            if(PORT_OPER_SPEED == f_idx)
            {
                len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, "%d,", (ptr_data[num] + 1));
            }
            else
            {
                len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, "%d,", ptr_data[num]);
            }
        }
        len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, "&");
        MW_FREE(ptr_msg);
    }
#ifdef AIR_SUPPORT_SFP
    err = httpd_queue_getData(PORT_OPER_INFO, PORT_OPER_MODE, DB_ALL_ENTRIES, &ptr_msg, &size, (void**)&ptr_data);
    /* Send port type and serdes mode */
    for(i = 0; i < PLAT_MAX_PORT_NUM; i++)
    {
        i2c_flag = FALSE;
        sfp_db_port_getPortType(unit, i + 1, ((UI8_T *)ptr_data)[i], &port_type);
        if(AIR_E_OK != rc)
        {
            LWIP_DEBUGF(HTTPD_DEBUG, ("parse port type failed, rc:%d\n", rc));
            break;
        }
        if((SFP_DB_PORT_TYPE_SERDES == port_type) || (SFP_DB_PORT_TYPE_COMBO_SERDES == port_type))
        {
            rc = sfp_db_parsePortMode(((UI8_T *)ptr_data)[i], NULL, NULL, NULL, &serdes_mode);
            if(AIR_E_OK != rc)
            {
                LWIP_DEBUGF(HTTPD_DEBUG, ("parse serdes mode failed, rc:%d\n", rc));
                break;
            }
            sfp_port_checkPinInitState(unit, i + 1, (SFP_PIN_SDA_INIT_SUCCEED | SFP_PIN_IO_INIT_SUCCEED), &i2c_flag);
        }
        len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, "%d-%d-%d,", port_type, serdes_mode, (TRUE == i2c_flag) ? 1 : 0);
    }
    len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, "&");
    MW_FREE(ptr_msg);
#endif /* AIR_SUPPORT_SFP */
    err = send_format_response(&len, ptr_pcb, apiflags, tmpbuf);
    if(MW_E_OK != err)
    {
        return err;
    }
    total_len += len;
    *ptr_length = total_len;

    return err;
}

MW_ERROR_NO_T
ssi_get_port_setting_info_Handle(
    I32_T *ptr_length,
    struct tcp_pcb *ptr_pcb,
    UI32_T apiflags)
{
    MW_ERROR_NO_T err = MW_E_OK, ret = MW_E_OK;
    UI32_T total_len = 0, len = 0;
    C8_T tmpbuf[(MAX_PORT_NUM * 3)] = {0};

    /* DB variable */
    DB_MSG_T *ptr_msg = NULL;
    UI8_T *ptr_data = NULL;
    UI8_T f_idx = 0;
#ifdef AIR_SUPPORT_SFP
    AIR_PORT_SERDES_MODE_T serdes_mode = AIR_PORT_SERDES_MODE_LAST;
#endif

    /* Port variable */
    C8_T param[MW_STATE_RANGE][MW_PORT_STATE_LNE] =
    {
        "state",
        "spd_cfg",
        "mode_cfg",
        "fc_cfg",
        "ability"
    };
    UI16_T size = 0, i = 0, num = 0;

    /* Buffer variable */
    UI32_T tmplen = 0;
    UI32_T strlen = HTTPD_MAX_RESPONSE_CHUNKBUFF_LEN;
    C8_T *ptr_ssi_str = NULL;
    if (MW_E_NO_MEMORY == osapi_calloc(strlen, HTTPD_QUEUE_CLI, (void **)&ptr_ssi_str))
    {
        return MW_E_NO_MEMORY;
    }

    tmplen += snprintf(ptr_ssi_str + tmplen, strlen - tmplen, "var max_port_num = [%d];\n", PLAT_MAX_PORT_NUM);
    tmplen += snprintf(ptr_ssi_str + tmplen, strlen - tmplen, "var sgmii_port_begin = [%d];\n", MW_SGMII_PORT_START);
    tmplen += snprintf(ptr_ssi_str + tmplen, strlen - tmplen, "var all_info = {\n");

    /* Request DB for port admin setting of status, speed, automatic and flow control */
    for(i = 0; i < MW_STATE_RANGE; i++)
    {
        f_idx = (PORT_ADMIN_STATUS + i);
        err = httpd_queue_getData(PORT_CFG_INFO, f_idx, DB_ALL_ENTRIES, &ptr_msg, &size, (void**)&ptr_data);
        if(MW_E_OK == err)
        {
            len = 0;
            memset(tmpbuf, 0, sizeof((MAX_PORT_NUM * 3)));
            for(num = 0; num < PLAT_MAX_PORT_NUM; num++)
            {
#ifdef AIR_SUPPORT_SFP
                if ((TRUE == sfp_port_is_comboSerdesPort(0, num + 1)) ||
                    (TRUE == sfp_port_is_serdesPort(0, num + 1)))
                {
                    err = air_port_getSerdesMode(0, num + 1, &serdes_mode);
                }
                else
                {
                    serdes_mode = AIR_PORT_SERDES_MODE_SGMII;
                    err = AIR_E_OK;
                }
                if (AIR_E_OK == ret)
                {
                    if (f_idx == PORT_ADMIN_SPD_ABILITY)
                    {
                        sfp_port_correctSpeedAbility(0, num + 1, serdes_mode, &ptr_data[num]);
                    }
                    if (f_idx == PORT_ADMIN_SPEED)
                    {
                        sfp_port_correctSpeed(0, num + 1, serdes_mode, &ptr_data[num]);
                    }
                    if (f_idx == PORT_ADMIN_FLOW_CTRL)
                    {
                        sfp_port_correctFlowctrl(0, num + 1, serdes_mode, &ptr_data[num]);
                    }
                }
#endif
                len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, "%d,", ptr_data[num]);
            }
            MW_FREE(ptr_msg);
            tmplen += snprintf(ptr_ssi_str + tmplen, strlen - tmplen, "%s:[%s],\n", param[i], tmpbuf);
        }
        else
        {
            break;
        }
    }
    if(MW_E_OK != err)
    {
        osapi_free(ptr_ssi_str);
        return err;
    }

    /* Port trunk status */
    err = httpd_queue_getData(PORT_CFG_INFO, PORT_TRUNK_ID, DB_ALL_ENTRIES, &ptr_msg, &size, (void**)&ptr_data);
    if(MW_E_OK == err)
    {
        len = 0;
        memset(tmpbuf, 0, sizeof((MAX_PORT_NUM * 2)));
        for(num = 0; num < PLAT_MAX_PORT_NUM; num++)
        {
            len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, "%d,", ptr_data[num]);
        }
        MW_FREE(ptr_msg);
        tmplen += snprintf(ptr_ssi_str + tmplen, strlen - tmplen, "trunk_info:[%s],\n", tmpbuf);
    }
    else
    {
        osapi_free(ptr_ssi_str);
        return err;
    }

    tmplen += snprintf(ptr_ssi_str + tmplen, strlen - tmplen, "};\n");

    /* Reqeust DB for port loop status information */
    err = httpd_queue_getData(PORT_OPER_INFO, PORT_LOOP_STATE, DB_ALL_ENTRIES, &ptr_msg, &size, (void**)&ptr_data);
    if(MW_E_OK == err)
    {
        len = 0;
        memset(tmpbuf, 0, sizeof((MAX_PORT_NUM * 2)));
        for(i = 0; i < PLAT_MAX_PORT_NUM; i++)
        {
            len += snprintf(tmpbuf + len, sizeof(tmpbuf) - len, "%d,", ptr_data[i]);
        }
        MW_FREE(ptr_msg);
        tmplen += snprintf(ptr_ssi_str + tmplen, strlen - tmplen, "var portState=[%s];\n", tmpbuf);
    }
    else
    {
        osapi_free(ptr_ssi_str);
        return err;
    }
    /* return error type */
    err = send_format_response(&total_len, ptr_pcb, apiflags, "<script>%svar errType=%d;\n</script>\n", ptr_ssi_str, _portSetting_errType);
    osapi_free(ptr_ssi_str);
    if(MW_E_OK != err)
    {
        return err;
    }
    _portSetting_errType = PORTSETTING_NO_ERR;

    *ptr_length = total_len;

    return err;
}

MW_ERROR_NO_T
cgi_set_handle_portSetting(
    I32_T iIndex,
    I32_T iNumParams,
    C8_T *ptr_pcParam[],
    C8_T *ptr_pcValue[])
{
    MW_ERROR_NO_T rc = MW_E_OK;
    UI32_T portMap = 0, i = 0, j = 0;
    UI8_T curState[MW_STATE_RANGE] = {0};
    UI8_T state_status = 0;
    /* DB variable */
    DB_MSG_T *ptr_msg = NULL;
    UI8_T *ptr_data = NULL;
    UI16_T size = 0;
    C8_T loop_state[(MAX_PORT_NUM * 2)] = {0};
#ifdef AIR_SUPPORT_SFP
    UI32_T trunkBitMap = 0;
    UI8_T port_mode = 0;
    UI32_T unit = 0;
    AIR_ERROR_NO_T err = AIR_E_OK;
    SFP_DB_PORT_SERDES_MODE_T cur_serdes_mode = SFP_DB_PORT_SERDES_MODE_UNKNOWN;
    SFP_DB_PORT_SERDES_MODE_T old_serdes_mode = SFP_DB_PORT_SERDES_MODE_UNKNOWN;
    AIR_PORT_SERDES_MODE_T serdes_mode = AIR_PORT_SERDES_MODE_LAST;
#endif /* AIR_SUPPORT_SFP */

    memset(curState, MW_STATE_NO_CHANGE, MW_STATE_RANGE);
    /* Parser name=value from cgi parameter */
    for(i = 0; i < iNumParams; i++)
    {
        if(!strcmp(ptr_pcParam[i], "port_bit"))
        {
            portMap = atoi(ptr_pcValue[i]);
        }
        if(!strcmp(ptr_pcParam[i], "state"))
        {
            curState[MW_STATE_STATUS] = atoi(ptr_pcValue[i]);
        }
        if(!strcmp(ptr_pcParam[i], "speed"))
        {
            curState[MW_STATE_SPEED] = atoi(ptr_pcValue[i]);
        }
        if(!strcmp(ptr_pcParam[i], "duplex"))
        {
            curState[MW_STATE_DUPLEX] = atoi(ptr_pcValue[i]);
        }
        if(!strcmp(ptr_pcParam[i], "flowcontrol"))
        {
            curState[MW_STATE_FLOWCTRL] = atoi(ptr_pcValue[i]);
        }
        if(!strcmp(ptr_pcParam[i], "ability"))
        {
            curState[MW_STATE_ABILITY] = atoi(ptr_pcValue[i]);
        }
#ifdef AIR_SUPPORT_SFP
        if (0 == strcmp(ptr_pcParam[i],"trunkBitMap"))
        {
            trunkBitMap = atoi(ptr_pcValue[i]);
        }
        if(!strcmp(ptr_pcParam[i], "serdes_mode"))
        {
            old_serdes_mode = atoi(ptr_pcValue[i]);
        }
#endif /* AIR_SUPPORT_SFP */
    }
#ifdef AIR_SUPPORT_SFP
    CGI_UTILITY_REVISE_PBMP_BY_TRUNK_INFO(trunkBitMap,&portMap,0);
#endif
    /* Reqeust DB for port loop status information */
    rc = httpd_queue_getData(PORT_OPER_INFO, PORT_LOOP_STATE, DB_ALL_ENTRIES, &ptr_msg, &size, (void**)&ptr_data);
    if(MW_E_OK == rc)
    {
        memcpy(loop_state, ptr_data, PLAT_MAX_PORT_NUM * sizeof(C8_T));
        MW_FREE(ptr_msg);
    }
    else
    {
        return rc;
    }

    //osapi_printf("portMap: 0x%x\n", portMap);

    /* Update port setting to DB */
    for(i = 0; i < PLAT_MAX_PORT_NUM; i++)
    {
        if((portMap & (0x01 << i)) && (0 == loop_state[i]))
        {
 
      #if 0    
      		osapi_printf("Port Index: %d\n", i);
            osapi_printf("Configuration Parameters:\n");
            osapi_printf("  State: %d\n", curState[MW_STATE_STATUS]);
            osapi_printf("  Speed: %d\n", curState[MW_STATE_SPEED]);
            osapi_printf("  Duplex: %d\n", curState[MW_STATE_DUPLEX]);
            osapi_printf("  Flow Control: %d\n", curState[MW_STATE_FLOWCTRL]);
            osapi_printf("  Ability: %d\n", curState[MW_STATE_ABILITY]);
      #endif      
#ifdef AIR_SUPPORT_SFP
            if (0 != curState[MW_STATE_STATUS])
            {
                if ((TRUE == sfp_port_is_comboSerdesPort(0, i + 1)) ||
                    (TRUE == sfp_port_is_serdesPort(0, i + 1)))
                {
                    rc = air_port_getSerdesMode(0, i + 1, &serdes_mode);
                }
                else
                {
                    serdes_mode = AIR_PORT_SERDES_MODE_SGMII;
                    rc = AIR_E_OK;
                }
                if (AIR_E_OK == rc)
                {
                    if (MW_STATE_NO_CHANGE != curState[MW_STATE_ABILITY])
                    {
                        sfp_port_correctSpeedAbility(0, (i + 1), serdes_mode, &curState[MW_STATE_ABILITY]);
                    }
                    if (MW_STATE_NO_CHANGE != curState[MW_STATE_SPEED])
                    {
                        sfp_port_correctSpeed(0, (i + 1), serdes_mode, &curState[MW_STATE_SPEED]);
                    }
                    if (MW_STATE_NO_CHANGE != curState[MW_STATE_FLOWCTRL])
                    {
                        sfp_port_correctFlowctrl(0, (i + 1), serdes_mode, &curState[MW_STATE_FLOWCTRL]);
                    }
                }
            }
#endif /* AIR_SUPPORT_SFP */
            /* Following SDK note:
             * 1. Disable admin status
             * 2. Configure port based on user settings
             * 3. Enable admin status
             */
            state_status = 0;
            rc = httpd_queue_setData(M_UPDATE, PORT_CFG_INFO, PORT_ADMIN_STATUS, (i + 1), &state_status, sizeof(UI8_T));
            if(MW_E_OK != rc)
            {
                return rc;
            }
            
            //osapi_printf("Port number: %d, Admin status disabled\n", (i + 1));
            /* There's no need to configure port if admin status is disabled by user. */
            if (0 == curState[MW_STATE_STATUS])
            {
                continue;
            }

            for(j = MW_STATE_DUPLEX; j < MW_STATE_RANGE; j++)
            {
                if(MW_STATE_NO_CHANGE != curState[j])
                {
                    rc = httpd_queue_setData(M_UPDATE, PORT_CFG_INFO, (PORT_ADMIN_STATUS + j), (i + 1), &curState[j], sizeof(UI8_T));
                    if(MW_E_OK != rc)
                    {
                        return rc;
                    }
                    //osapi_printf("Port number: %d, configuration[%d] completed\n", (i + 1), (PORT_ADMIN_STATUS + j)); 
                    osapi_delay(50);
                }
            }
            
            
            /*The AN mode is set in PORT_ADMIN_SPEED. Do it at last*/
            if(MW_STATE_NO_CHANGE != curState[MW_STATE_SPEED])
            {
                rc = httpd_queue_setData(M_UPDATE, PORT_CFG_INFO, PORT_ADMIN_SPEED, (i + 1), &curState[MW_STATE_SPEED], sizeof(UI8_T));
                if(MW_E_OK != rc)
                {
                    return rc;
                }
               
            }
			
            //osapi_printf("Port number: %d, Speed configuration completed\n", (i + 1));


            /* It will link down when change AN mode to Force mode 100M. In such case, wait 50ms before enable admin status again. */
            if (((AIR_PORT_PHY_AN_ADV_FLAGS_100FUDX >= curState[MW_STATE_ABILITY]) &&
                (0 != curState[MW_STATE_SPEED]) &&
                (MW_STATE_NO_CHANGE != curState[MW_STATE_SPEED])))
            {
                osapi_delay(50);
            }
            if(0 != curState[MW_STATE_STATUS])
            {
                state_status = 1;
                rc = httpd_queue_setData(M_UPDATE, PORT_CFG_INFO, PORT_ADMIN_STATUS, (i + 1), &state_status, sizeof(UI8_T));
                if(MW_E_OK != rc)
                {
                    return rc;
                }
                
            }
            //osapi_printf("Port number: %d, Admin status enabled\n", (i + 1));
            
        }
    }

    return rc;
}

