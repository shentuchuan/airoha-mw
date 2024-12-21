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

#include "mw_error.h"
#include "mw_types.h"

#include "osapi.h"
#include "osapi_string.h"

#include "mw_cmd_parser.h"
#include "mw_cmd_util.h"
#include "mw_cmd_mqttd.h"

#include "lwip/apps/http_client.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/altcp.h"
#include "lwip/altcp.h"
#include "lwip/ip_addr.h"
#include "lwip/altcp_tcp.h"

#ifdef AIR_SUPPORT_MQTTD
#include "mqttd.h"
#include "inet_utils.h"
#include "sys_mgmt.h"

#define sys_httpc_debug(fmt, ...)                                                          \
    do                                                                                     \
    {                                                                                      \
        osapi_printf("<%s:%d>(%s)" fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* LOCAL SUBPROGRAM SPECIFICATIONS
 */
static MW_ERROR_NO_T _mqttd_cmd_enable(const C8_T *tokens[], UI32_T token_idx);
static MW_ERROR_NO_T _mqttd_cmd_dump_topic(const C8_T *tokens[], UI32_T token_idx);
static MW_ERROR_NO_T _mqttd_cmd_debug(const C8_T *tokens[], UI32_T token_idx);
static MW_ERROR_NO_T _mqttd_cmd_show_state(const C8_T *tokens[], UI32_T token_idx);
static MW_ERROR_NO_T _mqttd_cmd_coding(const C8_T *tokens[], UI32_T token_idx);
static MW_ERROR_NO_T _mqttd_cmd_json(const C8_T *tokens[], UI32_T token_idx);
static MW_ERROR_NO_T _mqttd_http_test_json(const C8_T *tokens[], UI32_T token_idx);

/* GLOBAL VARIABLE DECLARATIONS
 */
static MW_CMD_VEC_T _mw_mqttd_cmd_vec[] =
    {
        {"state", 1, _mqttd_cmd_enable,
         "mqttd state { enable [ server=<IP address> ] | disable }\n"},
        {"dump", 1, _mqttd_cmd_dump_topic,
         "mqttd dump topic\n"},
        {"debug", 1, _mqttd_cmd_debug,
         "mqttd debug { all | db | pkt | disable }\n"},
        {"show", 1, _mqttd_cmd_show_state,
         "mqttd show state\n"},
        {"encode", 1, _mqttd_cmd_coding,
         "mqttd encode { enable | disable }\n"},
        {"http", 1, _mqttd_http_test_json,
         "mqttd http { 1 | 2 | 3 | 4 }\n"},
        {"json", 1, _mqttd_cmd_json,
         "mqttd json { enable | disable }\n"},
};

/* STATIC VARIABLE DECLARATIONS
 */

/* LOCAL SUBPROGRAM BODIES
 */
/* cmd: mqttd state { enable [server=<IPv4 address>] | disable }
 */
static MW_ERROR_NO_T
_mqttd_cmd_enable(
    const C8_T *tokens[],
    UI32_T token_idx)
{
    MW_ERROR_NO_T ret = MW_E_OK;
    UI8_T enable;
    MW_IPV4_T value = 0;

    /* Parser tokens */
    if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "enable"))
    {
        token_idx++;
        if (NULL != tokens[token_idx])
        {
            if (MW_E_OK == mw_cmd_getIpv4Addr(tokens, token_idx, "server", &value))
            {
                value = ntohl(value);
                osapi_printf("Server IP is: %s(%ul)\n", tokens[token_idx + 1], value);
                if ((IPADDR_ANY == value) ||
                    (IPADDR_LOOPBACK == value) ||
                    (IPADDR_BROADCAST == value) ||
                    (MW_IPV4_IS_MULTICAST(value)) ||
                    (IPADDR_NONE == value))
                {
                    osapi_printf("Invalid IP address of MQTT remoter server.\n");
                    return MW_E_BAD_PARAMETER;
                }
                token_idx += 2;
            }
            MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        }
        enable = TRUE;
        osapi_printf("Start the MQTT daemon\n");
    }
    else if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "disable"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        enable = FALSE;
        osapi_printf("Stop the MQTT daemon\n");
    }
    else
    {
        return MW_E_BAD_PARAMETER;
    }
    sys_mgmt_mqttd_enable_cmd_set(enable, (void *)&value);

    return ret;
}

/* STATIC VARIABLE DECLARATIONS
 */

/* LOCAL SUBPROGRAM BODIES
 */
/* cmd: mqttd encode { enable | disable }
 */
static MW_ERROR_NO_T
_mqttd_cmd_coding(
    const C8_T *tokens[],
    UI32_T token_idx)
{
    MW_ERROR_NO_T ret = MW_E_OK;
    UI8_T enable;

    /* Parser tokens */
    if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "enable"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        enable = 1;
        osapi_printf("Enable the MQTT coding\n");
    }
    else if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "disable"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        enable = 0;
        osapi_printf("Disable the MQTT coding\n");
    }
    else
    {
        return MW_E_BAD_PARAMETER;
    }

    return ret;
}

/* STATIC VARIABLE DECLARATIONS
 */

/* LOCAL SUBPROGRAM BODIES
 */
/* cmd: mqttd json { enable | disable }
 */
static MW_ERROR_NO_T
_mqttd_cmd_json(
    const C8_T *tokens[],
    UI32_T token_idx)
{
    MW_ERROR_NO_T ret = MW_E_OK;
    UI8_T enable;

    /* Parser tokens */
    if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "enable"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        enable = 1;
        osapi_printf("Enable the MQTT json dump\n");
    }
    else if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "disable"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        enable = 0;
        osapi_printf("Disable the MQTT json dump\n");
    }
    else
    {
        return MW_E_BAD_PARAMETER;
    }

    return ret;
}

/* cmd: mqttd dump topic
 */
static MW_ERROR_NO_T
_mqttd_cmd_dump_topic(
    const C8_T *tokens[],
    UI32_T token_idx)
{
    MW_ERROR_NO_T ret = MW_E_OK;

    /* Check token len */
    if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "topic"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        mqttd_dump_topic();
    }

    return ret;
}

/* cmd: mqttd debug { all | db | pkt | disable }
 */
static MW_ERROR_NO_T
_mqttd_cmd_debug(
    const C8_T *tokens[],
    UI32_T token_idx)
{
    MW_ERROR_NO_T ret = MW_E_OK;

    /* Parser tokens */
    if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "all"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        osapi_printf("Enable All MQTT debug messages.\n");
        mqttd_debug_enable(MQTTD_DEBUG_ALL);
    }
    else if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "db"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        osapi_printf("Enable MQTT print db flow.\n");
        mqttd_debug_enable(MQTTD_DEBUG_DB);
    }
    else if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "pkt"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        osapi_printf("Enable MQTT print incoming topics and messages.\n");
        mqttd_debug_enable(MQTTD_DEBUG_PKT);
    }
    else if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "disable"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        osapi_printf("Disable MQTT debug messages.\n");
        mqttd_debug_enable(MQTTD_DEBUG_DISABLE);
    }
    else
    {
        return MW_E_BAD_PARAMETER;
    }

    return ret;
}

/* cmd: mqttd show state
 */
static MW_ERROR_NO_T
_mqttd_cmd_show_state(
    const C8_T *tokens[],
    UI32_T token_idx)
{
    MW_ERROR_NO_T ret = MW_E_OK;

    /* Parser tokens */
    if (MW_E_OK == mw_cmd_checkString(tokens[token_idx], "state"))
    {
        token_idx++;
        MW_CMD_CHECK_LAST_TOKEN(tokens[token_idx]);
        mqttd_show_state();
    }
    else
    {
        return MW_E_BAD_PARAMETER;
    }

    return ret;
}

/* EXPORTED SUBPROGRAM BODIES
 */
/* FUNCTION NAME: mw_cmd_mqttd_dispatcher
 * PURPOSE:
 *      Function dispatcher for magic wand command: MQTTD.
 *
 * INPUT:
 *      tokens      --  Command tokens
 *      token_idx   --  The index of 1st valid token
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
mw_cmd_mqttd_dispatcher(
    const C8_T *tokens[],
    UI32_T token_idx)
{
    return (mw_cmd_dispatcher(tokens, token_idx, _mw_mqttd_cmd_vec, sizeof(_mw_mqttd_cmd_vec) / sizeof(MW_CMD_VEC_T)));
}

err_t _mqttd11_test_recv(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    mqttd_debug("-------------------------------------------------\n");
    mqttd_debug("-------------------------------------------------\n");
    mqttd_debug("-------------------------------------------------\n");
    mqttd_debug("-------------------------------------------------\n");
    mqttd_debug("-------------------------------------------------\n");
    return MW_E_OK;
}

/* HTTP客户端接收到数据后的数据处理回调函数 */
static err_t HTTPClientCallback(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err)
{
    struct pbuf *tcp_send_pbuf;
    char echoString[] = "GET https://www.cnblogs.com/foxclever/ HTTP/1.1\r\n"

                        "Host:www.cnblogs.com:80\r\n\r\n";

    if (tcp_recv_pbuf != NULL)
    {
        /* 更新接收窗口 */
        tcp_recved(pcb, tcp_recv_pbuf->tot_len);

        /* 将接收到的服务器内容回显*/
        tcp_write(pcb, echoString, strlen(echoString), 1);
        tcp_send_pbuf = tcp_recv_pbuf;
        tcp_write(pcb, tcp_send_pbuf->payload, tcp_send_pbuf->len, 1);

        pbuf_free(tcp_recv_pbuf);
    }
    else if (err == ERR_OK)
    {
        tcp_close(pcb);
        return ERR_OK;
    }

    return ERR_OK;
}

/* HTTP客户端连接服务器错误回调函数 */
static void HTTPClientConnectError(void *arg, err_t err)
{
    /* 重新启动连接 */
    sys_httpc_debug("http client connect error %d!\n", 1);
}

/* HTTP客户端连接到服务器回调函数 */
static err_t HTTPClientConnected(void *arg, struct tcp_pcb *pcb, err_t err)
{
    char clientString[] = "GET https://www.cnblogs.com/foxclever/ HTTP/1.1\r\n"

                          "Host:www.cnblogs.com:80\r\n\r\n";

    sys_httpc_debug("http client connected %s\n", clientString);

    /* 配置接收回调函数 */
    tcp_recv(pcb, HTTPClientCallback);

    /* 发送一个建立连接的问候字符串*/
    tcp_write(pcb, clientString, strlen(clientString), 0);

    return ERR_OK;
}

void _mqtt_http_test_tcp_send(u32_t addr)
{
#define TCP_HTTP_CLIENT_PORT 9090
#define TCP_HTTP_SERVER_PORT 8080
    /* HTTP客户端初始化配置*/
    struct tcp_pcb *tcp_client_pcb;
    ip_addr_t ipaddr;

    /* 将目标服务器的IP写入一个结构体，为pc机本地连接IP地址 */
    osapi_memcpy(&ipaddr, &addr, sizeof(ipaddr));

    /* 为tcp客户端分配一个tcp_pcb结构体    */
    tcp_client_pcb = tcp_new();

    /* 绑定本地端号和IP地址 */
    tcp_bind(tcp_client_pcb, IP_ADDR_ANY, TCP_HTTP_CLIENT_PORT);

    if (tcp_client_pcb != NULL)
    {
        /* 与目标服务器进行连接，参数包括了目标端口和目标IP */
        tcp_connect(tcp_client_pcb, &ipaddr, TCP_HTTP_SERVER_PORT, HTTPClientConnected);

        tcp_err(tcp_client_pcb, HTTPClientConnectError);
        tcp_close(tcp_client_pcb);
        osapi_printf("http client connect close\n");
    }
}

void _mqtt_http_test_get_file()
{
    int rc;
    ip_addr_t server_addr = {0};
    u32_t addr = 0x0a00a8c0; // 0xc0a8000a;//ipaddr_addr("192.168.0.10");
    osapi_memcpy(&server_addr, &addr, sizeof(server_addr));
    mqttd_debug("Server address: %s\n", ipaddr_ntoa(&server_addr));

    u16_t port = 8080;
    char *uri = "http://192.168.0.10:8080/airRTOSSystem.img";
    httpc_connection_t settings = {0};
    memset(&settings, 0, sizeof(settings));
    settings.altcp_allocator = NULL;

    altcp_recv_fn recv_fn = _mqttd11_test_recv;
    void *callback_arg = NULL;
    httpc_state_t *connection = NULL;
    rc = httpc_get_file(&server_addr, port, uri, &settings, recv_fn, callback_arg, &connection);
    mqttd_debug("httpc_get_file return %d", rc);
}

void _mqtt_http_test_tcp_file()
{
    struct altcp_pcb *conn = NULL;
    int rc;
    err_t err;
    ip_addr_t server_addr = {0};
    u32_t addr = 0x6400a8c0; // 0xc0a8000a;//ipaddr_addr("192.168.0.10");
    osapi_memcpy(&server_addr, &addr, sizeof(server_addr));
    sys_httpc_debug("Server address: %s\n", ipaddr_ntoa(&server_addr));

    conn = altcp_tcp_new_ip_type(IP_GET_TYPE(&server_addr));
    if (NULL == conn)
    {
        sys_httpc_debug("altcp_tcp_new_ip_type error %d!\n", rc);
        return;
    }

    altcp_nagle_disable(conn);

    // altcp_arg(conn, client);
    /* Any local address, pick random local port number */
    err = altcp_bind(conn, IP_ADDR_ANY, 0);
    if (err != ERR_OK)
    {
        sys_httpc_debug("mqtt_http_client_connect: Error binding to local ip/port, %d\n", err);
        goto tcp_fail;
    }

    /* Connect to server */
    err = altcp_connect(conn, &server_addr, 8080, HTTPClientConnected);
    if (err != ERR_OK)
    {
        sys_httpc_debug("mqtt_client_connect: Error connecting to remote ip/port, %d failed!\n", err);
        goto tcp_fail;
    }

    /* Set error callback */
    altcp_err(conn, HTTPClientConnectError);

    sys_httpc_debug("mqtt_client_connect: Connected to server %d.\n", err);

    err = conn->connected(conn->arg, conn, ERR_OK);
    if(ERR_OK != err){
        sys_httpc_debug("mqtt_client_connect: Error connecting to remote ip/port, %d failed!\n", err);
        goto tcp_fail;
    }
    
    return;

tcp_fail:
    if (conn)
    {
        altcp_abort(conn);
        conn = NULL;
    }
    return;
}

#define SYS_HTTPC_TASK_NAME "httpc_task"
threadhandle_t sys_httpc_task_handle;
u32_t g_http_task_type = 0;

static void sys_httpc_task(void *ptr_pvParameters)
{
    sys_httpc_debug("start http test type %d\n", g_http_task_type);
    switch (g_http_task_type)
    {
    case 1:
        _mqtt_http_test_get_file();
        break;

    case 2:
    {
        // 192.168.0.10
        u32_t ip_client = atoi("10");
        u32_t addr = 0x0000a8c0 | (ip_client << 24);
        sys_httpc_debug("Client address: %s\n", ipaddr_ntoa(&addr));
        _mqtt_http_test_tcp_send(addr);
    }
    break;

    case 3:
    {
        // 192.168.0.100
        u32_t ip_client = atoi("100");
        u32_t addr = 0x0000a8c0 | (ip_client << 24);
        sys_httpc_debug("Client address: %s\n", ipaddr_ntoa(&addr));
        _mqtt_http_test_tcp_send(addr);
    }
    break;

    case 4:
    {
        _mqtt_http_test_tcp_file();
    }
    break;

    default:
        break;
    }

    // 释放资源
    osapi_threadDelete(sys_httpc_task_handle);
}

void _mqtt_httpc_thread_create()
{
    osapi_memset(&sys_httpc_task_handle, 0, sizeof(sys_httpc_task_handle));
    if (osapi_threadCreate(SYS_HTTPC_TASK_NAME,
                           configMINIMAL_STACK_SIZE * 2,
                           MW_TASK_PRIORITY_SYSMGMT,
                           sys_httpc_task,
                           NULL,
                           &sys_httpc_task_handle) != MW_E_OK)
    {
        osapi_printf("create httpc task failed!\n");
        osapi_threadDelete(sys_httpc_task_handle);
        return MW_E_NO_MEMORY;
    }
    return;
}
static MW_ERROR_NO_T
_mqttd_http_test_json(
    const C8_T *tokens[],
    UI32_T token_idx)
{
    MW_ERROR_NO_T ret = MW_E_OK;

    sys_httpc_debug("try http test\n");

    /* Parser tokens */
    g_http_task_type = atoi(tokens[token_idx]);
    sys_httpc_debug("try http test type %d\n", g_http_task_type);
    _mqtt_httpc_thread_create();
    return ret;
}

/* FUNCTION NAME: mw_cmd_mqttd_usager
 * PURPOSE:
 *      Command usage for magic wand command: MQTTD.
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
mw_cmd_mqttd_usager(
    void)
{
    return (mw_cmd_usager(_mw_mqttd_cmd_vec, sizeof(_mw_mqttd_cmd_vec) / sizeof(MW_CMD_VEC_T)));
}

#endif /* AIR_SUPPORT_MQTTD */
