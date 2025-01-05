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

/* FILE NAME:  sys_mgmt.c
 * PURPOSE:
 * It provides SYS_MGMT module API.
 *
 * NOTES:
 *
 */

/* INCLUDE FILE DECLARATIONS
 */
#include "inc/sys_mgmt.h"
#include "lwip/api.h"
#ifdef AIR_SUPPORT_SNMP
#include "lwip/snmp.h"
#include "lwip/apps/snmp.h"
#endif
#ifdef AIR_SUPPORT_MQTTD
#include "mqttd.h"
#endif
#include "mw_tlv.h"
#include "web.h"

/* GLOBAL VARIABLE DECLARATIONS
*/
SYS_MGMT_T           sys_mgmt_info;
UI8_T                sys_mgmt_debug_level = SYS_MGMT_DEBUG_LEVEL_DISABLE;
QueueSetHandle_t     sys_mgmt_handle_set;
msghandle_t          db_reg_handle;
threadhandle_t       sys_mgmt_task_handle;
TimerHandle_t        sys_mgmt_timer_handle;
#if LWIP_NETIF_EXT_STATUS_CALLBACK
netif_ext_callback_t sys_mgmt_netif_callback;
#endif
#ifndef AIR_SUPPORT_DHCP_SNOOP
UI16_T               dhcp_acl_id = MW_ACL_ID_INVALID;
#endif /* AIR_SUPPORT_DHCP_SNOOP */
#ifdef AIR_SUPPORT_SNMP
UI16_T               snmp_linkup = 0;
UI16_T               snmp_send_coldwarm_start = 1;
#endif
static UI8_T  _mw_attack_prevention_global_state_ref_cnt = 0;
/* LOCAL SUBPROGRAM DECLARATIONS
 */
void sys_mgmt_get_default_ip(void);
void sys_mgmt_update_default_to_oper(void);
MW_ERROR_NO_T
sys_mgmt_queue_send(const UI8_T method,
                           const UI8_T t_idx,
                           const UI8_T f_idx,
                           const UI16_T e_idx,
                           const void *ptr_data,
                           const UI16_T size,
                           const C8_T *ptr_name);

static MW_ERROR_NO_T
_sys_mgmt_handle_db_ping_client(
    const DB_REQUEST_TYPE_T *ptr_request,
    const void  *ptr_data);

/* LOCAL SUBPROGRAM BODIES
*/
static void sys_mgmt_netif_ip_set(UI8_T is_dhcp)
{
    ip4_addr_t ip, mask, gw;
    ip_addr_t dns;
    struct netif *xNetIf = netif_get_by_index(netif_num_get());

    if (NULL == xNetIf)
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "netif is NULL");
        return;
    }

    if (FALSE == is_dhcp)
    {
        if ((FALSE == ip4_addr_isany_val(sys_mgmt_info.static_ip)) && (FALSE == ip4_addr_isany_val(sys_mgmt_info.static_mask)))
        {
            ip4_addr_copy(ip, sys_mgmt_info.static_ip);
            ip4_addr_copy(mask, sys_mgmt_info.static_mask);
            ip4_addr_copy(gw, sys_mgmt_info.static_gw);
            ip_addr_set_ip4_u32_val(dns, ip4_addr_get_u32(&sys_mgmt_info.static_dns));
        }
        else
        {
            ip4_addr_copy(ip, sys_mgmt_info.def_ip);
            ip4_addr_copy(mask, sys_mgmt_info.def_mask);
            ip4_addr_copy(gw, sys_mgmt_info.def_gw);
            ip_addr_set_ip4_u32_val(dns, ip4_addr_get_u32(&sys_mgmt_info.def_dns));
        }
        osapi_printf("Set interface IP as %s manually.\n", ip4addr_ntoa(&ip));
		osapi_printf("Set interface mask as %s.\n", ip4addr_ntoa(&mask));
		osapi_printf("Set interface gw as %s.\n", ip4addr_ntoa(&gw));
		osapi_printf("Set interface dns as 0x%x.\n", ip_addr_get_ip4_u32(&dns));
    }
    else
    {
        ip4_addr_set_u32(&ip, sys_mgmt_info.oper_ip);
        ip4_addr_set_u32(&mask, sys_mgmt_info.oper_mask);
        ip4_addr_set_u32(&gw, sys_mgmt_info.oper_gw);
        ip_addr_set_ip4_u32_val(dns, sys_mgmt_info.oper_dns);
        osapi_printf("Set interface IP as %s\n", ip4addr_ntoa(&ip));
    }

    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "ip=0x%x, mask=0x%x, gw=0x%x, dns=0x%x",
                   ip4_addr_get_u32(&ip),
                   ip4_addr_get_u32(&mask),
                   ip4_addr_get_u32(&gw),
                   ip_addr_get_ip4_u32(&dns));

    if ((FALSE == ip4_addr_isany_val(ip)) && (FALSE == ip4_addr_isany_val(mask)))
    {
        dns_setserver(0, &dns);
        netif_set_addr(xNetIf, &ip, &mask, &gw);
        if (FALSE == is_dhcp)
        {
            MW_IPV4_T dns_val = ip_addr_get_ip4_u32(&dns);
            sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_DNS, DB_ALL_ENTRIES, &dns_val, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
        }
    }
    else
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "Invalid parameter.");
    }
}

/* FUNCTION NAME:   sys_mgmt_dhcp_set
 * PURPOSE:
 *      This API is used to enable/disable sys_mgmt dhcp mode.
 *
 * INPUT:
 *      enable       --  admin mode
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
void sys_mgmt_dhcp_set(UI8_T enable)
{
    u8_t netif_num = netif_num_get();
    struct netif *xNetIf = netif_get_by_index(netif_num);

    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "enable=%d", enable);

    if (enable == sys_mgmt_info.dhcp_enable)
        return;

    if (enable)
    {
        sys_mgmt_info.dhcp_enable = enable;

#ifndef AIR_SUPPORT_DHCP_SNOOP
        if (MW_ACL_ID_INVALID == dhcp_acl_id)
        {
            UI32_T unit = 0;
            UI8_T index = 0;
            AIR_ACL_RULE_T acl_rule;
            if(MW_E_OK == mw_acl_mutex_take())
            {
                for (index = MW_ACL_ID_DYNAMIC_MIN; index <= MW_ACL_ID_DYNAMIC_MAX; index++)
                {
                    if (air_acl_getRule(unit, index, &acl_rule) == AIR_E_OK)
                    {
                        AIR_ACL_ACTION_T action;
                        AIR_ERROR_NO_T   rc;
                        UI8_T            i;
                        if (FALSE == acl_rule.rule_en)
                        {
                            osapi_memset(&acl_rule, 0, sizeof(AIR_ACL_RULE_T));
                            acl_rule.rule_en = TRUE;
                            AIR_PORT_BITMAP_COPY(acl_rule.portmap, PLAT_PORT_BMP_TOTAL);
                            AIR_PORT_DEL(acl_rule.portmap, PLAT_CPU_PORT);
                            acl_rule.end = TRUE;
                            acl_rule.key.etype = ETHTYPE_IP;
                            acl_rule.mask.etype = 0x3;
                            acl_rule.key.next_header = MW_IPPROTO_UDP;
                            acl_rule.key.dip = IPADDR_BROADCAST;
                            acl_rule.mask.dip = 0xf;
                            acl_rule.key.dport = MW_DHCP_CLIENT_PORT;
                            acl_rule.mask.dport = 0x3;
                            acl_rule.field_valid |= ((1U << AIR_ACL_ETYPE_KEY) | (1U << AIR_ACL_NEXT_HEADER_KEY) |
                                                    (1U << AIR_ACL_DIP_KEY) | (1U << AIR_ACL_DPORT_KEY));
                            rc = air_acl_setRule(unit, index, &acl_rule);
                            if (rc != AIR_E_OK)
                            {
                                osapi_printf("Add DHCP ACL rule entry-id %d failed, rc=%d.\n", index, rc);
                                break;
                            }

                            osapi_memset(&action, 0, sizeof(AIR_ACL_ACTION_T));
                            action.port_fw = MW_ACL_ACT_PORT_FW_CPU_INCLUDE;
                            action.pri_user = MW_ACL_RX_PRIORITY_NOMRAL_PACKET;
                            action.field_valid |= (1U << AIR_ACL_FW_PORT) | (1U << AIR_ACL_PRI);
                            rc = air_acl_setAction(unit, index, &action);
                            if (AIR_E_OK == rc)
                            {
                                dhcp_acl_id = index;
                            }
                            else
                            {
                                osapi_printf("Add DHCP ACL rule entry-id %d action fail, rc=%d.\n", index, rc);
                                air_acl_delRule(unit, index);
                            }
                            break;
                        }
                    }
                    else
                    {
                        osapi_printf("Get ACL rule entry-id %d failed\n", index);
                    }
                }
                mw_acl_mutex_release();
            }
        }
#endif /* AIR_SUPPORT_DHCP_SNOOP */
#if MW_DHCP
        sys_mgmt_info.oper_ip = 0;
        sys_mgmt_info.oper_mask = 0;
        sys_mgmt_info.oper_gw = 0;
        sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_ADDR, DB_ALL_ENTRIES, &sys_mgmt_info.oper_ip, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
        sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_MASK, DB_ALL_ENTRIES, &sys_mgmt_info.oper_mask, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
        sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_GW, DB_ALL_ENTRIES, &sys_mgmt_info.oper_gw, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
        if(MW_AUTODNS_ENABLE == sys_mgmt_info.autodns_enable)
        {
            sys_mgmt_info.oper_dns = 0;
            sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_DNS, DB_ALL_ENTRIES, &sys_mgmt_info.oper_dns, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
        }
        dhcp_start(xNetIf);
#else
        osapi_printf("MW_DHCP is not active\n");
#endif
    }
    else
    {
        sys_mgmt_info.autodns_enable = MW_AUTODNS_DISABLE;
        sys_mgmt_info.dhcp_enable = MW_DHCP_DISABLE;

        /* Set up the network interface. */
#if MW_DHCP
        dhcp_stop(xNetIf);
#else
        osapi_printf("MW_DHCP is not active\n");
#endif
        sys_mgmt_netif_ip_set(FALSE);
#ifndef AIR_SUPPORT_DHCP_SNOOP
        if (dhcp_acl_id != MW_ACL_ID_INVALID)
        {
            UI32_T         unit = 0;
            AIR_ERROR_NO_T rc;

            rc = air_acl_delAction(unit, dhcp_acl_id);
            if (rc != AIR_E_OK)
            {
                osapi_printf("Delete DHCP ACL rule entry-id %d action failed, rc %d.\n", dhcp_acl_id, rc);
            }
            rc = air_acl_delRule(unit, dhcp_acl_id);
            if (rc != AIR_E_OK)
            {
                osapi_printf("Delete DHCP ACL rule entry-id %d rule failed, rc %d.\n", dhcp_acl_id, rc);
            }
            dhcp_acl_id = MW_ACL_ID_INVALID;
        }
#endif /* AIR_SUPPORT_DHCP_SNOOP */
    }
    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "return");
    return;
}

/* FUNCTION NAME:   sys_mgmt_ip_config_set
 * PURPOSE:
 *      This API is used to set ip config.
 *
 * INPUT:
 *      ip_addr
 *      ip_mask
 *      ip_gw
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
static void sys_mgmt_ip_config_set(UI32_T ip_addr, UI32_T ip_mask, UI32_T ip_gw, UI32_T ip_dns)
{
    UI8_T ip_change = FALSE;

    if (ip_addr != ip4_addr_get_u32(&sys_mgmt_info.static_ip))
    {
        ip4_addr_set_u32(&sys_mgmt_info.static_ip, ip_addr);
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "set static_ip = 0x%x", ip4_addr_get_u32(&sys_mgmt_info.static_ip));
    }

    if (ip_mask != ip4_addr_get_u32(&sys_mgmt_info.static_mask))
    {
        ip4_addr_set_u32(&sys_mgmt_info.static_mask, ip_mask);
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "set static_mask = 0x%x", ip4_addr_get_u32(&sys_mgmt_info.static_mask));
    }

    if (ip_gw != ip4_addr_get_u32(&sys_mgmt_info.static_gw))
    {
        ip4_addr_set_u32(&sys_mgmt_info.static_gw, ip_gw);
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "set static_gw = 0x%x", ip4_addr_get_u32(&sys_mgmt_info.static_gw));
    }

    if (ip_dns != ip4_addr_get_u32(&sys_mgmt_info.static_dns))
    {
        ip4_addr_set_u32(&sys_mgmt_info.static_dns, ip_dns);
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "set static_dns = 0x%x", ip4_addr_get_u32(&sys_mgmt_info.static_dns));
    }

    if (ip4_addr_cmp(&sys_mgmt_info.static_ip, &sys_mgmt_info.def_ip) &&
        ip4_addr_cmp(&sys_mgmt_info.static_mask, &sys_mgmt_info.def_mask) &&
        ip4_addr_cmp(&sys_mgmt_info.static_gw, &sys_mgmt_info.def_gw) &&
        ip4_addr_cmp(&sys_mgmt_info.static_dns, &sys_mgmt_info.def_dns))
    {
        if ((IPADDR_ANY == sys_mgmt_info.oper_ip) ||
            (IPADDR_ANY == sys_mgmt_info.oper_mask) ||
            (IPADDR_ANY == sys_mgmt_info.oper_gw) ||
            (IPADDR_ANY == sys_mgmt_info.oper_dns))
        {
            ip_change = TRUE;
            sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "Set to default IP");
        }
    }
    else/* Not equal to default */
    {
        ip_change = TRUE;
    }

    if (TRUE == ip_change)
    {
        sys_mgmt_netif_ip_set(FALSE);
    }

    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "return");

    return;
}

/* FUNCTION NAME: _sys_mgmt_queue_send
 * PURPOSE:
 *      package message and call sending function to DB.
 *
 * INPUT:
 *      ptr_msg     -- A pointer to the item to be tranmitted
 *      size        --  size of ptr_data
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_ENTRY_NOT_FOUND
 *      MW_E_BAD_PARAMETER
 *      MW_E_TIMEOUT
 *
 * NOTES:
 *      The input parameters are depend on structure of DB.
 *      Please refer to db_api.h
 */
MW_ERROR_NO_T
_sys_mgmt_queue_send(
    DB_MSG_T *ptr_msg,
    UI32_T size)
{
    MW_ERROR_NO_T rc;
    MW_CHECK_PTR(ptr_msg);
    rc = dbapi_dbisReady();
    if (MW_E_OK != rc)
    {
        /* This message could not be send, drop it */
        osapi_free(ptr_msg);
        return rc;
    }
    rc = dbapi_sendRequesttoDb(size, ptr_msg);
    if (MW_E_OK != rc)
    {
        /* This message could not be send, drop it */
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "db_sendRequesttoDb() failed");
        osapi_free(ptr_msg);
    }
    return rc;
}

/* FUNCTION NAME: sys_mgmt_queue_send
 * PURPOSE:
 *      package message and call sending function to DB.
 *
 * INPUT:
 *      method      --  the method bitmap
 *      t_idx       --  the enum of the table
 *      f_idx       --  the enum of the field
 *      e_idx       --  the entry index in the table
 *      ptr_data    --  pointer to message data
 *      size        --  size of ptr_data
 *      ptr_name    --  name of module
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_BAD_PARAMETER
 *      MW_E_OP_INCOMPLETE
 *      MW_E_NO_MEMORY
 *
 * NOTES:
 *      The input parameters are depend on structure of DB.
 *      Please refer to db_api.h
 */
MW_ERROR_NO_T
sys_mgmt_queue_send(
    const UI8_T method,
    const UI8_T t_idx,
    const UI8_T f_idx,
    const UI16_T e_idx,
    const void *ptr_data,
    const UI16_T size,
    const C8_T *ptr_name)
{
    MW_ERROR_NO_T   rc = MW_E_OK;
    DB_MSG_T        *ptr_msg = NULL;
    DB_PAYLOAD_T    *ptr_payload = NULL;
    UI32_T          msg_size;

    MW_PARAM_CHK((t_idx >= TABLES_LAST), MW_E_BAD_PARAMETER);
    msg_size = DB_MSG_HEADER_SIZE + DB_MSG_PAYLOAD_SIZE + size;
    rc = osapi_calloc(
            msg_size,
            SYS_MGMT_MODULE_NAME,
            (void **)&ptr_msg);
    if (MW_E_OK != rc)
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "allocate memory failed(%d)", rc);
        return MW_E_NO_MEMORY;
    }
    /* message */
    osapi_strncpy(ptr_msg->cq_name, ptr_name, DB_Q_NAME_SIZE);
    ptr_msg->method = method;
    ptr_msg->type.count = 1;
    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "ptr_msg=%p, cq_name=%s, method=0x%X, count=%u, size=%u",
                  ptr_msg, ptr_msg->cq_name, ptr_msg->method, ptr_msg->type.count, size);
    /* payload */
    ptr_payload = (DB_PAYLOAD_T *)&(ptr_msg->ptr_payload);
    ptr_payload->request.t_idx = t_idx;
    ptr_payload->request.f_idx = f_idx;
    ptr_payload->request.e_idx = e_idx;
    ptr_payload->data_size = size;
    if (size > 0 && method != M_GET)
    {
        memcpy(&(ptr_payload->ptr_data), ptr_data, size);
    }
    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "ptr_payload=%p, t_idx=%u, f_idx=%u, e_idx=%u, data_size=%u",
                  ptr_payload,
                  ptr_payload->request.t_idx,
                  ptr_payload->request.f_idx,
                  ptr_payload->request.e_idx,
                  ptr_payload->data_size);
    /* Send message to DB */
    rc = _sys_mgmt_queue_send(ptr_msg, msg_size);
    if (MW_E_OK != rc)
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "Send message to DB failed(%d)", rc);
        return MW_E_OP_INCOMPLETE;
    }

    return MW_E_OK;
}

static MW_ERROR_NO_T
_sys_mgmt_db_msg_process(
    const UI8_T method,
    const DB_REQUEST_TYPE_T *ptr_request,
    const UI16_T data_size,
    const void  *ptr_data)
{
    MW_ERROR_NO_T rc = MW_E_OK;
    ip_addr_t dns;

    if ((NULL == ptr_request) || (NULL == ptr_data) || (0 == data_size))
    {
        return MW_E_BAD_PARAMETER;
    }

    switch (method)
    {
    case M_GET:
    case M_UPDATE:
        switch (ptr_request->t_idx)
        {
        case SYS_INFO:
            switch (ptr_request->f_idx)
            {
            case SYS_DHCP_ENABLE:
                if ((MW_DHCP_ENABLE == ((UI8_T *)ptr_data)[0]) ||
                    (MW_DHCP_WEB_ENABLE == ((UI8_T *)ptr_data)[0]) ||
                    (MW_DHCP_DISABLE == ((UI8_T *)ptr_data)[0]))
                {
                    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "SYS_DHCP_ENABLE: %d", ((UI8_T *)ptr_data)[0]);
                    sys_mgmt_dhcp_set(((UI8_T *)ptr_data)[0]);
                }
                else if (MW_DHCP_DONE == ((UI8_T *)ptr_data)[0])
                {
                    /* Active dhcp address */
                    sys_mgmt_info.dhcp_enable = MW_DHCP_DONE;
                    sys_mgmt_netif_ip_set(TRUE);
                }
                else
                {
                    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "SYS_DHCP_ENABLE: Invalid parameter - %d", ((UI8_T *)ptr_data)[0]);
                }
                break;
            case SYS_AUTODNS_ENABLE:
                sys_mgmt_info.autodns_enable = ((UI8_T *)ptr_data)[0];
                if ((MW_DHCP_DONE == sys_mgmt_info.dhcp_enable) && (MW_AUTODNS_ENABLE == sys_mgmt_info.autodns_enable))
                {
                    /*For DHCP on and turn on the autoDNS,then set temp dns */
                    sys_mgmt_info.oper_dns = ip4_addr_get_u32(&(sys_mgmt_info.temp_dns));
                    ip_addr_set_ip4_u32_val(dns, sys_mgmt_info.oper_dns);
                }
                else if ((MW_DHCP_DISABLE != sys_mgmt_info.dhcp_enable) && (MW_AUTODNS_DISABLE == sys_mgmt_info.autodns_enable))
                {
                    /*For DHCP on and turn off the autoDNS,then set static dns */
                    sys_mgmt_info.oper_dns = ip4_addr_get_u32(&sys_mgmt_info.static_dns);
                    ip_addr_set_ip4_u32_val(dns, ip4_addr_get_u32(&sys_mgmt_info.static_dns));
                }
                else
                {
                    /*other case do nothing!,can't be deleted!*/
                    break;
                }
                dns_setserver(0, &dns);
                sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_DNS, DB_ALL_ENTRIES, &sys_mgmt_info.oper_dns, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
                break;
            case SYS_STATIC_IP_DNS:
                /* For DHCP ON and AutoDNS OFF that get DNS data by user seting from db. */
                if ((MW_DHCP_DISABLE != sys_mgmt_info.dhcp_enable) && (MW_AUTODNS_DISABLE == sys_mgmt_info.autodns_enable))
                {
                    memcpy(&sys_mgmt_info.oper_dns, ptr_data, sizeof(MW_IPV4_T));
                    ip4_addr_set_u32(&sys_mgmt_info.static_dns, sys_mgmt_info.oper_dns);
                    ip_addr_set_ip4_u32_val(dns, sys_mgmt_info.oper_dns);

                    dns_setserver(0, &dns);
                    sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_DNS, DB_ALL_ENTRIES, &sys_mgmt_info.oper_dns, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
                }
                break;
            case DB_ALL_FIELDS:
            {
                DB_SYS_INFO_T cfg_sys_info;
                memcpy(&cfg_sys_info, ptr_data, sizeof(DB_SYS_INFO_T));

                if ((!cfg_sys_info.static_ip) && (!cfg_sys_info.static_mask))
                {
                    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_INFO, "update static ip config to 0");
                    sys_mgmt_update_default_to_oper();
                }
                else
                {
                    sys_mgmt_ip_config_set(cfg_sys_info.static_ip, cfg_sys_info.static_mask, cfg_sys_info.static_gw, cfg_sys_info.static_dns);
                }
                if (MW_DHCP_ENABLE == cfg_sys_info.dhcp_enable)
                {
                    /* This could be happened after swtich power on */
                    sys_mgmt_info.autodns_enable = cfg_sys_info.autodns_enable;
                    sys_mgmt_dhcp_set(MW_DHCP_ENABLE);
                }
            }
            break;
            default:
                sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "recv unknown field: [%d]", ptr_request->f_idx);
                break;
            }
            break;
#ifdef AIR_SUPPORT_SNMP
        case PORT_OPER_INFO:
            switch (ptr_request->f_idx)
            {
            case PORT_OPER_STATUS:
            {
                UI8_T trap_type = ((((UI8_T *)ptr_data)[0] == 0) ? SNMP_GENTRAP_LINKDOWN : SNMP_GENTRAP_LINKUP);
                if ((SNMP_GENTRAP_LINKUP == trap_type) && (0 == snmp_linkup))
                {
                    snmp_linkup = 1;
                }
                if ((SNMP_GENTRAP_LINKUP == trap_type) && (1 == snmp_send_coldwarm_start))
                {
                    snmp_send_coldwarm_start = 0;
                    struct snmpcallback_msg_trap trap_info = {SNMP_GENTRAP_COLDSTART, ptr_request->e_idx};
                    int br = 0;
                    air_chipscu_getBootReason(0, &br);
                    if (1 == br)
                    {
                        trap_info.trap_type = SNMP_GENTRAP_WARMSTART;
                    }
                    tcpip_callback(snmp_trap_callback, &trap_info);
                }
                struct snmpcallback_msg_trap trap_info = {trap_type, ptr_request->e_idx};
                tcpip_callback(snmp_trap_callback, &trap_info);
                break;
            }
            default:
                break;
            }
            break;
        case LOGON_INFO:
            switch (ptr_request->f_idx)
            {
            case LOGON_FAIL_COUNT:
            {
                u8_t fail_count = ((UI8_T *)ptr_data)[0];
                if (0 != fail_count)
                {
                    struct snmpcallback_msg_trap trap_info = {SNMP_GENTRAP_AUTH_FAILURE, ptr_request->e_idx};
                    tcpip_callback(snmp_trap_callback, &trap_info);
                }
                break;
            }
            default:
                break;
            }
            break;
        case SNMP_INFO:
            switch (ptr_request->f_idx)
            {
            case SNMP_VERSION:
            {
                u8_t snmp_ver = ((UI8_T *)ptr_data)[0];
                if (0 != (snmp_ver & SNMP_V1_SUPPORT))
                {
                    snmp_v1_enable(SNMP_ENABLE);
                }
                else
                {
                    snmp_v1_enable(SNMP_DISABLE);
                }
                if (0 != (snmp_ver & SNMP_V2_SUPPORT))
                {
                    snmp_v2c_enable(SNMP_ENABLE);
                }
                else
                {
                    snmp_v2c_enable(SNMP_DISABLE);
                }
                break;
            }
            case SNMP_TRAP_EN:
            {
                if (0 == ((UI8_T *)ptr_data)[0])
                {
                    snmp_trap_dst_enable(0, SNMP_DISABLE);
                }
                else
                {
                    snmp_trap_dst_enable(0, SNMP_ENABLE);
                }
                break;
            }
            case SNMP_TRAP_TYPE:
            {
                if (0 != (((UI8_T *)ptr_data)[0] & SNMP_AUTHFAIL_SUPPORT))
                {
                    snmp_set_auth_traps_enabled(SNMP_ENABLE);
                }
                else
                {
                    snmp_set_auth_traps_enabled(SNMP_DISABLE);
                }
                break;
            }
            case SNMP_TRAP_DST_IP:
            {
                UI32_T dip = 0;
                osapi_memcpy(&dip, ptr_data, sizeof(dip));
                if (0 != dip)
                {
                    ip_addr_t dst;

                    ip_addr_set_ip4_u32_val(dst, dip);
                    snmp_trap_dst_ip_set(0, &dst);
                }
                break;
            }
#if LWIP_DNS
            case SNMP_TRAP_HOSTNAME:
            {
                u8_t *trapName = NULL;

                rc = osapi_calloc(MAX_HOST_NAME_SIZE, SYS_MGMT_MODULE_NAME, (void **)&trapName);
                ip_addr_t dst = {0};

                if (NULL != trapName)
                {
                    memcpy(trapName, ptr_data, data_size > MAX_HOST_NAME_SIZE - 1 ? MAX_HOST_NAME_SIZE - 1 : data_size);
                    trapName[MAX_HOST_NAME_SIZE - 1] = '\0';
                    if (0 != trapName[0])
                    {
                        netconn_gethostbyname(trapName, &dst);
                        snmp_trap_dst_ip_set(0, &dst);
                    }
                    osapi_free(trapName);
                }
                break;
            }
#endif
            case SNMP_READ_COMMUNITY:
            {
                u8_t *read_community = NULL;

                rc = osapi_calloc(MAX_SNMP_CM_LEN, SYS_MGMT_MODULE_NAME, (void **)&read_community);
                if (NULL != read_community)
                {
                    memcpy(read_community, ptr_data, data_size > MAX_SNMP_CM_LEN - 1 ? MAX_SNMP_CM_LEN - 1 : data_size);
                    read_community[MAX_SNMP_CM_LEN - 1] = 0;
                    snmp_set_community(read_community, osapi_strlen(read_community));
                    osapi_free(read_community);
                }
                break;
            }
            case SNMP_WRITE_COMMUNITY:
            {
                u8_t *write_community = NULL;

                rc = osapi_calloc(MAX_SNMP_CM_LEN, SYS_MGMT_MODULE_NAME, (void **)&write_community);
                if (NULL != write_community)
                {
                    memcpy(write_community, ptr_data, data_size > MAX_SNMP_CM_LEN - 1 ? MAX_SNMP_CM_LEN - 1 : data_size);
                    write_community[MAX_SNMP_CM_LEN - 1] = 0;
                    snmp_set_community_write(write_community, osapi_strlen(write_community));
                    osapi_free(write_community);
                }
                break;
            }
            case SNMP_TRAP_COMMUNITY:
            {
                u8_t *trap_community = NULL;

                rc = osapi_calloc(MAX_SNMP_CM_LEN, SYS_MGMT_MODULE_NAME, (void **)&trap_community);
                if (NULL != trap_community)
                {
                    memcpy(trap_community, ptr_data, data_size > MAX_SNMP_CM_LEN - 1 ? MAX_SNMP_CM_LEN - 1 : data_size);
                    trap_community[MAX_SNMP_CM_LEN - 1] = 0;
                    snmp_set_community_trap(trap_community, osapi_strlen(trap_community));
                    osapi_free(trap_community);
                }
                break;
            }
            default:
                break;
            }
            break;
        case SYS_OPER_INFO:
            switch (ptr_request->f_idx)
            {
            case SYS_OPER_IP_ADDR:
            {
                UI32_T sys_ip = 0;
                osapi_memcpy(&sys_ip, ptr_data, sizeof(sys_ip));
                if ((0 != sys_ip) && (1 == snmp_send_coldwarm_start))
                {
                    DB_MSG_T *ptrMsg = NULL;
                    UI16_T dataSize = 0;
                    UI8_T *ptrData = NULL;
                    if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_VERSION, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                    {
                        u8_t snmp_ver = *(u8_t *)ptrData;
                        if (0 != (snmp_ver & SNMP_V1_SUPPORT))
                        {
                            snmp_v1_enable(SNMP_ENABLE);
                        }
                        else
                        {
                            snmp_v1_enable(SNMP_DISABLE);
                        }
                        if (0 != (snmp_ver & SNMP_V2_SUPPORT))
                        {
                            snmp_v2c_enable(SNMP_ENABLE);
                        }
                        else
                        {
                            snmp_v2c_enable(SNMP_DISABLE);
                        }
                        MW_FREE(ptrMsg);
                    }
                    if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_TRAP_EN, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                    {
                        u8_t snmp_trap_en = *(u8_t *)ptrData;
                        if (0 == snmp_trap_en)
                        {
                            snmp_trap_dst_enable(0, SNMP_DISABLE);
                        }
                        else
                        {
                            snmp_trap_dst_enable(0, SNMP_ENABLE);
                        }
                        MW_FREE(ptrMsg);
                    }
                    if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_TRAP_TYPE, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                    {
                        u8_t snmp_trap_type = *(u8_t *)ptrData;
                        if (0 != (snmp_trap_type & SNMP_AUTHFAIL_SUPPORT))
                        {
                            snmp_set_auth_traps_enabled(SNMP_ENABLE);
                        }
                        else
                        {
                            snmp_set_auth_traps_enabled(SNMP_DISABLE);
                        }
                        MW_FREE(ptrMsg);
                    }
                    if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_TRAP_DST_IP, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                    {
                        ip_addr_t dst;

                        ip_addr_set_ip4_u32_val(dst, *(u32_t *)ptrData);
                        if (FALSE == ip_addr_isany_val(dst))
                        {
                            snmp_trap_dst_ip_set(0, &dst);
                        }
                        MW_FREE(ptrMsg);
                    }
                    if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_TRAP_DST_IP, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                    {
                        ip_addr_t dst;

                        ip_addr_set_ip4_u32_val(dst, *(u32_t *)ptrData);
                        MW_FREE(ptrMsg);
                        if (FALSE == ip_addr_isany_val(dst))
                        {
                            snmp_trap_dst_ip_set(0, &dst);
                        }
                        else
                        {
                            if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_TRAP_HOSTNAME, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                            {
                                u8_t *trapName = NULL;

                                rc = osapi_calloc(MAX_HOST_NAME_SIZE, SYS_MGMT_MODULE_NAME, (void **)&trapName);
                                if (NULL != trapName)
                                {
                                    memcpy(trapName, ptrData, dataSize > MAX_HOST_NAME_SIZE - 1 ? MAX_HOST_NAME_SIZE - 1 : dataSize);
                                    trapName[MAX_HOST_NAME_SIZE - 1] = 0;
#if LWIP_DNS
                                    /* get ip addr by hostname */
                                    if (0 != trapName[0])
                                    {
                                        netconn_gethostbyname(trapName, &dst);
                                        snmp_trap_dst_ip_set(0, &dst);
                                    }
                                    osapi_free(trapName);
#endif
                                }
                                MW_FREE(ptrMsg);
                            }
                        }
                    }
                    if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_READ_COMMUNITY, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                    {
                        u8_t *read_community = NULL;

                        rc = osapi_calloc(MAX_SNMP_CM_LEN, SYS_MGMT_MODULE_NAME, (void **)&read_community);
                        if (NULL != read_community)
                        {
                            memcpy(read_community, ptrData, dataSize > MAX_SNMP_CM_LEN - 1 ? MAX_SNMP_CM_LEN - 1 : dataSize);
                            read_community[MAX_SNMP_CM_LEN - 1] = 0;
                            snmp_set_community(read_community, osapi_strlen(read_community));
                            osapi_free(read_community);
                        }
                        MW_FREE(ptrMsg);
                    }
                    if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_WRITE_COMMUNITY, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                    {
                        u8_t *write_community = NULL;

                        rc = osapi_calloc(MAX_SNMP_CM_LEN, SYS_MGMT_MODULE_NAME, (void **)&write_community);
                        if (NULL != write_community)
                        {
                            memcpy(write_community, ptrData, dataSize > MAX_SNMP_CM_LEN - 1 ? MAX_SNMP_CM_LEN - 1 : dataSize);
                            write_community[MAX_SNMP_CM_LEN - 1] = 0;
                            snmp_set_community_write(write_community, osapi_strlen(write_community));
                            osapi_free(write_community);
                        }
                        MW_FREE(ptrMsg);
                    }
                    if (MW_E_OK == snmp_queue_getData(SNMP_INFO, SNMP_TRAP_COMMUNITY, DB_ALL_ENTRIES, &ptrMsg, &dataSize, (void **)&ptrData))
                    {
                        u8_t *trap_community = NULL;

                        rc = osapi_calloc(MAX_SNMP_CM_LEN, SYS_MGMT_MODULE_NAME, (void **)&trap_community);
                        if (NULL != trap_community)
                        {
                            memcpy(trap_community, ptrData, dataSize > MAX_SNMP_CM_LEN - 1 ? MAX_SNMP_CM_LEN - 1 : dataSize);
                            trap_community[MAX_SNMP_CM_LEN - 1] = 0;
                            snmp_set_community_trap(trap_community, osapi_strlen(trap_community));
                            osapi_free(trap_community);
                        }
                        MW_FREE(ptrMsg);
                    }
                    if (1 == snmp_linkup)
                    {
                        snmp_send_coldwarm_start = 0;
                        struct snmpcallback_msg_trap trap_info = {SNMP_GENTRAP_COLDSTART, ptr_request->e_idx};
                        int br = 0;
                        air_chipscu_getBootReason(0, &br);
                        if (1 == br)
                        {
                            trap_info.trap_type = SNMP_GENTRAP_WARMSTART;
                        }
                        tcpip_callback(snmp_trap_callback, &trap_info);
                    }
                }
                break;
            }
            default:
                break;
            }
            break;
#endif
#ifdef AIR_SUPPORT_ICMP_CLIENT
        case ICMP_CLIENT_INFO:
            _sys_mgmt_handle_db_ping_client(ptr_request, ptr_data);
            break;
#endif /* AIR_SUPPORT_ICMP_CLIENT */
#ifdef AIR_SUPPORT_MQTTD
        case MQTTD_CFG_INFO:
            if (ptr_request->f_idx == MQTTD_CFG_ENABLE)
            {
                if (TRUE == ((UI8_T *)ptr_data)[0])
                {
                    /* mqttd initialization */
                    osapi_printf("===db start mqttd init===\n");
                    mqttd_init(NULL);
                }
                else
                {
                    /* mqttd close */
                    mqttd_shutdown();
                }
            }
            break;
#endif
        default:
            break;
        }
        break;
    case M_RESPONSE:
        /*
         *
         */
        break;
    case M_ACK:
        /*
         *
         */
        break;
    default:
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "recv unknown method: [%02X]", method);
        break;
    }
    return rc;
}

static MW_ERROR_NO_T
_sys_mgmt_queue_recv_msg(
    const C8_T *ptr_name,
    UI8_T **pptr_msg,
    const UI32_T timeout)
{
    return osapi_msgRecv(SYS_MGMT_DB_QUEUE_NAME, pptr_msg, 0, SYS_MGMT_TASK_DELAY);
}

static void
_sys_mgmt_handle_db_msg(
    void)
{
    UI8_T i = 0;
    DB_MSG_T *ptr_msg = NULL;
    DB_REQUEST_TYPE_T request = {0};
    UI16_T data_size = 0;
    UI8_T *ptr_data = NULL;
    UI8_T *ptr_payload_data = NULL;
    MW_ERROR_NO_T rc = MW_E_OK;

    rc = _sys_mgmt_queue_recv_msg(SYS_MGMT_DB_QUEUE_NAME, (UI8_T**)&ptr_msg, SYS_MGMT_TASK_DELAY);
    if(MW_E_OK == rc)
    {
       /* Process the notification message */
        do {
            rc = dbapi_parseMsg(ptr_msg, ptr_msg->type.count, &request, &data_size, &ptr_data, &ptr_payload_data);
            if (MW_E_OK == rc)
            {
                sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "index=%u, ptr_payload=%p, t_idx=%u, f_idx=%u, e_idx=%u, data_size=%u",
                                                        i++,
                                                        ptr_data,
                                                        request.t_idx,
                                                        request.f_idx,
                                                        request.e_idx,
                                                        data_size);

                sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "[%d]recv method - %02X", ptr_msg->type.count, ptr_msg->method);
                rc = _sys_mgmt_db_msg_process(ptr_msg->method, &request, data_size, ptr_data);
                if (MW_E_OK != rc)
                {
                    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "handle_db_msg failed!(%d)", rc);
                }
            }
            /* Continue to parse the next request within the payload. */
        } while ((MW_E_OK == rc) && (NULL != ptr_payload_data));

        MW_FREE(ptr_msg);
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "MW_FREE(ptr_msg %p) Done.", ptr_msg);

    }
    else
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "Receive message queue failed!(%d)", rc);
    }
}


/* FUNCTION NAME:   sys_mgmt_task
 * PURPOSE:
 *      This DHCP task.
 *
 * INPUT:
 *      ptr_pvParameters
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
static void sys_mgmt_task( void *ptr_pvParameters )
{
    MW_ERROR_NO_T rc = MW_E_OK;
    ip_addr_t dns;

    /* Just to kill the compiler warning. */
    (void)ptr_pvParameters;
    memset(&sys_mgmt_info, 0, sizeof(SYS_MGMT_T));

    sys_mgmt_get_default_ip();

    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "Check DB is ready or not...");
    /* Check DB is ready or not */
    do{
        rc = dbapi_dbisReady();
    }while(MW_E_OK != rc);

    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "->sys_mgmt_queue_send(M_SUBSCRIBE, DB_ALL_FIELDS)");
    sys_mgmt_queue_send(M_SUBSCRIBE, SYS_INFO, DB_ALL_FIELDS, DB_ALL_ENTRIES, 0, 0, SYS_MGMT_DB_QUEUE_NAME);
#ifdef AIR_SUPPORT_SNMP
    sys_mgmt_queue_send(M_SUBSCRIBE, PORT_OPER_INFO, PORT_OPER_STATUS, DB_ALL_ENTRIES, 0, 0, SYS_MGMT_DB_QUEUE_NAME);
    sys_mgmt_queue_send(M_SUBSCRIBE, SNMP_INFO, DB_ALL_FIELDS, DB_ALL_ENTRIES, 0, 0, SYS_MGMT_DB_QUEUE_NAME);
    sys_mgmt_queue_send(M_SUBSCRIBE, SYS_OPER_INFO, SYS_OPER_IP_ADDR, DB_ALL_ENTRIES, 0, 0, SYS_MGMT_DB_QUEUE_NAME);
    sys_mgmt_queue_send(M_SUBSCRIBE, LOGON_INFO, LOGON_FAIL_COUNT, DB_ALL_ENTRIES, 0, 0, SYS_MGMT_DB_QUEUE_NAME);
#endif
#ifdef AIR_SUPPORT_ICMP_CLIENT
    sys_mgmt_queue_send(M_SUBSCRIBE, ICMP_CLIENT_INFO, STATUS, DB_ALL_ENTRIES, 0, 0, SYS_MGMT_DB_QUEUE_NAME);
#endif /* AIR_SUPPORT_ICMP_CLIENT */
#ifdef AIR_SUPPORT_MQTTD
    sys_mgmt_queue_send(M_SUBSCRIBE, MQTTD_CFG_INFO, MQTTD_CFG_ENABLE, DB_ALL_ENTRIES, 0, 0, SYS_MGMT_DB_QUEUE_NAME);
#endif /* AIR_SUPPORT_MQTTD */

    while (1)
    {
        /* Wait until something arrives in the queue - this task will block
        indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
        FreeRTOSConfig.h.  It will not use any CPU time while it is in the
        Blocked state. */
        _sys_mgmt_handle_db_msg();
    }
}

/* FUNCTION NAME:   sys_mgmt_get_default_ip
 * PURPOSE:
 *      sys_mgmt get default ip/mask/gw function.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
void sys_mgmt_get_default_ip(void)
{
    C8_T   ip_str[SYS_MGMT_IPV4_STR_SIZE];
    u8_t   netif_num = netif_num_get();
    struct netif *xNetIf = netif_get_by_index(netif_num);

    if (xNetIf != NULL)
    {
        const ip_addr_t *ptr_dns = dns_getserver(0);

        memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "Interface Name: %c%c", xNetIf->name[0], xNetIf->name[1]);
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip_addr_get_ip4_u32(&xNetIf->ip_addr)));
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "IP Address    : %s", ip_str);
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip_addr_get_ip4_u32(&xNetIf->netmask)));
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "Net Mask      : %s", ip_str);
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip_addr_get_ip4_u32(&xNetIf->gw)));
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "Gateway       : %s", ip_str);
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip_addr_get_ip4_u32(ptr_dns)));
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "DNS server       : %s", ip_str);

        ip4_addr_copy(sys_mgmt_info.def_ip, *ip_2_ip4(&xNetIf->ip_addr));
        ip4_addr_copy(sys_mgmt_info.def_mask, *ip_2_ip4(&xNetIf->netmask));
        ip4_addr_copy(sys_mgmt_info.def_gw, *ip_2_ip4(&xNetIf->gw));
        ip4_addr_copy(sys_mgmt_info.def_dns, *ip_2_ip4(ptr_dns));
    }

    return;
}

/* FUNCTION NAME:   sys_mgmt_update_default_to_oper
 * PURPOSE:
 *      sys_mgmt update default ip/mask/gw to oper function.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
void sys_mgmt_update_default_to_oper(void)
{
    if ((FALSE == ip4_addr_isany_val(sys_mgmt_info.def_ip)) && (FALSE == ip4_addr_isany_val(sys_mgmt_info.def_mask)))
    {
        sys_mgmt_ip_config_set(0, 0, 0, 0);
    }
    return;
}

/* FUNCTION NAME:   sys_mgmt_netif_ext_status_callback
 * PURPOSE:
 *      sys_mgmt callback function for netif status change.
 *      Ex: IP/Mask/GW is changed.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
#if LWIP_NETIF_EXT_STATUS_CALLBACK
static void
sys_mgmt_netif_ext_status_callback(struct netif *netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t *args)
{
    C8_T ip_str[SYS_MGMT_IPV4_STR_SIZE];
    MW_ERROR_NO_T ret = MW_E_OK;
    const netif_ext_callback_args_t *cb_args = args;
    MW_IPV4_T new_ip = 0, new_mask = 0, new_gw = 0;
    ip_addr_t new_dns;
    UI8_T autodns_state = 0;
    UI8_T state = 0;

    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_INFO, "reason: 0x%x", reason);
    ip_addr_set_zero_ip4(&new_dns);

    if (NULL == netif)
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "netif is NULL");
        return;
    }
    else if (reason & (LWIP_NSC_IPV4_ADDRESS_CHANGED | LWIP_NSC_IPV4_NETMASK_CHANGED | LWIP_NSC_IPV4_GATEWAY_CHANGED |
                       LWIP_NSC_IPV4_SETTINGS_CHANGED))
    {
        memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_INFO, "Interface Name: %c%c", netif->name[0], netif->name[1]);

        if((MW_DHCP_DONE == sys_mgmt_info.dhcp_enable) && (IPADDR_ANY == sys_mgmt_info.oper_ip))
        {
            /* This case should occur when dhcp release ip address */
            sys_mgmt_info.dhcp_enable = MW_DHCP_ENABLE;
        }
        if((MW_DHCP_ENABLE == sys_mgmt_info.dhcp_enable) || (MW_DHCP_WEB_ENABLE == sys_mgmt_info.dhcp_enable))
        {
            ip4_addr_set_any(&sys_mgmt_info.temp_dns);
            if (NULL != cb_args)
            {
                /* Use struct old ip info to carry dhcp ip address */
                if (NULL != cb_args->ipv4_changed.old_address)
                {
                    new_ip = ip_addr_get_ip4_u32(cb_args->ipv4_changed.old_address);
                }
                if (NULL != cb_args->ipv4_changed.old_netmask)
                {
                    new_mask = ip_addr_get_ip4_u32(cb_args->ipv4_changed.old_netmask);
                }
                if (NULL != cb_args->ipv4_changed.old_gw)
                {
                    new_gw = ip_addr_get_ip4_u32(cb_args->ipv4_changed.old_gw);
                }
                if (NULL != cb_args->ipv4_changed.old_dns)
                {
                    ip4_addr_copy(sys_mgmt_info.temp_dns, *ip_2_ip4(cb_args->ipv4_changed.old_dns));
                }
            }

            if (MW_AUTODNS_ENABLE == sys_mgmt_info.autodns_enable)
            {
                if (FALSE == ip4_addr_isany_val(sys_mgmt_info.temp_dns))
                {
                    ip_addr_set_ip4_u32_val(new_dns, ip4_addr_get_u32(&(sys_mgmt_info.temp_dns)));
                }
                else if (FALSE == ip4_addr_isany_val(sys_mgmt_info.static_dns))
                {
                    ip_addr_set_ip4_u32_val(new_dns, ip4_addr_get_u32(&sys_mgmt_info.static_dns));
                }
                else
                {
                    ip_addr_set_ip4_u32_val(new_dns, ip4_addr_get_u32(&sys_mgmt_info.def_dns));
                }
            }
            else
            {
                const ip_addr_t *ptr_dns = dns_getserver(0);
                ip_addr_copy(new_dns, *ptr_dns);
            }
        }
        else
        {
            ip_addr_t *ptr_dns = (ip_addr_t *)dns_getserver(0);

            ip_addr_copy(new_dns, *ptr_dns);
            new_ip = ip_addr_get_ip4_u32(&netif->ip_addr);
            new_mask = ip_addr_get_ip4_u32(&netif->netmask);
            new_gw = ip_addr_get_ip4_u32(&netif->gw);

            if ((NULL != cb_args) && (LWIP_NSC_IPV4_SETTINGS_CHANGED == reason) && (MW_AUTODNS_ENABLE == sys_mgmt_info.autodns_enable))
            {
                /* This is for DHCP renew case with AUTO_DNS enabled. */
                ptr_dns = cb_args->ipv4_changed.old_dns;
                if (NULL != ptr_dns)
                {
                    ip_addr_copy(new_dns, *ptr_dns);
                    dns_setserver(0, &new_dns);
                }
            }
        }

        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(new_ip));
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_INFO, "IP Address    : %s\n", ip_str);
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(new_mask));
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_INFO, "Net Mask      : %s\n", ip_str);
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(new_gw));
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_INFO, "Gateway       : %s\n", ip_str);
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip_addr_get_ip4_u32(&new_dns)));
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_INFO, "DNS           : %s\n", ip_str);

        if((sys_mgmt_info.oper_ip != new_ip) ||
           (sys_mgmt_info.oper_mask != new_mask) ||
           (sys_mgmt_info.oper_gw != new_gw) ||
           (sys_mgmt_info.oper_dns != ip_addr_get_ip4_u32(&new_dns)))
        {
            sys_mgmt_info.oper_ip = new_ip;
            sys_mgmt_info.oper_mask = new_mask;
            sys_mgmt_info.oper_gw = new_gw;
            sys_mgmt_info.oper_dns = ip_addr_get_ip4_u32(&new_dns);
            sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "->sys_mgmt_queue_send(M_UPDATE, SYS_OPER_IP_ADDR)");
            ret = sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_ADDR, DB_ALL_ENTRIES, &sys_mgmt_info.oper_ip, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
            if(MW_E_OK != ret)
            {
                sys_mgmt_info.oper_ip = IPADDR_ANY;
            }
            sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "->sys_mgmt_queue_send(M_UPDATE, SYS_OPER_IP_MASK)");
            ret = sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_MASK, DB_ALL_ENTRIES, &sys_mgmt_info.oper_mask, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
            if(MW_E_OK != ret)
            {
                sys_mgmt_info.oper_mask = IPADDR_ANY;
            }
            sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "->sys_mgmt_queue_send(M_UPDATE, SYS_OPER_IP_GW)");
            ret = sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_GW, DB_ALL_ENTRIES, &sys_mgmt_info.oper_gw, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
            if(MW_E_OK != ret)
            {
                sys_mgmt_info.oper_gw = IPADDR_ANY;
            }
            sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "->sys_mgmt_queue_send(M_UPDATE, SYS_OPER_IP_DNS)");
            ret = sys_mgmt_queue_send(M_UPDATE, SYS_OPER_INFO, SYS_OPER_IP_DNS, DB_ALL_ENTRIES, &sys_mgmt_info.oper_dns, sizeof(MW_IPV4_T), SYS_MGMT_DB_QUEUE_NAME);
            if(MW_E_OK != ret)
            {
                sys_mgmt_info.oper_dns = IPADDR_ANY;
            }
            if(MW_DHCP_ENABLE == sys_mgmt_info.dhcp_enable)
            {
                sys_mgmt_info.dhcp_enable = MW_DHCP_DONE;
                ret = sys_mgmt_queue_send(M_UPDATE, SYS_INFO, SYS_DHCP_ENABLE, DB_ALL_ENTRIES, &sys_mgmt_info.dhcp_enable, sizeof(UI8_T), SYS_MGMT_DB_QUEUE_NAME);
                if(MW_E_OK != ret)
                {
                    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "->sys_mgmt_queue_send(M_UPDATE, SYS_DHCP_ENABLE) fail.");
                    sys_mgmt_info.dhcp_enable = MW_DHCP_ENABLE;
                }
                sys_mgmt_netif_ip_set(TRUE);
            }
        }
    }
    else if (reason & LWIP_NSC_IPV4_DHCP_FAIL)
    {
        /* DHCP timeout, disable dhcp and set ip to default ip address */
        autodns_state = MW_AUTODNS_DISABLE;
        state = MW_DHCP_DISABLE;
        sys_mgmt_queue_send(M_UPDATE, SYS_INFO, SYS_AUTODNS_ENABLE, DB_ALL_ENTRIES, &autodns_state, sizeof(UI8_T), SYS_MGMT_DB_QUEUE_NAME);
        sys_mgmt_queue_send(M_UPDATE, SYS_INFO, SYS_DHCP_ENABLE, DB_ALL_ENTRIES, &state, sizeof(UI8_T), SYS_MGMT_DB_QUEUE_NAME);
        sys_mgmt_dhcp_set(state);
    }

    return;
}
#endif

/* FUNCTION NAME:   sys_mgmt_free_resource
 * PURPOSE:
 *      Free the resources in sys_mgmt init function.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T sys_mgmt_free_resource(void)
{
    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "");

    if (osapi_msgDelete(SYS_MGMT_DB_QUEUE_NAME) != MW_E_OK)
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "osapi_msgDelete for %s failed !", SYS_MGMT_DB_QUEUE_NAME);
    }

    osapi_threadDelete(sys_mgmt_task_handle);

    return MW_E_OK;
}

/* FUNCTION NAME:   sys_mgmt_init
 * PURPOSE:
 *      This sys_mgmt init function.
 *
 * INPUT:
 *      pvParameters
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_NO_MEMORY
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T sys_mgmt_init(void)
{
    memset(&sys_mgmt_info, 0, sizeof(SYS_MGMT_T));

    /* Create DB client socket */
    if (osapi_msgCreate(SYS_MGMT_DB_QUEUE_NAME, SYS_MGMT_QUEUE_LENGTH, sizeof(void *)) != MW_E_OK)
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "osapi_msgCreate %s fail", SYS_MGMT_DB_QUEUE_NAME);
        return MW_E_NO_MEMORY;
    }

    if (osapi_threadCreate(SYS_MGMT_TASK_NAME,
                       configMINIMAL_STACK_SIZE*2,
                       MW_TASK_PRIORITY_SYSMGMT,
                       sys_mgmt_task,
                       NULL,
                       &sys_mgmt_task_handle) != MW_E_OK)
    {
        sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_ERROR, "osapi_threadCreate for SYS_MGMT failed !");
        sys_mgmt_free_resource();
        return MW_E_NO_MEMORY;
    }

#if LWIP_NETIF_EXT_STATUS_CALLBACK
    /* register for netif events when started on first netif */
    netif_add_ext_callback(&sys_mgmt_netif_callback, sys_mgmt_netif_ext_status_callback);
#endif
    /* Initialize system language */
    sys_mgmt_language_init();
    return MW_E_OK;
}


/* FUNCTION NAME:   sys_mgmt_debug_level_set
 * PURPOSE:
 *      This API is used to set sys_mgmt debug print level.
 *
 * INPUT:
 *      enable       --  fast-leave mode
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
void sys_mgmt_debug_level_set(UI8_T level)
{
    sys_mgmt_debug_level = level;

    return;
}

/* FUNCTION NAME:   sys_mgmt_dump
 * PURPOSE:
 *      This API is used to dump SYS_MGMT group and mrouter entry.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
void sys_mgmt_dump()
{
    C8_T   ip_str[SYS_MGMT_IPV4_STR_SIZE];

    osapi_printf("\nSYS_MGMT:\n");

    osapi_printf("\tDHCP Mode: %s\n", sys_mgmt_info.dhcp_enable ? "Enable" : "Disable");

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.static_ip)));
    osapi_printf("\tStatic IP: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.static_mask)));
    osapi_printf("\tStatic MASK: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.static_gw)));
    osapi_printf("\tStatic GATEWAY: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.static_dns)));
    osapi_printf("\tStatic DNS: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.def_ip)));
    osapi_printf("\n\tDefault IP: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.def_mask)));
    osapi_printf("\tDefault MASK: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.def_gw)));
    osapi_printf("\tDefault GATEWAY: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.def_dns)));
    osapi_printf("\tDefault DNS: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    if(IPADDR_ANY == sys_mgmt_info.oper_ip)
    {
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.static_ip)));
    }
    else
    {
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(sys_mgmt_info.oper_ip));
    }
    osapi_printf("\n\tOper IP: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    if(IPADDR_ANY == sys_mgmt_info.oper_mask)
    {
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.static_mask)));
    }
    else
    {
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(sys_mgmt_info.oper_mask));
    }
    osapi_printf("\tOper MASK: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    if(IPADDR_ANY == sys_mgmt_info.oper_gw)
    {
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.static_gw)));
    }
    else
    {
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(sys_mgmt_info.oper_gw));
    }
    osapi_printf("\tOper GATEWAY: %s\n", ip_str);

    memset(ip_str, 0, SYS_MGMT_IPV4_STR_SIZE);
    if(IPADDR_ANY == sys_mgmt_info.oper_dns)
    {
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(ip4_addr_get_u32(&sys_mgmt_info.static_dns)));
    }
    else
    {
        MW_UTIL_IPV4_TO_STR(ip_str, PP_HTONL(sys_mgmt_info.oper_dns));
    }
    osapi_printf("\n\tOper DNS: %s\n\n", ip_str);

    osapi_printf("\tDebug Level: %d\n", sys_mgmt_debug_level);
#ifndef AIR_SUPPORT_DHCP_SNOOP
    printf("\tDHCP ACL rule entry-id: %d\n\n", dhcp_acl_id);
#endif /* AIR_SUPPORT_DHCP_SNOOP */

    return;
}

/* FUNCTION NAME:   sys_mgmt_dhcp_enable_cmd_set
 * PURPOSE:
 *      This API is used for mw cmd to enable/disable SYS MGMT DHCP admin mode.
 *
 * INPUT:
 *      enable       --  dhcp mode
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
void sys_mgmt_dhcp_enable_cmd_set(UI8_T enable)
{
    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "->sys_mgmt_queue_send(M_UPDATE, SYS_INFO, SYS_DHCP_ENABLE, DB_ALL_ENTRIES, enable=%d)", enable);
    sys_mgmt_queue_send(M_UPDATE, SYS_INFO, SYS_DHCP_ENABLE, DB_ALL_ENTRIES, &enable, sizeof(UI8_T), SYS_MGMT_DB_QUEUE_NAME);
    sys_mgmt_dhcp_set(enable);

    return;
}

/* FUNCTION NAME:   sys_mgmt_autodns_enable_cmd_set
 * PURPOSE:
 *      This API is used for mw cmd to enable/disable SYS MGMT Auto DNS admin mode.
 *
 * INPUT:
 *      enable       --  Auto DNS mode
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
void sys_mgmt_autodns_enable_cmd_set(UI8_T enable)
{
    sys_mgmt_info.autodns_enable = enable;
    return;
}
#ifdef AIR_SUPPORT_ICMP_CLIENT
/* FUNCTION NAME:   _sys_mgmt_handle_db_ping_client()
 * PURPOSE:
 *      This API is used for sys_mgmt task handle db notify about ping client.
 *
 * INPUT:
 *      ptr_pload       --  db payload
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_BAD_PARAMETER
 *
 * NOTES:
 *      None
 */
static MW_ERROR_NO_T
_sys_mgmt_handle_db_ping_client(
    const DB_REQUEST_TYPE_T *ptr_request,
    const void  *ptr_data)
{
    MW_ERROR_NO_T ret = MW_E_OK;
    if((NULL == ptr_data) || (NULL == ptr_request))
    {
        return MW_E_BAD_PARAMETER;
    }
    if(ptr_request->f_idx == STATUS)
    {
        UI16_T status;
        osapi_memcpy(&status, ptr_data, sizeof(UI16_T));
        ping_debug(PING_DEBUG_LEVEL,"status: %d",status);
#if LWIP_RAW
        ping_set_db_ping_status(status);
        ret = MW_E_NOT_SUPPORT;
        if(status == AIR_ICMP_CLIENT_ERR_START)
        {
            ret = ping_create_ping_thread();
            if(MW_E_OK == ret)
            {
                ping_debug(PING_DEBUG_LEVEL,"ping thread crete success!");
            }
            else
            {
                ping_debug(PING_DEBUG_LEVEL,"ping thread crete failed, ret: %d", ret);
                status = AIR_ICMP_CLIENT_ERR_STOPPED;
                sys_mgmt_queue_send(M_UPDATE, ICMP_CLIENT_INFO, STATUS, DB_ALL_ENTRIES, &status,sizeof(UI16_T), SYS_MGMT_DB_QUEUE_NAME);
                ping_set_db_ping_status(status);
            }
        }
#else  /* LWIP_RAW */
        status = AIR_ICMP_CLIENT_ERR_STOPPED;
        sys_mgmt_queue_send(M_UPDATE, ICMP_CLIENT_INFO, STATUS, DB_ALL_ENTRIES, &status,sizeof(UI16_T), SYS_MGMT_DB_QUEUE_NAME);
#endif /* LWIP_RAW */
    }
    return ret;
}
#endif /* AIR_SUPPORT_ICMP_CLIENT */

#ifdef AIR_SUPPORT_MQTTD
/* FUNCTION NAME:   sys_mgmt_mqttd_enable_cmd_set
 * PURPOSE:
 *      This API is used for mw cmd to enable/disable MQTTD admin mode.
 *
 * INPUT:
 *      enable       --  mqttd administrative mode
 *      server_ip    --  remote cloud server
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *
 * NOTES:
 *      None
 */
void sys_mgmt_mqttd_enable_cmd_set(UI8_T enable, void *server_ip)
{
    sys_mgmt_debug(SYS_MGMT_DEBUG_LEVEL_DEBUG, "->sys_mgmt_queue_send(M_UPDATE, MQTTD, MQTTD_CFG_ENABLE, DB_ALL_ENTRIES, enable=%d)", enable);
    sys_mgmt_queue_send(M_UPDATE, MQTTD_CFG_INFO, MQTTD_CFG_ENABLE, DB_ALL_ENTRIES, &enable, sizeof(UI8_T), SYS_MGMT_DB_QUEUE_NAME);
    if (TRUE == enable)
    {
        /* mqttd initialization */
        osapi_printf("===cmd start mqttd init===\n");
        mqttd_init(server_ip);
    }
    else
    {
        /* mqttd close */
        mqttd_shutdown();
    }
}


void sys_mgmt_mqttd_enable_coding(UI8_T enable)
{
    mqttd_coding_enable(enable);
}

void sys_mgmt_mqttd_enable_json(UI8_T enable)
{
    mqttd_json_dump_enable(enable);
}

#endif

/* FUNCTION NAME: mw_dos_setGlobalCfg
 * PURPOSE:
 *      Sets the global configuration for the DoS module.
 *
 * INPUT:
 *      unit                 -- Device unit number
 *      enable               -- Enable/disable DoS module
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OTHERS
 *      MW_E_OK
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T
mw_dos_setGlobalCfg(
    UI32_T     unit,
    BOOL_T     enable
)
{
    BOOL_T              dos_global_state = FALSE;
    AIR_ERROR_NO_T      rc = AIR_E_OK;

    if(TRUE == enable)
    {
        air_dos_getGlobalCfg(unit, &dos_global_state);
        if(FALSE == dos_global_state)
        {
            rc = air_dos_setGlobalCfg(unit,TRUE);
        }
        if(AIR_E_OK == rc)
        {
            _mw_attack_prevention_global_state_ref_cnt ++;
        }
    }
    else
    {
        if(1 == _mw_attack_prevention_global_state_ref_cnt)
        {
            rc = air_dos_setGlobalCfg(unit, FALSE);
            if(AIR_E_OK == rc)
            {
                _mw_attack_prevention_global_state_ref_cnt --;
            }
        }
        else if(1 < _mw_attack_prevention_global_state_ref_cnt)
        {
            _mw_attack_prevention_global_state_ref_cnt --;
            rc = AIR_E_OK;
        }
        else
        {
            rc = AIR_E_OTHERS;
        }
    }

    return ((AIR_E_OK == rc) ? MW_E_OK : MW_E_OTHERS);
}

/* FUNCTION NAME: sys_mgmt_language_init()
 * PURPOSE:
 *      Initialize system language.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T
sys_mgmt_language_init(void)
{
    UI32_T          tlv_type_addr = 0;
    UI8_T           lang_idx = 0;

    if(MW_E_OK == mw_is_tlv_data_exist(MW_TLV_TYPE_LANGUAGE, &tlv_type_addr))
    {
        if(MW_E_OK == mw_read_tlv_data(sizeof(UI8_T), (tlv_type_addr + TLV_DATA_HEADER_SIZE), (void *)&lang_idx))
        {
            if((0 <= lang_idx) && (LANG_LAST > lang_idx))
            {
                language_info.lang_idx = lang_idx;
            }
        }
    }
    return MW_E_OK;
}

