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

/* FILE NAME:   logon.c
 * PURPOSE:
 *      CGI and SSI function of logon web page.
 * NOTES:
 */

/* INCLUDE FILE DECLARATIONS
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "web.h"
#include "db_api.h"
#include "osapi_memory.h"
#include "httpd_queue.h"
#include <lwip/apps/httpd.h>
#ifdef AIR_8851_SUPPORT
#include "mbedtls/sha256.h"
#endif

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* GLOBAL VARIABLE DECLARATIONS
 */
LOGON_ERR_TYPE_T logon_errtype = LOGON_CFM_OK;

/* LOCAL SUBPROGRAM DECLARATIONS
 */

/* STATIC VARIABLE DECLARATIONS
 */

/* LOCAL SUBPROMGRAM BODIES
*/

/* EXPORTED SUBPROGRAM BODIES
 */

const char* cgi_set_logon_info_handle(char* pcValue[])
{
    DB_MSG_T *db_msg = NULL;
    UI16_T db_size = 0;
    void *db_data = NULL;
    C8_T passwd[MAX_PASSWORD_SIZE];
    C8_T username[MAX_USER_NAME_SIZE];
    UI8_T logon_fail_count = 0;
    MW_ERROR_NO_T rc = MW_E_OK;
#ifdef AIR_8851_SUPPORT
    UI8_T hashPass[MAX_PASSWORD_SIZE] = {0};

    /* Use SHA-256 to hash the password */
    if ((NULL == pcValue[1]) || ('\0' == pcValue[1][0]))
    {
        logon_errtype = LOGON_CFM_FAIL;
        return "/index.html";
    }
    if (MW_E_OK != mbedtls_sha256_ret((const UI8_T *)pcValue[1],
                        strlen(pcValue[1]),
                        hashPass,
                        FALSE))
    {
        logon_errtype = LOGON_CFM_FAIL;
        return "/index.html";
    }
#endif

    /* get db username*/
    rc = httpd_queue_getData(ACCOUNT_INFO, ACC_USERNAME, DB_ALL_ENTRIES, &db_msg, &db_size, &db_data);
    if(MW_E_OK == rc)
    {
        LWIP_DEBUGF(HTTPD_DEBUG, ("get username success, ptr_msg =%p\n", db_msg));
    } else
    {
        LWIP_DEBUGF(HTTPD_DEBUG, ("get username failed \n"));
        return (const char*)ERR_VAL;
    }
    memset(username, 0, sizeof(username));
    memcpy(username, db_data, db_size);
    MW_FREE(db_msg);

    /* get db password*/
    rc = httpd_queue_getData(ACCOUNT_INFO, ACC_PASSWD, DB_ALL_ENTRIES, &db_msg, &db_size, &db_data);
    if(MW_E_OK == rc)
    {
        LWIP_DEBUGF(HTTPD_DEBUG, ("get password success, ptr_msg =%p\n", db_msg));
    } else
    {
        LWIP_DEBUGF(HTTPD_DEBUG, ("get password failed \n"));
        return (const char*)ERR_VAL;
    }
    memset(passwd, 0, sizeof(passwd));
    memcpy(passwd, db_data, db_size);
    MW_FREE(db_msg);

#ifdef AIR_8851_SUPPORT
    if ((strcmp(pcValue[0], username) == 0) && (memcmp(hashPass, passwd, MAX_PASSWORD_SIZE) == 0))
#else
    if ((strcmp(pcValue[0], username) == 0) && (strcmp(pcValue[1], passwd) == 0))
#endif
    {
        logon_errtype = LOGON_CFM_OK;
        logon_fail_count = 0;
        rc = httpd_queue_setData(M_UPDATE, LOGON_INFO, LOGON_FAIL_COUNT, DB_ALL_ENTRIES, &logon_fail_count, sizeof(logon_fail_count));
        if(MW_E_OK != rc)
        {
            LWIP_DEBUGF(HTTPD_DEBUG, ("set logon info failed \n"));
        }
        return "/index_re.html";
    }
    else
    {
        logon_errtype = LOGON_CFM_FAIL;
        /* get db logon fail count*/
        rc = httpd_queue_getData(LOGON_INFO, LOGON_FAIL_COUNT, DB_ALL_ENTRIES, &db_msg, &db_size, &db_data);
        if(MW_E_OK == rc)
        {
            LWIP_DEBUGF(HTTPD_DEBUG, ("get logon fail count success, ptr_msg =%p\n", db_msg));
        }
        else
        {
            LWIP_DEBUGF(HTTPD_DEBUG, ("get logon fail count failed \n"));
            return (const char*)ERR_VAL;
        }
        memcpy(&logon_fail_count, db_data, db_size);
        MW_FREE(db_msg);
        if(logon_fail_count < 255)
        {
            logon_fail_count ++;
        }
        else
        {
            logon_fail_count = 1;
        }
        rc = httpd_queue_setData(M_UPDATE, LOGON_INFO, LOGON_FAIL_COUNT, DB_ALL_ENTRIES, &logon_fail_count, sizeof(logon_fail_count));
        if(MW_E_OK != rc)
        {
            LWIP_DEBUGF(HTTPD_DEBUG, ("set logon info failed \n"));
        }
        return "/index.html";
    }
}

char ssi_get_errtype_info_Handle(int *length, struct tcp_pcb *pcb, unsigned int apiflags)
{
    UI32_T total_len = 0, len = 0;
    C8_T err = 0;

#if HTTPD_DBG_ON
    DEBUG(debugflags, "[%s] line [%d] entered\n", __FUNCTION__, __LINE__);
#endif

    err = send_format_response(&len, pcb, apiflags, "%d", logon_errtype);
    if(ERR_OK != err)
    {
        return err;
    }

    logon_errtype = LOGON_CFM_OK;

    total_len = len;
    *length = total_len;

#if HTTPD_DBG_ON
    DEBUG(debugflags, "[%s] line [%d] leave\n", __FUNCTION__, __LINE__);
#endif

    return ERR_OK;
}

MW_ERROR_NO_T
ssi_get_connection_info_Handle(
    int *ptr_length,
    struct tcp_pcb *ptr_pcb,
    unsigned int apiflags)
{
    char err = 0;
    UI32_T len = 0;

    if(HTTPD_CONNECTION_FULL == http_cookie_index || HTTPD_CONNECTION_TIMEOUT == http_cookie_index)
    {
        if(HTTPD_CONNECTION_FULL == http_cookie_index)
        {
            logon_errtype = LOGON_CFM_FULL;
        }
        err = send_format_response(&len, ptr_pcb, apiflags, "<script>var allow = 0;</script>");
    }
    else
    {
        err = send_format_response(&len, ptr_pcb, apiflags, "<script>var allow = 1;</script>");
    }

    if(err != ERR_OK)
    {
        return MW_E_OTHERS;
    }

    *ptr_length = len;

    return MW_E_OK;
}

MW_ERROR_NO_T
ssi_get_cookie_info_Handle(
    int *ptr_length,
    struct tcp_pcb *ptr_pcb,
    unsigned int apiflags)
{
    char err = 0;
    UI32_T len = 0;

    if(HTTPD_CONNECTION_FULL == http_cookie_index || HTTPD_CONNECTION_TIMEOUT == http_cookie_index)
    {
        err = send_format_response_no_chunk(&len, ptr_pcb, apiflags, "");
    }
    else
    {
        err = send_format_response_no_chunk(&len, ptr_pcb, apiflags, "Set-cookie: Cookies=%s\r\n", http_cookies[http_cookie_index].cookie);
    }
    if(err != ERR_OK)
    {
        return MW_E_OTHERS;
    }

    *ptr_length = len;

    return MW_E_OK;
}

MW_ERROR_NO_T
cgi_set_handle_logout(
    I32_T iIndex,
    I32_T iNumParams,
    C8_T *ptr_pcParam[],
    C8_T *ptr_pcValue[])
{
    int i = 0, idx = 0, cookieLen = 0;

    for(i = 0; i < iNumParams; i ++)
    {
        if(!strcmp(ptr_pcParam[i], "Cookies"))
        {
            for(idx = 0; idx < HTTPD_MAX_LOGIN_NUM; idx ++)
            {
                cookieLen = strlen(http_cookies[idx].cookie);
                if(0 != cookieLen)
                {
                    if(0 == strncmp(ptr_pcValue[i], http_cookies[idx].cookie, cookieLen))
                    {
                        memset(http_cookies[idx].cookie, 0, HTTPD_MAX_COOKIE_LEN);
                        http_cookies[idx].idleTime = 0;
                        break;
                    }
                }
            }
        }
    }

    return MW_E_OK;
}
