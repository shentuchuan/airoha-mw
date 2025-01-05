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

/* FILE NAME:   mw_init.c
 * PURPOSE:
 *      Middleware initialization function call.
 * NOTES:
 */

/* INCLUDE FILE DECLARATIONS
 */
#include <mw_init.h>
#include <syncd.h>
#include <mac_init.h>
#include <air_l2.h>
#include <air_swc.h>
#include <air_init.h>
#include <mw_platform.h>
#include <mw_portbmp.h>
#include <air_ver.h>
#ifdef AIR_SUPPORT_IPV6
#include "lwip/netif.h"
#include "lwip/ip6_addr.h"
#endif /* AIR_SUPPORT_IPV6 */
#ifdef AIR_SUPPORT_POE
#include <air_cfg.h>
#endif
#include "air_dos.h"
#include "air_sec.h"
#include "mqttd_http.h"

/* NAMING CONSTANT DECLARATIONS
 */
#define MW_VERSION_PLATFORM "EN8851, "
#define MW_VERSION_MW "MW " MW_VER_STR ", "
#define MW_VERSION_SDK "SDK " AIR_VER_SDK ", "
#define MW_VERSION_FREERTOS "FreeRTOS Kernel V10.1.1"
#define SA_ZERO (1)
#define DA_ZERO (2)
#define ALLZERO_MAC_UDF_NUM (3)
#define BCAST_MAC_UDF_NUM (3)

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* GLOBAL VARIABLE DECLARATIONS
 */
unsigned int _mw_total_port = MW_DEFAULT_MAX_PORT_NUM;
unsigned int _mw_eth_port = MW_DEFAULT_MAX_PORT_NUM;
unsigned int _mw_cpu_port = MW_DEFAULT_CPU_PORT;
#ifdef AIR_SUPPORT_POE
unsigned int _mw_poe_port = MW_DEFAULT_MAX_PORT_NUM;
#endif
AIR_PORT_BITMAP_T _mw_port_capacity;
AIR_PORT_BITMAP_T _mw_port_bmp_total;
unsigned char _mw_mac_addr[MAC_ADDRESS_LEN] = {0};
char _mw_sys_desc[MW_SYS_DESCRI_LEN_MAX] = {0};
#ifdef AIR_SUPPORT_IPV6
static UI8_T _mw_multicast_mac_addr[][MAC_ADDRESS_LEN] = {
    {0x33, 0x33, 0, 0, 0, 0x01}, /* MAC address for all nodes addresses */
    {0x33, 0x33, 0xFF, 0, 0, 0}, /* Incomplete MAC address for solicited-node addresses */
};
#endif /* AIR_SUPPORT_IPV6*/
UI16_T _allzero_mac_acl_udf_id[ALLZERO_MAC_UDF_NUM];
UI16_T _broadcast_mac_udf_id[BCAST_MAC_UDF_NUM] = {MW_ACL_ID_INVALID, MW_ACL_ID_INVALID, MW_ACL_ID_INVALID};

C8_T *const unknown_acl_name = "UNKNOWN";

/* LOCAL SUBPROGRAM DECLARATIONS
 */
MW_ERROR_NO_T
_system_cpu_port_init(
    void);

static MW_ERROR_NO_T
_system_acl_init(void)
{
    UI32_T unit = 0;
    UI32_T port = 0;
    AIR_ERROR_NO_T rc;

    mw_acl_mutex_init();
    AIR_PORT_FOREACH(PLAT_PORT_BMP_TOTAL, port)
    {
        if (PLAT_CPU_PORT == port)
        {
            /* Skip cpu port */
            continue;
        }
        rc = air_acl_setPortState(unit, port, ENABLE);
        if (rc != AIR_E_OK)
        {
            osapi_printf("Enable ACL port %d config failed, rc=%d.\n", port, rc);
        }
    }
    rc = air_acl_setGlobalState(unit, ENABLE);
    if (rc != AIR_E_OK)
    {
        osapi_printf("Enable ACL global config failed, rc=%d.\n", rc);
    }
    return MW_E_OK;
}

static MW_ERROR_NO_T
_system_acl_add(
    C8_T *ptr_acl_name,
    UI32_T rule_id,
    AIR_ACL_RULE_T *ptr_acl_rule,
    AIR_ACL_ACTION_T *ptr_acl_action)
{
    const UI32_T unit = 0;
    UI32_T acl_rule_id = rule_id;
    AIR_ACL_RULE_T cur_acl_rule;
    AIR_ERROR_NO_T rc;

    if ((ptr_acl_rule == NULL) || (ptr_acl_action == NULL))
    {
        return AIR_E_BAD_PARAMETER;
    }

    if (ptr_acl_name == NULL)
    {
        ptr_acl_name = unknown_acl_name;
    }

    if (FALSE == MW_ACL_ID_IS_STATIC(acl_rule_id))
    {
        for (; acl_rule_id <= MW_ACL_ID_DYNAMIC_MAX; acl_rule_id++)
        {
            if (air_acl_getRule(unit, acl_rule_id, &cur_acl_rule) == AIR_E_OK)
            {
                if (FALSE == cur_acl_rule.rule_en)
                {
                    break;
                }
            }
        }

        if (acl_rule_id > MW_ACL_ID_DYNAMIC_MAX)
        {
            osapi_printf("[MAC INIT] Get free ACL rule-id for %s failed !\n", ptr_acl_name);
            return MW_E_OP_INCOMPLETE;
        }
    }

    rc = air_acl_setRule(unit, acl_rule_id, ptr_acl_rule);
    if (rc != AIR_E_OK)
    {
        osapi_printf("[MAC INIT] Add %s ACL rule entry-id %d failed, rc=%d.\n", ptr_acl_name, acl_rule_id, rc);
        return MW_E_OP_INCOMPLETE;
    }

    if (ptr_acl_action->field_valid & BIT(AIR_ACL_MIB))
    {
        ptr_acl_action->acl_mib_id = acl_rule_id;
    }

    rc = air_acl_setAction(unit, acl_rule_id, ptr_acl_action);
    if (rc != AIR_E_OK)
    {
        osapi_printf("[MAC INIT] Add %s ACL rule entry-id %d action fail, rc=%d.\n", ptr_acl_name, acl_rule_id, rc);
        air_acl_delRule(unit, acl_rule_id);
        return MW_E_OP_INCOMPLETE;
    }
    return MW_E_OK;
}

static MW_ERROR_NO_T
_system_udf_add(
    C8_T *ptr_acl_udf_name,
    UI32_T *ptr_acl_udf_id,
    AIR_ACL_UDF_RULE_T *ptr_acl_udf_rule)
{
    UI32_T acl_udf_id = 0;
    const UI32_T unit = 0;
    AIR_ERROR_NO_T rc;
    AIR_ACL_UDF_RULE_T acl_udf_rule;

    if ((ptr_acl_udf_id == NULL) || (ptr_acl_udf_rule == NULL))
    {
        return AIR_E_BAD_PARAMETER;
    }

    if (ptr_acl_udf_name == NULL)
    {
        ptr_acl_udf_name = unknown_acl_name;
    }

    for (; acl_udf_id < AIR_MAX_NUM_OF_UDF_ENTRY; acl_udf_id++)
    {
        if (air_acl_getUdfRule(unit, acl_udf_id, &acl_udf_rule) == AIR_E_OK)
        {
            if (FALSE == acl_udf_rule.udf_rule_en)
            {
                break;
            }
        }
    }
    if (acl_udf_id == AIR_MAX_NUM_OF_UDF_ENTRY)
    {
        osapi_printf("%s udf get free ACL udf-id failed !\n", ptr_acl_udf_name);
        return MW_E_ENTRY_REACH_END;
    }

    rc = air_acl_setUdfRule(unit, acl_udf_id, ptr_acl_udf_rule);
    if (rc != AIR_E_OK)
    {
        osapi_printf("Add %s udf-id %d failed, rc=%d !\n", ptr_acl_udf_name, acl_udf_id, rc);
        return MW_E_OP_INCOMPLETE;
    }
    *ptr_acl_udf_id = acl_udf_id;
    return MW_E_OK;
}

static MW_ERROR_NO_T
_system_arp_acl(void)
{
    const UI8_T udf_wofs_base = 0;
    UI8_T index;
    UI8_T acl_ap_id = MW_ACL_ID_INVALID;
    UI32_T unit = 0;
    UI32_T acl_udf_id = 0;
    AIR_ACL_UDF_RULE_T acl_udf_rule = {};
    AIR_ACL_RULE_T acl_rule = {};
    AIR_ACL_ACTION_T acl_action = {};
    AIR_DOS_RATE_LIMIT_CFG_T dos_rate_limit = {0};

    /* Broadcast MAC UDF */
    acl_udf_rule.udf_rule_en = TRUE;
    AIR_PORT_BITMAP_COPY(acl_udf_rule.portmap, PLAT_PORT_BMP_TOTAL);
    AIR_PORT_DEL(acl_udf_rule.portmap, PLAT_CPU_PORT);
    acl_udf_rule.udf_pkt_type = AIR_ACL_MAC_HEADER;
    acl_udf_rule.word_ofst = 0;     /* offset */
    acl_udf_rule.cmp_sel = 0;       /* pattern */
    acl_udf_rule.cmp_pat = 0xFFFF;  /* CMP pattern */
    acl_udf_rule.cmp_mask = 0xFFFF; /* CMP mask */

    for (index = 0; index < BCAST_MAC_UDF_NUM; ++index)
    {
        acl_udf_id = MW_ACL_ID_INVALID;
        acl_udf_rule.word_ofst = udf_wofs_base + index;
        _system_udf_add("Broadcast MAC", &acl_udf_id, &acl_udf_rule);
        _broadcast_mac_udf_id[index] = acl_udf_id;
    }

    mw_dos_setGlobalCfg(unit, TRUE);
    for (acl_ap_id = 0; acl_ap_id < AIR_MAX_NUM_OF_ATTACK_PREVENTION_ID; acl_ap_id++)
    {
        if (AIR_E_OK == air_dos_getRateLimitCfg(unit, acl_ap_id, &dos_rate_limit))
        {
            if (TRUE == ((0 == dos_rate_limit.pkt_thld) &&
                         (0 == dos_rate_limit.time_span) &&
                         (0 == dos_rate_limit.block_time)))
            {
                dos_rate_limit.pkt_thld = 4;
                dos_rate_limit.time_span = 100;
                dos_rate_limit.block_time = 1;
                dos_rate_limit.tick_sel = AIR_DOS_RATE_TICKSEL_1MS;
                air_dos_setRateLimitCfg(unit, acl_ap_id, &dos_rate_limit);
                break;
            }
        }
    }

    /* ARP ACL */
    acl_rule.rule_en = TRUE;
    acl_rule.end = TRUE;
    AIR_PORT_BITMAP_COPY(acl_rule.portmap, PLAT_PORT_BMP_TOTAL);
    AIR_PORT_DEL(acl_rule.portmap, PLAT_CPU_PORT);
    memcpy(acl_rule.key.dmac, _mw_mac_addr, MAC_ADDRESS_LEN);
    acl_rule.mask.dmac = 0x3f;
    acl_rule.key.etype = ETHTYPE_ARP;
    acl_rule.mask.etype = 0x3;
    for (index = 0; index < BCAST_MAC_UDF_NUM; ++index)
    {
        if (_broadcast_mac_udf_id[index] == MW_ACL_ID_INVALID)
        {
            return MW_E_BAD_PARAMETER;
        }
        acl_rule.key.udf |= BIT(_broadcast_mac_udf_id[index]);
    }
    acl_rule.mask.udf = acl_rule.key.udf;
    acl_rule.field_valid = BIT(AIR_ACL_DMAC_KEY) | BIT(AIR_ACL_ETYPE_KEY);
    acl_action.port_fw = MW_ACL_ACT_PORT_FW_CPU_ONLY;
    acl_action.pri_user = MW_ACL_RX_PRIORITY_ARP;
    acl_action.field_valid = BIT(AIR_ACL_MIB) | BIT(AIR_ACL_FW_PORT) | BIT(AIR_ACL_PRI);
    if (AIR_MAX_NUM_OF_ATTACK_PREVENTION_ID != acl_ap_id)
    {
        acl_action.attack_rate_id = acl_ap_id;
        acl_action.field_valid |= (1U << AIR_ACL_ATTACK);
    }
    _system_acl_add("System Unicast ARP", MW_ACL_ID_STATIC_UNICAST_ARP, &acl_rule, &acl_action);

    acl_rule.field_valid = BIT(AIR_ACL_ETYPE_KEY) | BIT(AIR_ACL_UDF_KEY);
    acl_action.port_fw = MW_ACL_ACT_PORT_FW_CPU_INCLUDE;

    _system_acl_add("System Broadcast ARP", MW_ACL_ID_DYNAMIC_MIN, &acl_rule, &acl_action);
    return MW_E_OK;
}

/* FUNCTION NAME: system_mac_acl
 * PURPOSE:
 *      Set ACL for trapping the packet with DA=system MAC to CPU.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_OP_INCOMPLETE
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T
system_mac_acl(
    void)
{
    UI32_T unit = 0;
    UI32_T acl_rule_id = MW_ACL_ID_STATIC_SYSTEM_MAC_ADDRESS;
    AIR_ACL_RULE_T acl_rule;
    AIR_ACL_ACTION_T action;
    AIR_ERROR_NO_T rc = AIR_E_OP_INCOMPLETE;

    if ((air_acl_getRule(unit, acl_rule_id, &acl_rule) == AIR_E_OK) && (FALSE == acl_rule.rule_en))
    {
        /* Match the packets from all ports exclude the CPU port the DA of which
         * is equal to the system MAC address.
         */
        osapi_memset(&acl_rule, 0, sizeof(AIR_ACL_RULE_T));
        acl_rule.rule_en = TRUE;
        AIR_PORT_BITMAP_COPY(acl_rule.portmap, PLAT_PORT_BMP_TOTAL);
        AIR_PORT_DEL(acl_rule.portmap, PLAT_CPU_PORT);
        acl_rule.end = TRUE;
        osapi_memcpy(acl_rule.key.dmac, _mw_mac_addr, MAC_ADDRESS_LEN);
        acl_rule.mask.dmac = 0x3f;
        acl_rule.field_valid |= (1U << AIR_ACL_DMAC_KEY);
        rc = air_acl_setRule(unit, acl_rule_id, &acl_rule);
        if (rc != AIR_E_OK)
        {
            osapi_printf("Add System MAC ACL rule entry-id %d failed, rc=%d.\n", acl_rule_id, rc);
            return (rc == AIR_E_OK) ? MW_E_OK : MW_E_OP_INCOMPLETE;
        }

        /* Forward the packets matches the rule above to the CPU port only with
         * the priority of MW_ACL_RX_PRIORITY_NOMRAL_PACKET.
         */
        osapi_memset(&action, 0, sizeof(AIR_ACL_ACTION_T));
        action.acl_mib_id = acl_rule_id; /* Set mib id as acl rule id. */
        action.field_valid |= (1U << AIR_ACL_MIB);
        action.port_fw = MW_ACL_ACT_PORT_FW_CPU_ONLY;
        action.field_valid |= (1U << AIR_ACL_FW_PORT);
        action.pri_user = MW_ACL_RX_PRIORITY_NOMRAL_PACKET;
        action.field_valid |= (1U << AIR_ACL_PRI);
        rc = air_acl_setAction(unit, acl_rule_id, &action);
        if (AIR_E_OK != rc)
        {
            osapi_printf("Add System MAC ACL rule entry-id %d action fail, rc=%d.\n", acl_rule_id, rc);
            air_acl_delRule(unit, acl_rule_id);
        }
    }
    else
    {
        osapi_printf("Error occurs. Or the static ACL ID:%d is occupied unexpectly.\n", acl_rule_id);
    }

    return (rc == AIR_E_OK) ? MW_E_OK : MW_E_OP_INCOMPLETE;
}

/* FUNCTION NAME: allzero_mac_acl
 * PURPOSE:
 *      Set ACL for dropping the packet with SA/DA=all-zero MAC.
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
allzero_mac_acl(UI8_T mac_opt)
{
    UI32_T unit = 0;
    UI32_T acl_rule_id = 0;
    UI32_T port = 0;
    UI8_T index;
    AIR_ACL_RULE_T acl_rule;

    for (acl_rule_id = MW_ACL_ID_DYNAMIC_MIN; acl_rule_id <= MW_ACL_ID_DYNAMIC_MAX; acl_rule_id++)
    {
        if (air_acl_getRule(unit, acl_rule_id, &acl_rule) == AIR_E_OK)
        {
            AIR_ACL_ACTION_T action;
            AIR_ERROR_NO_T rc;

            if (FALSE == acl_rule.rule_en)
            {
                osapi_memset(&acl_rule, 0, sizeof(AIR_ACL_RULE_T));
                acl_rule.rule_en = TRUE;
                AIR_PORT_BITMAP_COPY(acl_rule.portmap, PLAT_PORT_BMP_TOTAL);
                AIR_PORT_DEL(acl_rule.portmap, PLAT_CPU_PORT);
                acl_rule.end = TRUE;
                if (SA_ZERO == mac_opt)
                {
                    acl_rule.mask.smac = 0x3f;
                    acl_rule.field_valid |= (1U << AIR_ACL_SMAC_KEY);
                }
                else if (DA_ZERO == mac_opt)
                {
                    for (index = 0; index < ALLZERO_MAC_UDF_NUM; index++)
                    {
                        if (MW_ACL_ID_INVALID == _allzero_mac_acl_udf_id[index])
                        {
                            return MW_E_BAD_PARAMETER;
                        }
                        acl_rule.key.udf |= (1 << _allzero_mac_acl_udf_id[index]);
                        acl_rule.mask.udf |= (1 << _allzero_mac_acl_udf_id[index]);
                    }

                    acl_rule.field_valid |= (1U << AIR_ACL_UDF_KEY);
                }
                rc = air_acl_setRule(unit, acl_rule_id, &acl_rule);
                if (rc != AIR_E_OK)
                {
                    osapi_printf("Add all-zero MAC ACL rule entry-id %d failed, rc=%d.\n", acl_rule_id, rc);
                    break;
                }

                osapi_memset(&action, 0, sizeof(AIR_ACL_ACTION_T));
                action.acl_mib_id = acl_rule_id; /* Set mib id as acl rule id. */
                action.field_valid |= (1U << AIR_ACL_MIB);
                action.port_fw = MW_ACL_ACT_PORT_FW_DROP;
                action.field_valid |= (1U << AIR_ACL_FW_PORT);
                rc = air_acl_setAction(unit, acl_rule_id, &action);
                if (AIR_E_OK != rc)
                {
                    osapi_printf("Add all-zero MAC ACL rule entry-id %d action fail, rc=%d.\n", acl_rule_id, rc);
                    air_acl_delRule(unit, acl_rule_id);
                    acl_rule_id = MW_ACL_ID_DYNAMIC_MAX + 1;
                }
                break;
            }
        }
        else
        {
            osapi_printf("Get ACL rule entry-id %d failed\n", acl_rule_id);
        }
    }

    if (MW_ACL_ID_DYNAMIC_MAX < acl_rule_id)
    {
        osapi_printf("Get free ACL rule-id for all-zero MAC failed !\n");
        return MW_E_ENTRY_REACH_END;
    }

    return MW_E_OK;
}

/* FUNCTION NAME: allzero_mac_udf
 * PURPOSE:
 *      Set UDF for dropping the packet with DA=all-zero MAC.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_BAD_PARAMETER
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T
allzero_mac_udf(void)
{
    UI8_T index;
    UI16_T udf_cmp;
    UI32_T unit = 0;

    for (index = 0; index < ALLZERO_MAC_UDF_NUM; index++)
    {
        _allzero_mac_acl_udf_id[index] = MW_ACL_ID_INVALID;
    }

    for (index = 0; index < ALLZERO_MAC_UDF_NUM; index++)
    {
        UI8_T acl_udf_id = 0;
        AIR_ACL_UDF_RULE_T acl_udf_rule;

        for (; acl_udf_id < AIR_MAX_NUM_OF_UDF_ENTRY; acl_udf_id++)
        {
            if (air_acl_getUdfRule(unit, acl_udf_id, &acl_udf_rule) == AIR_E_OK)
            {
                AIR_ERROR_NO_T rc;
                if (FALSE == acl_udf_rule.udf_rule_en)
                {
                    switch (index)
                    {
                    case 0:
                        acl_udf_rule.udf_rule_en = TRUE;
                        AIR_PORT_BITMAP_COPY(acl_udf_rule.portmap, PLAT_PORT_BMP_TOTAL);
                        AIR_PORT_DEL(acl_udf_rule.portmap, PLAT_CPU_PORT);
                        acl_udf_rule.udf_pkt_type = AIR_ACL_MAC_HEADER;
                        acl_udf_rule.word_ofst = 0;     /* offset 0 */
                        acl_udf_rule.cmp_sel = 0;       /* pattern */
                        acl_udf_rule.cmp_pat = 0;       /* CMP pattern */
                        acl_udf_rule.cmp_mask = 0xFFFF; /* CMP mask */
                        break;
                    case 1:
                        acl_udf_rule.udf_rule_en = TRUE;
                        AIR_PORT_BITMAP_COPY(acl_udf_rule.portmap, PLAT_PORT_BMP_TOTAL);
                        AIR_PORT_DEL(acl_udf_rule.portmap, PLAT_CPU_PORT);
                        acl_udf_rule.udf_pkt_type = AIR_ACL_MAC_HEADER;
                        acl_udf_rule.word_ofst = 1;     /* offset 2 */
                        acl_udf_rule.cmp_sel = 0;       /* pattern */
                        acl_udf_rule.cmp_pat = 0;       /* CMP pattern */
                        acl_udf_rule.cmp_mask = 0xFFFF; /* CMP mask */
                        break;
                    case 2:
                        acl_udf_rule.udf_rule_en = TRUE;
                        AIR_PORT_BITMAP_COPY(acl_udf_rule.portmap, PLAT_PORT_BMP_TOTAL);
                        AIR_PORT_DEL(acl_udf_rule.portmap, PLAT_CPU_PORT);
                        acl_udf_rule.udf_pkt_type = AIR_ACL_MAC_HEADER;
                        acl_udf_rule.word_ofst = 2;     /* offset 4 */
                        acl_udf_rule.cmp_sel = 0;       /* pattern */
                        acl_udf_rule.cmp_pat = 0;       /* CMP pattern */
                        acl_udf_rule.cmp_mask = 0xFFFF; /* CMP mask */
                        break;
                    default:
                        break;
                    }
                    rc = air_acl_setUdfRule(unit, acl_udf_id, &acl_udf_rule);
                    if (rc != AIR_E_OK)
                    {
                        osapi_printf("Add all-zero MAC ACL udf-id %d failed, rc=%d !\n", acl_udf_id, rc);
                        return MW_E_OP_INCOMPLETE;
                    }
                    _allzero_mac_acl_udf_id[index] = acl_udf_id;
                    break;
                }
            }
        }
        if (acl_udf_id == AIR_MAX_NUM_OF_UDF_ENTRY)
        {
            osapi_printf("All-zero mac udf get free ACL udf-id failed !\n");
            return MW_E_ENTRY_REACH_END;
        }
    }

    return MW_E_OK;
}

/* FUNCTION NAME: _mw_system_init
 * PURPOSE:
 *      Initial board related information
 *
 * INPUT:
 *      unit            --  The unit ID
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
MW_ERROR_NO_T
_mw_system_init(
    const UI32_T unit)
{
    MW_ERROR_NO_T ret = MW_E_OK;
    AIR_ERROR_NO_T rc = AIR_E_OK;
    unsigned char i = 0;
    C8_T def_mac_addr[MAC_ADDRESS_LEN] = {0x0, 0xaa, 0xbb, 0x11, 0x22, 0x33};
    AIR_MAC_ENTRY_T mt;
    AIR_INIT_PORT_MAP_T *ptr_portMapList = NULL, *cur_ptr = NULL;
    UI32_T ver_len = 0;
#ifdef AIR_SUPPORT_POE
    UI32_T productId = mw_get_productID();
#endif

    /*
     * Init port information
     */
    osapi_calloc((sizeof(AIR_INIT_PORT_MAP_T) * MW_DEFAULT_MAX_PORT_NUM), "MW", (void **)&ptr_portMapList);
    if (NULL != ptr_portMapList)
    {
        rc = air_init_getSdkPortMap(unit, &_mw_total_port, ptr_portMapList);
        if (AIR_E_OK == rc)
        {
            _mw_cpu_port = 0;
            AIR_PORT_BITMAP_CLEAR(_mw_port_capacity);
            AIR_PORT_BITMAP_CLEAR(_mw_port_bmp_total);

            cur_ptr = ptr_portMapList;
            for (i = 0; i < _mw_total_port; i++)
            {
                switch (cur_ptr->port_type)
                {
                case AIR_INIT_PORT_TYPE_BASET:
                    AIR_PORT_ADD(_mw_port_bmp_total, (cur_ptr->port));
                    break;
                case AIR_INIT_PORT_TYPE_XSGMII:
                    if (AIR_INIT_PORT_SPEED_2500M == cur_ptr->max_speed)
                    {
                        AIR_PORT_ADD(_mw_port_capacity, (cur_ptr->port));
                    }
                    AIR_PORT_ADD(_mw_port_bmp_total, (cur_ptr->port));
                    break;
                case AIR_INIT_PORT_TYPE_CPU:
                    _mw_cpu_port = cur_ptr->port;
                    AIR_PORT_ADD(_mw_port_bmp_total, (cur_ptr->port));
                    break;
                default:
                    osapi_printf("Wrong type of port info.\n");
                    break;
                }
                cur_ptr++;
            }
            _mw_eth_port = _mw_total_port - 1;
        }
        else
        {
            osapi_printf("air_init_getSdkPortMap get fail.\n");
        }
        osapi_free(ptr_portMapList);
    }
    else
    {
        osapi_printf("Init port fail, use default port mapping.\n");
    }

#ifdef AIR_SUPPORT_POE
    switch (productId)
    {
    case MW_PRODUCT_ID_EN8851C_8P:
    case MW_PRODUCT_ID_EN8851C_RFB:
    case MW_PRODUCT_ID_EN8851C_8P_2SFP:
    case MW_PRODUCT_ID_EN8851C_8P_2SFP_L:
    case MW_PRODUCT_ID_EN8853C_8P_EN8804_4SFP:
    case MW_PRODUCT_ID_EN8853C_8P_EN8804_8SFP:
    case MW_PRODUCT_ID_EN8853C_8P_EN8804_12SFP:
    case MW_PRODUCT_ID_EN8853C_8P_EN8804_16SFP:
        _mw_poe_port = 8;
        break;
    case MW_PRODUCT_ID_EN8853C_24P:
    case MW_PRODUCT_ID_EN8853C_RFB:
    case MW_PRODUCT_ID_EN8853C_24P_EN8804_4P_4SFP:
    case MW_PRODUCT_ID_EN8853C_24P_1SFP:
        _mw_poe_port = 24;
        break;
    case MW_PRODUCT_ID_EN8853C_16P_2SFP:
        _mw_poe_port = 16;
        break;
    default:
        _mw_poe_port = 0;
        break;
    }
#endif

    /*
     * Init system mac
     */
    read_mac_addr(_mw_mac_addr);
    osapi_printf("sys mac addr: %02X-%02X-%02X-%02X-%02X-%02X\n", _mw_mac_addr[0], _mw_mac_addr[1], _mw_mac_addr[2], _mw_mac_addr[3], _mw_mac_addr[4], _mw_mac_addr[5]);
    for (i = 0; i < MAC_ADDRESS_LEN; i++)
    {
        if (0 != _mw_mac_addr[i] && 0xFF != _mw_mac_addr[i])
        {
            break;
        }
    }
    if (MAC_ADDRESS_LEN == i)
    {
        /* Read all-zero from flash, set the default MAC address */
        memcpy(_mw_mac_addr, def_mac_addr, MAC_ADDRESS_LEN);
        osapi_printf("Use default mac addr: %02X-%02X-%02X-%02X-%02X-%02X\n", _mw_mac_addr[0], _mw_mac_addr[1], _mw_mac_addr[2], _mw_mac_addr[3], _mw_mac_addr[4], _mw_mac_addr[5]);
    }

    _system_acl_init();
    _system_arp_acl();
    system_mac_acl();
    allzero_mac_udf();
    allzero_mac_acl(SA_ZERO);
    allzero_mac_acl(DA_ZERO);
    _system_cpu_port_init();

    /*
     * Set system MAC address
     */
    rc = air_swc_setSystemMac(unit, _mw_mac_addr);
    if (AIR_E_OK != rc)
    {
        osapi_printf("Set system MAC address fail\n");
    }

    if (AIR_E_OK != rc)
    {
        ret = MW_E_OTHERS;
    }

    /*
     * Set system description
     */
    ver_len = osapi_strlen(MW_VERSION_PLATFORM) + osapi_strlen(MW_VERSION_MW) + osapi_strlen(MW_VERSION_SDK) + osapi_strlen(MW_VERSION_FREERTOS);
    if (ver_len > (MW_SYS_DESCRI_LEN_MAX - 1))
    {
        osapi_printf("Version too long.\n");
        return MW_E_BAD_PARAMETER;
    }

    osapi_memset(_mw_sys_desc, 0, MW_SYS_DESCRI_LEN_MAX);
    osapi_strcat(_mw_sys_desc, MW_VERSION_PLATFORM);
    osapi_strcat(_mw_sys_desc, MW_VERSION_MW);
    osapi_strcat(_mw_sys_desc, MW_VERSION_SDK);
    osapi_strcat(_mw_sys_desc, MW_VERSION_FREERTOS);

    return ret;
}

#ifdef AIR_SUPPORT_IPV6
static MW_ERROR_NO_T
_mw_init_set_acl_for_multicast_mac_address(
    UI8_T *ptr_mac_addr,
    UI32_T rate,
    UI8_T port_fw,
    UI16_T acl_rule_id)
{
    UI32_T meter_id = AIR_MAX_NUM_OF_METER_ID, unit = 0, meter_rate;
    BOOL_T meter_enable;
    AIR_ERROR_NO_T ret = AIR_E_OTHERS;
    AIR_ACL_RULE_T acl_rule;
    AIR_ACL_ACTION_T acl_action = {0};

    if ((NULL == ptr_mac_addr) || (FALSE == MW_ACL_ID_IS_VALID(acl_rule_id)))
    {
        return MW_E_BAD_PARAMETER;
    }

    /* Set rate if it is not 0. */
    if (rate != 0)
    {
        /* Get a disabled meter table */
        for (meter_id = 0; meter_id < AIR_MAX_NUM_OF_METER_ID; meter_id++)
        {
            if ((air_acl_getMeterTable(unit, meter_id, &meter_enable, &meter_rate) == AIR_E_OK) && (FALSE == meter_enable))
            {
                break;
            }
        }

        if (meter_id >= AIR_MAX_NUM_OF_METER_ID)
        {
            osapi_printf("[MW INIT] No available ACL meter table!\n");
            return MW_E_OP_INCOMPLETE;
        }

        /* Set the rate limitation. */
        ret = air_acl_setMeterTable(unit, meter_id, TRUE, rate);
        if (ret != AIR_E_OK)
        {
            osapi_printf("[MW INIT] air_acl_setMeterTable failed. meter_id:%d, ret=%d rate:%d\n", meter_id, ret, rate);
            return MW_E_OP_INCOMPLETE;
        }
    }

    /* Set ACL rule for mac address */
    osapi_memset(&acl_rule, 0, sizeof(AIR_ACL_RULE_T));
    acl_rule.rule_en = TRUE;
    AIR_PORT_BITMAP_COPY(acl_rule.portmap, PLAT_PORT_BMP_TOTAL);
    AIR_PORT_DEL(acl_rule.portmap, PLAT_CPU_PORT);
    acl_rule.end = TRUE;
    memcpy(acl_rule.key.dmac, ptr_mac_addr, MAC_ADDRESS_LEN);
    acl_rule.mask.dmac = 0x3f;
    acl_rule.field_valid |= (1U << AIR_ACL_DMAC_KEY);
    ret = air_acl_setRule(unit, acl_rule_id, &acl_rule);
    if (ret != AIR_E_OK)
    {
        osapi_printf("[MW INIT] air_acl_setRule failed. rule_id:%d ret=%d addr[5]:%x\n", acl_rule_id, ret, ptr_mac_addr[5]);
        if (rate != 0)
        {
            air_acl_setMeterTable(unit, meter_id, FALSE, rate);
        }
        return MW_E_OP_INCOMPLETE;
    }

    /* Set ACL action for the rule */
    acl_action.acl_mib_id = acl_rule_id;
    acl_action.field_valid |= (1U << AIR_ACL_MIB);
    if (meter_id != AIR_MAX_NUM_OF_METER_ID)
    {
        acl_action.meter_id = meter_id;
    }
    acl_action.port_fw = port_fw;
    acl_action.field_valid |= (1U << AIR_ACL_FW_PORT);
    acl_action.pri_user = MW_ACL_RX_PRIORITY_NOMRAL_PACKET;
    acl_action.field_valid |= (1U << AIR_ACL_PRI);
    ret = air_acl_setAction(unit, acl_rule_id, &acl_action);
    if (ret != AIR_E_OK)
    {
        osapi_printf("[MW INIT] air_acl_setAction failed. rule_id:%d ret=%d.\n", acl_rule_id, ret);
        air_acl_delRule(unit, acl_rule_id);
        if (rate != 0)
        {
            air_acl_setMeterTable(unit, meter_id, FALSE, rate);
        }
        return MW_E_OP_INCOMPLETE;
    }

    return MW_E_OK;
}
#endif /* AIR_SUPPORT_IPV6 */

/* FUNCTION NAME: _system_cpu_port_init
 * PURPOSE:
 *      This function initializes the CPU port.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_OP_INCOMPLETE
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T
_system_cpu_port_init(
    void)
{
    UI8_T storm_cfg = ENABLE;
    BOOL_T fc_en = DISABLE;
    UI32_T unit = 0;
    UI32_T port = PLAT_CPU_PORT;
    UI32_T bc_rate = 64;
    AIR_ERROR_NO_T air_rc = AIR_E_OK;
    AIR_SEC_STORM_TYPE_T type = AIR_SEC_STORM_TYPE_BC;
    AIR_SEC_STORM_RATE_MODE_T storm_mode = AIR_SEC_STORM_RATE_MODE_PPS;

    /* Set CPU port storm control enable [broadcast packets] */
    air_rc = air_sec_setPortStormCtrl(unit, port, type, storm_cfg);
    if (AIR_E_OK != air_rc)
    {
        osapi_printf("[MW INIT] Set CPU port storm control failed. air_rc=%d.\n", air_rc);
        return MW_E_OP_INCOMPLETE;
    }
    /* Set CPU port storm control rate [broadcast packets] */
    air_rc = air_sec_setPortStormCtrlRate(unit, port, type, storm_mode, bc_rate);
    if (AIR_E_OK != air_rc)
    {
        osapi_printf("[MW INIT] Set CPU port storm control failed. air_rc=%d.\n", air_rc);
        return MW_E_OP_INCOMPLETE;
    }

    /* Set CPU port Tx flow control */
    air_rc = air_port_setFlowCtrl(unit, port, AIR_PORT_DIR_TX, fc_en);
    /* Set CPU port Rx flow control */
    air_rc |= air_port_setFlowCtrl(unit, port, AIR_PORT_DIR_RX, fc_en);
    if (AIR_E_OK != air_rc)
    {
        osapi_printf("[MW INIT] Set CPU port flow control failed. air_rc=%d.\n", air_rc);
        return MW_E_OP_INCOMPLETE;
    }

    return MW_E_OK;
}

/* STATIC VARIABLE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
/* FUNCTION NAME: mw_init_initModule
 * PURPOSE:
 *      This API is used to initialize the middleware modules.
 *
 * INPUT:
 *      unit            --  The unit ID
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_NO_MEMORY
 *      MW_E_ALREADY_INITED
 *      MW_E_OP_STOPPED
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T
mw_init_initModule(
    const UI32_T unit)
{
    MW_ERROR_NO_T ret = MW_E_OK;

    /* Initial board platform */
    _mw_system_init(unit);

    /* Initial DB task */
    osapi_printf("db task init\n");
    db_task_init();

    /* SyncD initialization */
    osapi_printf("syncd init\n");
    syncd_init();

    osapi_printf("initSwitching init\n");
    ret = mw_init_initSwitching(unit);
    if (MW_E_OK != ret)
    {
        osapi_printf("Switching module init fail\n");
    }
    /* sys_mgmt initialization */
    osapi_printf("sys mgmt init\n");
    ret = sys_mgmt_init();
    if (MW_E_OK != ret)
    {
        osapi_printf("System management init fail\n");
    }
#ifdef AIR_SUPPORT_POE
    poe_init();
#endif
    mqttd_httpc_thread_create();
    return ret;
}

/* FUNCTION NAME: mw_init_initSwitching
 * PURPOSE:
 *      This API is used to initialize the switching modules.
 *
 * INPUT:
 *      unit            --  The unit ID
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_NO_MEMORY
 *      MW_E_ALREADY_INITED
 *      MW_E_OP_STOPPED
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T
mw_init_initSwitching(
    const UI32_T unit)
{
    MW_ERROR_NO_T ret = MW_E_OK;

    /* Initialize MAC related setting */
    mac_init();

#ifdef AIR_SUPPORT_LP
    /* Initial loop prevention module */
    ret = loop_prevent_init();
    if (MW_E_OK != ret)
    {
        osapi_printf("loop prevention init fail\n");
    }
#endif

#ifdef AIR_SUPPORT_IGMP_SNP
    /* Initial IGMP snooping thread. */
    igmp_snp_init();
#endif

#ifdef AIR_SUPPORT_DHCP_SNOOP
    /* Initial DHCP snooping */
    dhcp_snp_init();
#endif /* AIR_SUPPORT_DHCP_SNOOP */

#ifdef AIR_SUPPORT_VOICE_VLAN
    /* Initial Voice VLAN */
    voice_vlan_init();
#endif /* AIR_SUPPORT_VOICE_VLAN */
#ifdef AIR_SUPPORT_RSTP
    /* Initial RSTP */
    rstp_sys_init();
#endif /* AIR_SUPPORT_RSTP*/
#ifdef AIR_SUPPORT_LLDPD
    /*Initial LLDP daemon*/
    lldp_init();
#endif /*AIR_SUPPORT_LLDPD*/
    return ret;
}

#ifdef AIR_SUPPORT_IPV6
/* FUNCTION NAME: mw_init_set_acl_for_icmpv6_nd
 * PURPOSE:
 *      Set ACL for ICMPv6 ND at the initialization stage.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_BAD_PARAMETER
 *      MW_E_OP_INCOMPLETE
 *
 * NOTES:
 *      It must be called after the IPv6 link-local address is generated.
 */
MW_ERROR_NO_T
mw_init_set_acl_for_icmpv6_nd(
    void)
{
    UI32_T acl_udf_id = 0;
    AIR_ACL_UDF_RULE_T acl_udf_rule;
    AIR_ACL_RULE_T acl_rule;
    AIR_ACL_ACTION_T acl_action = {};
    UI32_T unit = 0;
    MW_ERROR_NO_T rc;

    /* ICMPv6 ND UDF */
    osapi_memset(&acl_udf_rule, 0, sizeof(AIR_ACL_UDF_RULE_T));
    acl_udf_rule.udf_rule_en = TRUE;
    AIR_PORT_BITMAP_COPY(acl_udf_rule.portmap, PLAT_PORT_BMP_TOTAL);
    AIR_PORT_DEL(acl_udf_rule.portmap, PLAT_CPU_PORT);
    acl_udf_rule.udf_pkt_type = AIR_ACL_L3_PAYLOAD;
    acl_udf_rule.word_ofst = 0;     /* offset 0 */
    acl_udf_rule.cmp_sel = 1;       /* threshold */
    acl_udf_rule.cmp_pat = 0x8600;  /* low threshold */
    acl_udf_rule.cmp_mask = 0x89FF; /* high threshold */
    acl_udf_id = MW_ACL_ID_INVALID;
    rc = _system_udf_add("ICMPv6 ND", &acl_udf_id, &acl_udf_rule);
    if (MW_E_OK != rc)
    {
        return rc;
    }

    /* ICMPv6 ND ACL */
    osapi_memset(&acl_rule, 0, sizeof(AIR_ACL_RULE_T));
    acl_rule.rule_en = TRUE;
    acl_rule.end = TRUE;
    AIR_PORT_BITMAP_COPY(acl_rule.portmap, PLAT_PORT_BMP_TOTAL);
    AIR_PORT_DEL(acl_rule.portmap, PLAT_CPU_PORT);
    acl_rule.isipv6 = TRUE;
    acl_rule.key.etype = ETHTYPE_IPV6;
    acl_rule.mask.etype = 0x3;
    acl_rule.key.next_header = MW_IPV6PROTO_ICMPV6;
    acl_rule.key.udf = BIT(acl_udf_id);
    acl_rule.mask.udf = acl_rule.key.udf;
    acl_rule.field_valid = BIT(AIR_ACL_ETYPE_KEY) | BIT(AIR_ACL_NEXT_HEADER_KEY) |
                           BIT(AIR_ACL_UDF_KEY);
    acl_action.port_fw = MW_ACL_ACT_PORT_FW_CPU_INCLUDE;
    acl_action.pri_user = MW_ACL_RX_PRIORITY_ICMPV6_ND;
    acl_action.field_valid = BIT(AIR_ACL_MIB) | BIT(AIR_ACL_FW_PORT) | BIT(AIR_ACL_PRI);
    rc = _system_acl_add("ICMPv6 ND", MW_ACL_ID_STATIC_ICMPV6_ND, &acl_rule, &acl_action);
    if (MW_E_OK != rc)
    {
        return rc;
    }

    return MW_E_OK;
}

/* FUNCTION NAME: mw_init_set_acl_for_multicast_mac_addresses
 * PURPOSE:
 *      Set ACL for IPv6 multicast MAC addresses at the initialization stage.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_BAD_PARAMETER
 *      MW_E_OP_INCOMPLETE
 *
 * NOTES:
 *      It must be called after the IPv6 link-local address is generated.
 */
MW_ERROR_NO_T
mw_init_set_acl_for_multicast_mac_addresses(void)
{
    UI8_T netif_num = netif_num_get();
    struct netif *ptr_netif = netif_get_by_index(netif_num);
    const ip6_addr_t *ptr_link_local_addr = NULL;
    UI16_T addr_block = 0;
    MW_ERROR_NO_T ret = MW_E_OTHERS;

    if (NULL == ptr_netif)
    {
        return MW_E_OP_INCOMPLETE;
    }

    ptr_link_local_addr = netif_ip6_addr(ptr_netif, 0);

    /* Init MAC address of the Solicited-node multicast addresses. */
    addr_block = IP6_ADDR_BLOCK7(ptr_link_local_addr);
    _mw_multicast_mac_addr[1][3] = addr_block & 0xFF;
    addr_block = IP6_ADDR_BLOCK8(ptr_link_local_addr);
    _mw_multicast_mac_addr[1][4] = (addr_block >> 8) & 0xFF;
    _mw_multicast_mac_addr[1][5] = addr_block & 0xFF;

    ret = _mw_init_set_acl_for_multicast_mac_address(_mw_multicast_mac_addr[0], 1, MW_ACL_ACT_PORT_FW_CPU_INCLUDE, MW_ACL_ID_STATIC_IPV6_ALL_NODE_ADDESS);
    ret |= _mw_init_set_acl_for_multicast_mac_address(_mw_multicast_mac_addr[1], 1, MW_ACL_ACT_PORT_FW_CPU_INCLUDE, MW_ACL_ID_STATIC_IPV6_SOLICITED_NODE_ADDESS);
    if (ret != MW_E_OK)
    {
        osapi_printf("[MW INIT] set ACL failed. ret=%d.\n", ret);
        return ret;
    }

    return ret;
}
#endif /* AIR_SUPPORT_IPV6 */
