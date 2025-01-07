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

/* FILE NAME:   macCfg.c
 * PURPOSE:
 *      CGI and SSI function of static MAC address configuration web page.
 * NOTES:
 */

/* INCLUDE FILE DECLARATIONS
 */
#include <string.h>
#include "osapi_string.h"

#include "mw_utils.h"
#include "mac_utils.h"
#include "db_api.h"
#include "httpd_queue.h"
#include "web.h"

/* NAMING CONSTANT DECLARATIONS
 */
#define MACCFG_PORT_MAX_HOST_NUM    (100)

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */
typedef struct ONE_DB_STATIC_MAC_ENTRY_S{
    MW_MAC_T    mac_addr; /* The static MAC address */
    UI16_T      vid;      /* The VID of the static MAC */
    UI16_T      port;     /* The port of the static MAC */
}ONE_DB_STATIC_MAC_ENTRY_T;

typedef struct CGI_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_S{
    BOOL_T      firstreq; /* First request or not */
    UI8_T       times;    /* The times of DB updates required to load the whole webpage */
}CGI_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_T;

/* GLOBAL VARIABLE DECLARATIONS
 */

/* LOCAL SUBPROGRAM DECLARATIONS
 */

/* STATIC VARIABLE DECLARATIONS
 */
static CGI_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_T _cgi_dynamic_mac_address_entry_info = {0};

/* LOCAL SUBPROMGRAM BODIES
*/

/* EXPORTED SUBPROGRAM BODIES
 */
//==========================================================================================
//  CGI handler
//==========================================================================================
MW_ERROR_NO_T
cgi_set_handle_maxMacNumSet(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[])
{
    int i;
    MW_ERROR_NO_T rc = MW_E_OK;
    UI32_T portBmp = 0;
    UI16_T maxnum = 0;
    UI32_T portIdx = 0;
#if(defined(AIR_SUPPORT_SFP))
    UI32_T trunkpbm = 0;
#endif
    /* expected CGI format:
     * maxmacnumSet.cgi?port=1&max=10 */

    /* get value from url params */
    for (i = 0; i < iNumParams; i++)
    {
        if (!strcmp(pcParam[i], "portBmp"))
        {
            portBmp = atoi(pcValue[i]);
        }
        if (!strcmp(pcParam[i], "max"))
        {
            maxnum = atoi(pcValue[i]);
        }
#if(defined(AIR_SUPPORT_SFP))
        if (0 == strcmp(pcParam[i], "trunkBitMap"))
        {
            trunkpbm = atoi(pcValue[i]);
        }
#endif
    }
#if(defined(AIR_SUPPORT_SFP))
    CGI_UTILITY_REVISE_PBMP_BY_TRUNK_INFO(trunkpbm,&portBmp,0);
#endif
    /* parser params to db format */
    DEBUG(debugflags, "<%s:%u>Set DB port=%u,maxnum=%u,size=%lu\n", __func__, __LINE__, portBmp, maxnum, sizeof(maxnum));
    for (portIdx = 0; portIdx < PLAT_MAX_PORT_NUM; portIdx++)
    {
        if (0 != ( portBmp & (1<<portIdx) ))
        {
            rc = httpd_queue_setData(M_UPDATE, PORT_CFG_INFO, PORT_MAC_LIMIT, (portIdx+1), &maxnum, sizeof(maxnum));
        }
        if (MW_E_OK != rc)
        {
            break;
        }
    }
    return rc;
}

MW_ERROR_NO_T
cgi_set_handle_addStaticMac(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[])
{
    int i;
    MW_ERROR_NO_T rc = MW_E_OK;
    char mac_str[13] = {0};
    UI16_T idx = 0;
    ONE_DB_STATIC_MAC_ENTRY_T mac_data = {0};
    ether_addr_t ether_addr;
    UI32_T portIdx = 0, port =0;
#if(defined(AIR_SUPPORT_SFP))
    UI32_T trunkpbm = 0;
#endif

    /* expected CGI format:
     * addstaticMac.cgi?port=0&vlan=1&mac=001122335599&idx=3 */
    /* get value from url params */
    for (i = 0; i < iNumParams; i++)
    {
        if (!strcmp(pcParam[i], "port"))
        {
            port = atoi(pcValue[i]);
        }
        else if (!strcmp(pcParam[i], "vlan"))
        {
            mac_data.vid = (UI16_T)atoi(pcValue[i]);
        }
        else if (!strcmp(pcParam[i], "mac"))
        {
            osapi_memset(mac_str, 0, 13);
            osapi_strncpy(mac_str, pcValue[i], 12);
        }
        else if (!strcmp(pcParam[i], "idx"))
        {
            idx = atoi(pcValue[i]);
        }
#if(defined(AIR_SUPPORT_SFP))
        else if (0 == strcmp(pcParam[i], "trunkBitMap"))
        {
            trunkpbm = atoi(pcValue[i]);
        }
#endif
    }
#if(defined(AIR_SUPPORT_SFP))
    CGI_UTILITY_REVISE_PBMP_BY_TRUNK_INFO(trunkpbm,&port,0);
#endif
    memset(mac_data.mac_addr, 0, sizeof(MW_MAC_T));
    strToMac((UI8_T*)mac_str, &ether_addr);
    memcpy(mac_data.mac_addr, ether_addr.octet, ETHER_ADDR_LEN);

    /* parser params to db format */
    for (i = 0; i < ETHER_ADDR_LEN; i++)
    {
        DEBUG(debugflags, " %02X", mac_data.mac_addr[i]);
    }
    DEBUG(debugflags, "\n");

    for (portIdx = 0; portIdx < PLAT_MAX_PORT_NUM; portIdx++)
    {
        if (0 != ( port & (1<<portIdx) ))
        {
            mac_data.port =  (portIdx+1);
            rc = httpd_queue_setData(M_CREATE, STATIC_MAC_ENTRY, DB_ALL_FIELDS, idx, &mac_data, sizeof(mac_data));
            DEBUG(debugflags, "<%s:%u>Set DB idx=%u,port=%u,vid=%u,mac=", __func__, __LINE__, idx, mac_data.port, mac_data.vid);
            //printf("<%s:%u>Set DB idx=%u,port=%u,vid=%u,mac=\n", __func__, __LINE__, idx, mac_data.port, mac_data.vid);
        }
        if (MW_E_OK != rc)
        {
            break;
        }
    }
    /* SyncD will delete the new mac entry if creation failed.
     * Add a few delay to get the correct data from DB */
    osapi_delay(100);

    return rc;
}

MW_ERROR_NO_T
cgi_set_handle_delStaticMac(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[])
{
    int i;
    MW_ERROR_NO_T rc, ret = MW_E_OK;
    UI32_T pbm = 0;
    UI16_T idx;

    /* expected CGI format:
     * delstaticmac.cgi?del_bit=3 */

    /* get value from url params */
    for (i = 0; i < iNumParams; i++)
    {
        if(!strcmp(pcParam[i],"del_bit"))
        {
            pbm = strtoul(pcValue[i], NULL, 10);
        }
    }
    /* parser params to db format */
    for (idx = 1; idx <= MAX_STATIC_MAC_NUM; idx++)
    {
        if (pbm & BIT(idx - 1))
        {
            DEBUG(debugflags, "<%s:%u>Delete entry(%u)\n", __func__, __LINE__, idx);
            rc = httpd_queue_setData(M_DELETE,
                    STATIC_MAC_ENTRY, DB_ALL_FIELDS, idx,
                    NULL, 0);
            if (MW_E_OK != rc)
            {
                DEBUG(debugflags, "<%s:%u>Delete entry(%u) fail(%d)\n", __func__, __LINE__, idx, rc);
                ret = rc;
                continue;
            }
        }
    }

    return ret;
}

MW_ERROR_NO_T
cgi_set_handle_freshDynamicMac(
    int iIndex,
    int iNumParams,
    char *ptr_param[],
    char *ptr_value[])
{
    int i;
    MW_ERROR_NO_T rc = MW_E_OK;
    UI8_T action = AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_DEFAULT;
    /* expected CGI format:
     * freshdynamicmac.cgi?firstreq=1&times=4 */

    /* get value from url params */
    for (i = 0; i < iNumParams; i++)
    {
        if(0 == strcmp(ptr_param[i],"firstreq"))
        {
            _cgi_dynamic_mac_address_entry_info.firstreq = (1 == atoi(ptr_value[i])) ? TRUE : FALSE;
        }

        if(0 == strcmp(ptr_param[i],"times"))
        {
            _cgi_dynamic_mac_address_entry_info.times = atoi(ptr_value[i]);
        }
    }
    if((TRUE == _cgi_dynamic_mac_address_entry_info.firstreq) && (0 != _cgi_dynamic_mac_address_entry_info.times))
    {
        action = AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_ACTION_START;
    }
    else if((FALSE == _cgi_dynamic_mac_address_entry_info.firstreq) && (0 != _cgi_dynamic_mac_address_entry_info.times))
    {
        action = AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_ACTION_CONTINUE;
    }
    else
    {
        return ERR_VAL;
    }
    rc = httpd_queue_setData(M_UPDATE, DYNAMIC_MAC_ADDRESS_ENTRY_CFG, ACTION_RESULT, DB_ALL_ENTRIES, &action, sizeof(UI8_T));
    return rc;
}

//==========================================================================================
//  SSI handler
//==========================================================================================
char
ssi_get_static_mac_info_Handle(
    int *length,
    struct tcp_pcb *pcb,
    unsigned int apiflags)
{
    char err = 0;
    int len = 0, total_len = 0, i = 0;
    char mac_str[13] = {0};
    MW_ERROR_NO_T mw_rc = MW_E_OK;
    DB_MSG_T *ptr_msg;
    DB_STATIC_MAC_ENTRY_T *ptr_mac_entry;
    MW_MAC_T *ptr_mac_addr;
    UI16_T data_size;
    UI8_T ety_cnt = 0;
    ether_addr_t ether_addr;

    /*  expected javascript string
     *  <script>
     *  var staticinfo_ds = {
     *  sys_mac:'00aabb112233',
     *  mac_info:[
     *      {idx:1,vids:1000,mac:'001122334455',port:1},
     *      {idx:2,vids:2000,mac:'00aabbccddee',port:8},],
     *  hostnum:2,
     *  maxhostnum:32,
     *  }
     *  </script>
     */
    DEBUG(debugflags, "<%s:%u> IN ===>\n", __func__, __LINE__);
    err = send_format_response((UI32_T*)&len, pcb, apiflags, "<script>var staticinfo_ds={" );
    if (ERR_OK != err)
    {
        return err;
    }
    total_len += len;

    /* Get system MAC address */
    mw_rc = httpd_queue_getData(
            SYS_OPER_INFO, SYS_OPER_MAC, 1,
            &ptr_msg, &data_size, (void**)&ptr_mac_addr);
    if (MW_E_OK != mw_rc)
    {
        DEBUG(debugflags, "<%s:%u> Error out\n", __func__, __LINE__);
        return ERR_INPROGRESS;
    }
    memcpy(ether_addr.octet, *ptr_mac_addr, ETHER_ADDR_LEN);
    MW_FREE(ptr_msg);
    macToStr(&ether_addr, (UI8_T*)mac_str, FALSE);
    DEBUG(debugflags, "<%s:%u> sys_mac:'%s'\n", __func__, __LINE__, mac_str);
    err = send_format_response((UI32_T*)&len, pcb, apiflags,
            "sys_mac:'%s',mac_info:[", mac_str);
    if (ERR_OK != err)
    {
        return err;
    }
    total_len += len;

    /* Get static MAC address information */
    mw_rc = httpd_queue_getData(
            STATIC_MAC_ENTRY, DB_ALL_FIELDS, DB_ALL_ENTRIES,
            &ptr_msg, &data_size, (void**)&ptr_mac_entry);
    if (MW_E_OK != mw_rc)
    {
        DEBUG(debugflags, "<%s:%u> Error out\n", __func__, __LINE__);
        return ERR_INPROGRESS;
    }

    for (i = 0; i < MAX_STATIC_MAC_NUM; i++)
    {
        /* Port value in DB is 1-based, 0 means invalid entry */
        if (0 != ptr_mac_entry ->port[i])
        {
            ety_cnt++;
            memcpy(ether_addr.octet, ptr_mac_entry ->mac_addr[i], ETHER_ADDR_LEN);
            macToStr(&ether_addr, (UI8_T*)mac_str, FALSE);
            DEBUG(debugflags, "<%s:%u> {idx:%u,vids:%u,mac:'%s',port:%u}\n", __func__, __LINE__,
                i+1,
                ptr_mac_entry ->vid[i],
                mac_str,
                ptr_mac_entry ->port[i]);
            /* Send E-ID as entry index*/
            err = send_format_response((UI32_T*)&len, pcb, apiflags,
                "{idx:%u,vids:%u,mac:'%s',port:%u},",
                i+1,
                ptr_mac_entry ->vid[i],
                mac_str,
                ptr_mac_entry ->port[i]);
            if (ERR_OK != err)
            {
                continue;
            }
            total_len += len;
        }
    }
    /* Release allocated memory before return */
    MW_FREE(ptr_msg);

    DEBUG(debugflags, "<%s:%u> ety_cnt:%u\n", __func__, __LINE__, ety_cnt);
    err = send_format_response((UI32_T*)&len, pcb, apiflags,
        "],hostnum:%u,maxhostnum:%u,};</script>",
        ety_cnt,
        MAX_STATIC_MAC_NUM
        );
    if (ERR_OK != err)
    {
        return err;
    }
    total_len += len;

    *length = total_len;

    DEBUG(debugflags, "<%s:%u> <=== OUT\n", __func__, __LINE__);

    return ERR_OK;
}
char
ssi_get_port_maxmac_info_Handle(
    int *length,
    struct tcp_pcb *pcb,
    unsigned int apiflags)
{
    char hostnumbuf[128] = {0};
    char err = 0;
    int len = 0, i = 0;
    int buf_len = 0;
    MW_ERROR_NO_T mw_rc = MW_E_OK;
    DB_MSG_T *ptr_msg;
    UI16_T data_size;
    UI16_T *ptr_mac_limit;        /* The maximum MAC number of the port */

    /*  expected value
     *  <script>
     *  var portinfo_ds = {
     *      portnum:28,
     *      maxhostnum:100,
     *      maxhostnums:[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,]}
     *  </script>
     */
    DEBUG(debugflags, "<%s:%u> IN ===>\n", __func__, __LINE__);

    mw_rc = httpd_queue_getData(
            PORT_CFG_INFO, PORT_MAC_LIMIT, DB_ALL_ENTRIES,
            &ptr_msg, &data_size, (void**)&ptr_mac_limit);
    if (MW_E_OK != mw_rc)
    {
        DEBUG(debugflags, "<%s:%u> Error out\n", __func__, __LINE__);
        return ERR_INPROGRESS;
    }

    for (i = 0; i < PLAT_MAX_PORT_NUM; i++)
    {
        buf_len += osapi_snprintf(hostnumbuf + buf_len, sizeof(hostnumbuf) - buf_len, "%u,", ptr_mac_limit[i]);
    }

    len = 0;
    err = send_format_response((UI32_T*)&len, pcb, apiflags,
        "<script>var portinfo_ds={portnum:%u,maxhostnum:%u,maxhostnums:[%s]}</script>",
        PLAT_MAX_PORT_NUM,
        MACCFG_PORT_MAX_HOST_NUM,
        hostnumbuf);

    /* Release allocated memory */
    MW_FREE(ptr_msg);

    if (ERR_OK != err)
    {
        return err;
    }

    *length = len;

    DEBUG(debugflags, "<%s:%u> <=== OUT\n", __func__, __LINE__);

    return ERR_OK;
}

char
ssi_get_vlan_mac_info_Handle(
    int *length,
    struct tcp_pcb *pcb,
    unsigned int apiflags)
{
    int len = 0, i = 0;
    char err = 0, vid_buf[161] = {0};
    int buf_len = 0;
    MW_ERROR_NO_T mw_rc = MW_E_OK;
    DB_MSG_T *ptr_msg;
    UI16_T data_size;
    UI16_T *ptr_vlan_id;
    UI8_T ety_cnt = 0;

    /*  expected value
     *  <script>
     *  var vlan_ds = {
     *      vids:[1,100,200,1000,2000],
     *      count:5}
     *  </script>
     */
    DEBUG(debugflags, "<%s:%u> IN ===>\n", __func__, __LINE__);

    mw_rc = httpd_queue_getData(
            VLAN_ENTRY, VLAN_ID, DB_ALL_ENTRIES,
            &ptr_msg, &data_size, (void**)&ptr_vlan_id);
    if (MW_E_OK != mw_rc)
    {
        DEBUG(debugflags, "<%s:%u> Error out\n", __func__, __LINE__);
        return ERR_INPROGRESS;
    }

    for (i = 0; i < MAX_VLAN_ENTRY_NUM; i++)
    {
        if( 0 != ptr_vlan_id[i])
        {
            ety_cnt++;
            buf_len += osapi_snprintf(vid_buf + buf_len, sizeof(vid_buf) - buf_len, "%u,", ptr_vlan_id[i]);
        }
    }

    /* Release allocated memory */
    MW_FREE(ptr_msg);

    DEBUG(debugflags, "<%s:%u> cnt=%u vlans=[%s]\n", __func__, __LINE__, ety_cnt, vid_buf);
    err = send_format_response((UI32_T*)&len, pcb, apiflags,
        "<script>var vlan_ds={vids:[%s],count:%u};</script>",
        vid_buf,
        ety_cnt);
    if (ERR_OK != err)
    {
        return err;
    }

    *length = len;

    DEBUG(debugflags, "<%s:%u> <=== OUT\n", __func__, __LINE__);

    return ERR_OK;
}

char
ssi_get_dynamic_mac_address_entry_info_Handle(
    int *ptr_length,
    struct tcp_pcb *ptr_pcb,
    unsigned int apiflags)
{
    char err = 0;
    int len = 0;
    /*  expected javascript string
     *  <script>
     *  var dynamiccfg = {
     *  maxshownums:200,
     *  perrequests:50,
     *  }
     *  </script>
     */
    DEBUG(debugflags, "<%s:%u> IN ===>\n", __func__, __LINE__);
    err = send_format_response((UI32_T*)&len, ptr_pcb, apiflags,
        "<script>var dynamiccfg={maxshownums:%u,perrequests:%u,};</script>",
        MAX_PER_WEBPAGE_SHOW_DYNAMIC_MAC_ADDRESS_ENTRY_NUM,
        PER_REQUESTED_DYNAMIC_MAC_ADDRESS_ENTRY_NUM
        );
    if (ERR_OK != err)
    {
        return err;
    }
    *ptr_length = len;
    DEBUG(debugflags, "<%s:%u> <=== OUT\n", __func__, __LINE__);
    return ERR_OK;
}

char
ssi_get_dynamic_mac_address_entry_xmlHandle(
    int *ptr_length,
    struct tcp_pcb *ptr_pcb,
    unsigned int apiflags)
{
    char err = 0;
    int len = 0, total_len = 0, i = 0;
    BOOL_T invalid = FALSE;
    MW_ERROR_NO_T mw_rc = MW_E_OK;
    DB_MSG_T *ptr_db_msg;
    UI16_T db_size;
    void *ptr_db_data = NULL;
    DB_DYNAMIC_MAC_ADDRESS_ENTRY_T *ptr_dynamic_mac_address_entry = NULL;
    char mac_str[18] = {0};
    DB_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_T dynamic_mac_address_entrycfg = {0};
    UI8_T action = AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_DEFAULT;
    ether_addr_t ether_addr;
    /*  expected xml string
     *  8;
     *  3;
     *  1000,001122334455,1,300;
     *  2000,00aabbccddee,8,200;
     *  5,00aabbccddff,5,127;
     */
    UNUSED(action);
    DEBUG(debugflags, "<%s:%u> IN ===>\n", __func__, __LINE__);
    /* Get dynamic mac configuration*/
    mw_rc = httpd_queue_getData(DYNAMIC_MAC_ADDRESS_ENTRY_CFG, DB_ALL_FIELDS, DB_ALL_ENTRIES,&ptr_db_msg, &db_size, &ptr_db_data);
    if (MW_E_OK != mw_rc)
    {
        DEBUG(debugflags, "<%s:%u> Error out\n", __func__, __LINE__);
        return ERR_INPROGRESS;
    }
    memcpy(&dynamic_mac_address_entrycfg, ptr_db_data, db_size);
    MW_FREE(ptr_db_msg);
    do
    {
        if(((AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_START_DONE    == dynamic_mac_address_entrycfg.action_result) ||
            (AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_START_END      == dynamic_mac_address_entrycfg.action_result) ||
            (AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_CONTINUE_DONE  == dynamic_mac_address_entrycfg.action_result) ||
            (AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_CONTINUE_END   == dynamic_mac_address_entrycfg.action_result)) &&
            (0 != dynamic_mac_address_entrycfg.dynamic_entry_count))
        {
            if((TRUE == _cgi_dynamic_mac_address_entry_info.firstreq) &&
               ((AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_START_DONE != dynamic_mac_address_entrycfg.action_result) &&
                (AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_START_END  != dynamic_mac_address_entrycfg.action_result)))
            {
                /* it may be the case the action is GET_START but the result is CONT_DONE or CONT_END */
                invalid = TRUE;
                break;
            }
            if((FALSE == _cgi_dynamic_mac_address_entry_info.firstreq) &&
               ((AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_START_DONE == dynamic_mac_address_entrycfg.action_result) ||
                (AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_START_END  == dynamic_mac_address_entrycfg.action_result)))
            {
                /* it may be the case the action is GET_CONTINUE but the result is START_DONE or START_END */
                invalid = TRUE;
                break;
            }
            if(PER_REQUESTED_DYNAMIC_MAC_ADDRESS_ENTRY_NUM < dynamic_mac_address_entrycfg.dynamic_entry_count)
            {
                invalid = TRUE;
                break;
            }

            err = send_format_response((UI32_T*)&len, ptr_pcb, apiflags,
            "%u;%u;",
            dynamic_mac_address_entrycfg.action_result,
            dynamic_mac_address_entrycfg.dynamic_entry_count);
            if (ERR_OK != err)
            {
                return err;
            }
            total_len += len;
            /* Get dynamic MAC address information */
            mw_rc = httpd_queue_getData(
                DYNAMIC_MAC_ADDRESS_ENTRY, DB_ALL_FIELDS, DB_ALL_ENTRIES,
                &ptr_db_msg, &db_size, (void**)&ptr_dynamic_mac_address_entry);
            if (MW_E_OK != mw_rc)
            {
                DEBUG(debugflags, "<%s:%u> Error out\n", __func__, __LINE__);
                return ERR_INPROGRESS;
            }

            _cgi_dynamic_mac_address_entry_info.times--;
            _cgi_dynamic_mac_address_entry_info.firstreq = FALSE;
            /* Update next n dynamic MAC to DB */
            if(((AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_START_END != dynamic_mac_address_entrycfg.action_result) &&
                (AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_RESULT_CONTINUE_END != dynamic_mac_address_entrycfg.action_result)) &&
                (0 < _cgi_dynamic_mac_address_entry_info.times))
            {
                action = AIR_DYNAMIC_MAC_ADDRESS_ENTRY_CFG_ACTION_CONTINUE;
                mw_rc = httpd_queue_setData(M_UPDATE, DYNAMIC_MAC_ADDRESS_ENTRY_CFG, ACTION_RESULT, DB_ALL_ENTRIES, &action, sizeof(UI8_T));
            }

            for (i = 0; i < dynamic_mac_address_entrycfg.dynamic_entry_count; i++)
            {
                memcpy(ether_addr.octet, ptr_dynamic_mac_address_entry->mac_addr[i], ETHER_ADDR_LEN);
                macToStr(&ether_addr, (UI8_T*)mac_str, TRUE);
                /* Send dynamic MAC entry */
                err = send_format_response((UI32_T*)&len, ptr_pcb, apiflags,
                                            "%u,%s,0x%x,%u;",
                                            ptr_dynamic_mac_address_entry->vid[i],
                                            mac_str,
                                            ptr_dynamic_mac_address_entry->port[i],
                                            ptr_dynamic_mac_address_entry->age[i]);
                if (ERR_OK != err)
                {
                    continue;
                }
                total_len += len;
                DEBUG(debugflags, "{i:%u,vids:%u,mac:%s,port:%u,age:%u}\n",
                        i,
                        ptr_dynamic_mac_address_entry->vid[i],
                        mac_str,
                        ptr_dynamic_mac_address_entry->port[i],
                        ptr_dynamic_mac_address_entry->age[i]);
            }
            /* Release allocated memory before return */
            MW_FREE(ptr_db_msg);
        }
        else
        {
            dynamic_mac_address_entrycfg.dynamic_entry_count = 0;
            err = send_format_response((UI32_T*)&len, ptr_pcb, apiflags,
                "%u;%u; ;",
                dynamic_mac_address_entrycfg.action_result,
                dynamic_mac_address_entrycfg.dynamic_entry_count);
            if (ERR_OK != err)
            {
                return err;
            }
            total_len += len;
        }
    }while (0);

    if(TRUE == invalid)
    {
        err = send_format_response((UI32_T*)&len, ptr_pcb, apiflags, "%u;%u; ;" ,0 ,0);
        if (ERR_OK != err)
        {
            return err;
        }
        total_len += len;
    }
    DEBUG(debugflags, "<%s:%u> <=== OUT\n", __func__, __LINE__);
    return ERR_OK;
}

