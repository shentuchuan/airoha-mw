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
#include "lwip/ip_addr.h"
#include "lwip/altcp_tcp.h"

/* POSIX includes. */
// #include <unistd.h>

/* Include Demo Config as the first non-system header. */
#include "demo_config.h"

/* Common HTTP demo utilities. */
#include "http_demo_utils.h"

/* HTTP API header. */
#include "core_http_client.h"

/* Plaintext sockets transport header. */
#include "plaintext_posix.h"

#ifdef AIR_SUPPORT_MQTTD
#include "mqttd.h"
#include "mqttd_http.h"
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
    mqttd_coding_enable(enable);
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
    mqttd_json_dump_enable(enable);
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

#define GET_PATH "/1.txt"
#define HEAD_PATH "/get"
#define PUT_PATH "/put"
#define POST_PATH "/post"

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define TRANSPORT_SEND_RECV_TIMEOUT_MS (5000)

/**
 * @brief The length in bytes of the user buffer.
 */
#define USER_BUFFER_LENGTH (2048)

/**
 * @brief Request body to send for PUT and POST requests in this demo.
 */
#define REQUEST_BODY "Hello, world!"
#define HTTP_PORT 8080
#define SERVER_HOST "192.168.0.100"

/* Check that hostname of the server is defined. */
#ifndef SERVER_HOST
#error "Please define a SERVER_HOST."
#endif

/* Check that port of the server is defined. */
#ifndef HTTP_PORT
#error "Please define a HTTP_PORT."
#endif

/* Check that a path for HTTP Method GET is defined. */
#ifndef GET_PATH
#error "Please define a GET_PATH."
#endif

/* Check that a path for HTTP Method HEAD is defined. */
#ifndef HEAD_PATH
#error "Please define a HEAD_PATH."
#endif

/* Check that a path for HTTP Method PUT is defined. */
#ifndef PUT_PATH
#error "Please define a PUT_PATH."
#endif

/* Check that a path for HTTP Method POST is defined. */
#ifndef POST_PATH
#error "Please define a POST_PATH."
#endif

/**
 * @brief Delay in seconds between each iteration of the demo.
 */
#define DEMO_LOOP_DELAY_SECONDS (5U)

/* Check that transport timeout for transport send and receive is defined. */
#ifndef TRANSPORT_SEND_RECV_TIMEOUT_MS
#define TRANSPORT_SEND_RECV_TIMEOUT_MS (1000)
#endif

/* Check that size of the user buffer is defined. */
#ifndef USER_BUFFER_LENGTH
#define USER_BUFFER_LENGTH (1024)
#endif

/**
 * @brief The length of the HTTP server host name.
 */
#define SERVER_HOST_LENGTH (sizeof(SERVER_HOST) - 1)

/**
 * @brief The length of the HTTP GET method.
 */
#define HTTP_METHOD_GET_LENGTH (sizeof(HTTP_METHOD_GET) - 1)

/**
 * @brief The length of the HTTP HEAD method.
 */
#define HTTP_METHOD_HEAD_LENGTH (sizeof(HTTP_METHOD_HEAD) - 1)

/**
 * @brief The length of the HTTP PUT method.
 */
#define HTTP_METHOD_PUT_LENGTH (sizeof(HTTP_METHOD_PUT) - 1)

/**
 * @brief The length of the HTTP POST method.
 */
#define HTTP_METHOD_POST_LENGTH (sizeof(HTTP_METHOD_POST) - 1)

/**
 * @brief The length of the HTTP GET path.
 */
#define GET_PATH_LENGTH (sizeof(GET_PATH) - 1)

/**
 * @brief The length of the HTTP HEAD path.
 */
#define HEAD_PATH_LENGTH (sizeof(HEAD_PATH) - 1)

/**
 * @brief The length of the HTTP PUT path.
 */
#define PUT_PATH_LENGTH (sizeof(PUT_PATH) - 1)

/**
 * @brief The length of the HTTP POST path.
 */
#define POST_PATH_LENGTH (sizeof(POST_PATH) - 1)

/**
 * @brief Length of the request body.
 */
#define REQUEST_BODY_LENGTH (sizeof(REQUEST_BODY) - 1)

/**
 * @brief Number of HTTP paths to request.
 */
#define NUMBER_HTTP_PATHS (1)

/**
 * @brief A pair containing a path string of the URI and its length.
 */
typedef struct httpPathStrings
{
    const char *httpPath;
    size_t httpPathLength;
} httpPathStrings_t;

/**
 * @brief A pair containing an HTTP method string and its length.
 */
typedef struct httpMethodStrings
{
    const char *httpMethod;
    size_t httpMethodLength;
} httpMethodStrings_t;

/**
 * @brief A buffer used in the demo for storing HTTP request headers and
 * HTTP response headers and body.
 *
 * @note This demo shows how the same buffer can be re-used for storing the HTTP
 * response after the HTTP request is sent out. However, the user can also
 * decide to use separate buffers for storing the HTTP request and response.
 */
static uint8_t resp_userBuffer[USER_BUFFER_LENGTH];

/*-----------------------------------------------------------*/

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    PlaintextParams_t *pParams;
};

/*-----------------------------------------------------------*/

/**
 * @brief Connect to HTTP server with reconnection retries.
 *
 * @param[out] pNetworkContext The output parameter to return the created network context.
 *
 * @return EXIT_FAILURE on failure; EXIT_SUCCESS on successful connection.
 */
static int32_t connectToServer(NetworkContext_t *pNetworkContext);

/**
 * @brief Send an HTTP request based on a specified method and path, then
 * print the response received from the server.
 *
 * @param[in] pTransportInterface The transport interface for making network calls.
 * @param[in] pMethod The HTTP request method.
 * @param[in] methodLen The length of the HTTP request method.
 * @param[in] pPath The Request-URI to the objects of interest.
 * @param[in] pathLen The length of the Request-URI.
 *
 * @return EXIT_FAILURE on failure; EXIT_SUCCESS on success.
 */
static int32_t sendHttpRequest(const TransportInterface_t *pTransportInterface,
                               const char *pMethod,
                               size_t methodLen,
                               const char *pPath,
                               size_t pathLen);

/*-----------------------------------------------------------*/

static int32_t connectToServer(NetworkContext_t *pNetworkContext)
{
    int32_t returnStatus = EXIT_FAILURE;
    /* Status returned by plaintext sockets transport implementation. */
    SocketStatus_t socketStatus;
    /* Information about the server to send the HTTP requests. */
    ServerInfo_t serverInfo;

    /* Initialize server information. */
    serverInfo.pHostName = SERVER_HOST;
    serverInfo.hostNameLength = SERVER_HOST_LENGTH;
    serverInfo.port = 8080;

    /* Establish a TCP connection with the HTTP server. This example connects
     * to the HTTP server as specified in SERVER_HOST and HTTP_PORT
     * in demo_config.h. */
    sys_httpc_debug("Try Establishing a TCP connection with %s:%d.",
                    SERVER_HOST,
                    HTTP_PORT);
    socketStatus = Plaintext_Connect(pNetworkContext,
                                     &serverInfo,
                                     TRANSPORT_SEND_RECV_TIMEOUT_MS,
                                     TRANSPORT_SEND_RECV_TIMEOUT_MS);

    if (socketStatus == SOCKETS_SUCCESS)
    {
        returnStatus = EXIT_SUCCESS;
    }
    else
    {
        returnStatus = EXIT_FAILURE;
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int32_t sendHttpRequest(const TransportInterface_t *pTransportInterface,
                               const char *pMethod,
                               size_t methodLen,
                               const char *pPath,
                               size_t pathLen)
{
    /* Return value of this method. */
    int32_t returnStatus = EXIT_SUCCESS;

    /* Configurations of the initial request headers that are passed to
     * #HTTPClient_InitializeRequestHeaders. */
    HTTPRequestInfo_t requestInfo;
    /* Represents a response returned from an HTTP server. */
    HTTPResponse_t response;
    /* Represents header data that will be sent in an HTTP request. */
    HTTPRequestHeaders_t requestHeaders;

    /* Return value of all methods from the HTTP Client library API. */
    HTTPStatus_t httpStatus = HTTPSuccess;

    if (pMethod == NULL || pPath == NULL)
    {
        return EXIT_FAILURE;
    }

    /* Initialize all HTTP Client library API structs to 0. */
    (void)memset(&requestInfo, 0, sizeof(requestInfo));
    (void)memset(&response, 0, sizeof(response));
    (void)memset(&requestHeaders, 0, sizeof(requestHeaders));

    /* Initialize the request object. */
    requestInfo.pHost = SERVER_HOST;
    requestInfo.hostLen = SERVER_HOST_LENGTH;
    requestInfo.pMethod = pMethod;
    requestInfo.methodLen = methodLen;
    requestInfo.pPath = pPath;
    requestInfo.pathLen = pathLen;

    /* Set "Connection" HTTP header to "keep-alive" so that multiple requests
     * can be sent over the same established TCP connection. */
    requestInfo.reqFlags = HTTP_REQUEST_KEEP_ALIVE_FLAG;

    /* Set the buffer used for storing request headers. */
    requestHeaders.pBuffer = resp_userBuffer;
    requestHeaders.bufferLen = USER_BUFFER_LENGTH;
    response.pBuffer = resp_userBuffer;
    response.bufferLen = USER_BUFFER_LENGTH;

    httpStatus = HTTPClient_InitializeRequestHeaders(&requestHeaders,
                                                     &requestInfo);

    if (httpStatus == HTTPSuccess)
    {
        /* Initialize the response object. The same buffer used for storing
         * request headers is reused here. */
        response.pBuffer = resp_userBuffer;
        response.bufferLen = USER_BUFFER_LENGTH;

        sys_httpc_debug("Sending HTTP %s request to %s %s...",
                        requestInfo.pMethod,
                        SERVER_HOST,
                        requestInfo.pPath);
        sys_httpc_debug("Request Headers:\n%s\n"
                        "Request Body:\n%s\n",
                        (char *)requestHeaders.pBuffer,
                        REQUEST_BODY);

        if (0 == strcmp(HTTP_METHOD_GET, pMethod))
        {
            httpStatus = HTTPClient_Send(pTransportInterface,
                                         &requestHeaders,
                                         (uint8_t *)"",
                                         0,
                                         &response,
                                         0);
        }
        else
            /* Send the request and receive the response. */
            httpStatus = HTTPClient_Send(pTransportInterface,
                                         &requestHeaders,
                                         (uint8_t *)REQUEST_BODY,
                                         REQUEST_BODY_LENGTH,
                                         &response,
                                         0);
    }
    else
    {
        sys_httpc_debug("Failed to initialize HTTP request headers: Error=%s.",
                        HTTPClient_strerror(httpStatus));
    }

    if (httpStatus == HTTPSuccess)
    {
        /*         sys_httpc_debug("Received HTTP response from %s %s ...\n"
                                "Response Headers:\n%s\n"
                                "Response Status:\n%u\n"
                                "Response Body:\n%s\n",
                                SERVER_HOST, requestInfo.pPath, response.pHeaders, response.statusCode, response.pBody);
         */
        // osapi_printf("Recevied HTTP bufferLen [%d] [%s] ...\n", requestHeaders.bufferLen, requestHeaders.pBuffer);
        // osapi_printf("Recevied HTTP header [%d] [%s] ...\n", requestHeaders.headersLen, requestHeaders.pBuffer + requestHeaders.headersLen);
        // sys_httpc_debug("Recevied HTTP response [%d] [%s] ...\n", response.bodyLen, response.pBody);
    }
    else
    {
        sys_httpc_debug("Failed to send HTTP %s request to %s%s: Error=%s.",
                        requestInfo.pMethod,
                        SERVER_HOST,
                        requestInfo.pPath,
                        HTTPClient_strerror(httpStatus));
    }

    if (httpStatus != HTTPSuccess)
    {
        returnStatus = EXIT_FAILURE;
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief Entry point of demo.
 *
 * This example resolves a domain, then establishes a TCP connection with an
 * HTTP server to demonstrate HTTP request/response communication without using
 * an encrypted channel (i.e. without TLS). After which, HTTP Client library API
 * is used to send a GET, HEAD, PUT, and POST request in that order. For each
 * request, the HTTP response from the server (or an error code) is logged.
 *
 * @note This example is single-threaded and uses statically allocated memory.
 *
 */
int http_client_main(int argc,
                     char **argv)
{
    /* Return value of main. */
    int32_t returnStatus = EXIT_SUCCESS;
    /* The transport layer interface used by the HTTP Client library. */
    TransportInterface_t transportInterface;
    /* The network context for the transport layer interface. */
    NetworkContext_t networkContext;
    PlaintextParams_t plaintextParams;
    /* An array of HTTP paths to request. */
    const httpPathStrings_t httpMethodPaths[] =
        {
            {GET_PATH, GET_PATH_LENGTH}
            /*{ HEAD_PATH, HEAD_PATH_LENGTH },
            { PUT_PATH,  PUT_PATH_LENGTH  },
            { POST_PATH, POST_PATH_LENGTH }*/
        };
    /* The respective method for the HTTP paths listed in #httpMethodPaths. */
    const httpMethodStrings_t httpMethods[] =
        {
            {HTTP_METHOD_GET, HTTP_METHOD_GET_LENGTH}
            /*{ HTTP_METHOD_HEAD, HTTP_METHOD_HEAD_LENGTH },
            { HTTP_METHOD_PUT,  HTTP_METHOD_PUT_LENGTH  },
            { HTTP_METHOD_POST, HTTP_METHOD_POST_LENGTH }*/
        };

    (void)argc;
    (void)argv;

    /* Set the pParams member of the network context with desired transport. */
    networkContext.pParams = &plaintextParams;

    for (;;)
    {
        int i = 0;

        /**************************** Connect. ******************************/

        /* Establish TCP connection. */
        if (returnStatus == EXIT_SUCCESS)
        {
            /* Attempt to connect to the HTTP server. If connection fails, retry after
             * a timeout. Timeout value will be exponentially increased till the maximum
             * attempts are reached or maximum timeout value is reached. The function
             * returns EXIT_FAILURE if the TCP connection cannot be established to
             * broker after configured number of attempts. */
            returnStatus = connectToServer(&networkContext);

            if (returnStatus == EXIT_FAILURE)
            {
                /* Log error to indicate connection failure after all
                 * reconnect attempts are over. */
                sys_httpc_debug("Failed to connect to HTTP server %.*s.",
                                (int32_t)SERVER_HOST_LENGTH,
                                SERVER_HOST);
            }
        }

        /* Define the transport interface. */
        if (returnStatus == EXIT_SUCCESS)
        {
            (void)memset(&transportInterface, 0, sizeof(transportInterface));
            transportInterface.recv = Plaintext_Recv;
            transportInterface.send = Plaintext_Send;
            transportInterface.pNetworkContext = &networkContext;
        }

        /********************** Send HTTPS requests. ************************/

        for (i = 0; i < NUMBER_HTTP_PATHS; i++)
        {
            if (returnStatus == EXIT_SUCCESS)
            {
                returnStatus = sendHttpRequest(&transportInterface,
                                               httpMethods[i].httpMethod,
                                               httpMethods[i].httpMethodLength,
                                               httpMethodPaths[i].httpPath,
                                               httpMethodPaths[i].httpPathLength);
            }
            else
            {
                break;
            }
        }

        if (returnStatus == EXIT_SUCCESS)
        {
            /* Log message indicating an iteration completed successfully. */
            LogInfo(("Demo completed successfully."));
        }

        /************************** Disconnect. *****************************/

        /* Close TCP connection. */
        (void)Plaintext_Disconnect(&networkContext);

        break; /* end test. */

        LogInfo(("Short delay before starting the next iteration....\n"));
        sleep(DEMO_LOOP_DELAY_SECONDS);
    }

    return returnStatus;
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
    if (ERR_OK != err)
    {
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
        http_client_main(0, NULL);
    }
    break;

    case 5:
    {
        int rc = 0;
        char host[] = "192.168.0.100";
        char http_path[] = "/1.txt";
        mqttd_http_t mqttd_httpc = {0};
        char buff[1024] = {0};
        mqttd_httpc.http_port = 8080;
        mqttd_httpc.host = host;
        mqttd_httpc.host_len = strlen(mqttd_httpc.host);
        mqttd_httpc.http_path = http_path;
        mqttd_httpc.http_path_len = strlen(mqttd_httpc.http_path);
        mqttd_httpc.response_buffer = buff;
        mqttd_httpc.response_buffer = sizeof(buff);
        rc = mqttd_http_update(&mqttd_httpc);
        if (MW_E_OK != rc)
        {
            mqttd_debug("mqttd_http_update failed!\n");
        }
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
