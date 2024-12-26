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

#include "lwip/apps/http_client.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/altcp.h"
#include "lwip/ip_addr.h"
#include "lwip/altcp_tcp.h"

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
#include "mqttd_queue.h"
#include "mqttd_http.h"
#include "inet_utils.h"
#include "sys_mgmt.h"

#define sys_httpc_debug(fmt, ...)                                                          \
    do                                                                                     \
    {                                                                                      \
        osapi_printf("<%s:%d>(%s)" fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

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
#define USER_BUFFER_LENGTH (1024)

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

static int32_t connectToServer(NetworkContext_t *pNetworkContext, ServerInfo_t *serverInfo)
{
    int32_t returnStatus = EXIT_FAILURE;
    /* Status returned by plaintext sockets transport implementation. */
    SocketStatus_t socketStatus;

    /* Establish a TCP connection with the HTTP server. This example connects
     * to the HTTP server as specified in SERVER_HOST and HTTP_PORT
     * in demo_config.h. */

    socketStatus = Plaintext_Connect(pNetworkContext,
                                     serverInfo,
                                     TRANSPORT_SEND_RECV_TIMEOUT_MS,
                                     TRANSPORT_SEND_RECV_TIMEOUT_MS);

    if (socketStatus == SOCKETS_SUCCESS)
    {
        returnStatus = EXIT_SUCCESS;
    }
    else
    {
        sys_httpc_debug("Try Establishing a TCP connection with %s:%d failed!", serverInfo->pHostName, serverInfo->port);
        returnStatus = EXIT_FAILURE;
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int32_t sendHttpRequest(const TransportInterface_t *pTransportInterface,
                               const char *pMethod,
                               size_t methodLen, mqttd_http_t *mqttd_httpc)
{
    /* Return value of this method. */
    int32_t returnStatus = EXIT_SUCCESS;

    /* Configurations of the initial request headers that are passed to
     * #HTTPClient_InitializeRequestHeaders. */
    HTTPRequestInfo_t requestInfo = {};
    /* Represents a response returned from an HTTP server. */
    HTTPResponse_t response = {};
    /* Represents header data that will be sent in an HTTP request. */
    HTTPRequestHeaders_t requestHeaders = {};

    /* Return value of all methods from the HTTP Client library API. */
    HTTPStatus_t httpStatus = HTTPSuccess;

    if (pMethod == NULL || mqttd_httpc->http_path == NULL)
    {
        return EXIT_FAILURE;
    }

    /* Initialize all HTTP Client library API structs to 0. */
    (void)memset(&requestInfo, 0, sizeof(requestInfo));
    (void)memset(&response, 0, sizeof(response));
    (void)memset(&requestHeaders, 0, sizeof(requestHeaders));

    /* Initialize the request object. */
    requestInfo.pHost = mqttd_httpc->host;
    requestInfo.hostLen = mqttd_httpc->host_len;
    requestInfo.pMethod = pMethod;
    requestInfo.methodLen = methodLen;
    requestInfo.pPath = mqttd_httpc->http_path;
    requestInfo.pathLen = mqttd_httpc->http_path_len;

    /* Set "Connection" HTTP header to "keep-alive" so that multiple requests
     * can be sent over the same established TCP connection. */
    requestInfo.reqFlags = HTTP_REQUEST_KEEP_ALIVE_FLAG;

    /* Set the buffer used for storing request headers. */
    requestHeaders.pBuffer = resp_userBuffer;
    requestHeaders.bufferLen = USER_BUFFER_LENGTH;
    response.pBuffer = mqttd_httpc->response_buffer;
    response.bufferLen = mqttd_httpc->response_buffer_len;

    httpStatus = HTTPClient_InitializeRequestHeaders(&requestHeaders,
                                                     &requestInfo);

    if (httpStatus == HTTPSuccess)
    {
        /* Initialize the response object. The same buffer used for storing
         * request headers is reused here. */
        response.pBuffer = resp_userBuffer;
        response.bufferLen = USER_BUFFER_LENGTH;

        sys_httpc_debug("Sending HTTP %s request to %s %s...\n",
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
        sys_httpc_debug("Failed to initialize HTTP request headers: Error=%s.\n",
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
        sys_httpc_debug("Recevied HTTP response [%d] [%s] ...\n", response.bodyLen, response.pBody);
    }
    else
    {
        sys_httpc_debug("Failed to send HTTP %s request to %s%s: Error=%s.\n",
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

int mqttd_http_update(mqttd_http_t *mqttd_httpc)
{
    /* Information about the server to send the HTTP requests. */
    ServerInfo_t serverInfo = {};

    /* Initialize server information. */
    serverInfo.pHostName = mqttd_httpc->host;
    serverInfo.hostNameLength = mqttd_httpc->host_len;
    serverInfo.port = mqttd_httpc->http_port;

    /* Return value of main. */
    int32_t returnStatus = EXIT_SUCCESS;
    /* The transport layer interface used by the HTTP Client library. */
    TransportInterface_t transportInterface = {};
    /* The network context for the transport layer interface. */
    NetworkContext_t networkContext = {};
    PlaintextParams_t plaintextParams = {};

    /* Set the pParams member of the network context with desired transport. */
    networkContext.pParams = &plaintextParams;

    /**************************** Connect. ******************************/

    /* Establish TCP connection. */
    /* Attempt to connect to the HTTP server. If connection fails, retry after
     * a timeout. Timeout value will be exponentially increased till the maximum
     * attempts are reached or maximum timeout value is reached. The function
     * returns EXIT_FAILURE if the TCP connection cannot be established to
     * broker after configured number of attempts. */

    sys_httpc_debug("try connect \n");
    returnStatus = connectToServer(&networkContext, &serverInfo);
    sys_httpc_debug("try connect %d \n", returnStatus);
    if (returnStatus == EXIT_FAILURE)
    {
        /* Log error to indicate connection failure after all
         * reconnect attempts are over. */
        sys_httpc_debug("Failed to connect to HTTP server %s.", SERVER_HOST);
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

    if (returnStatus == EXIT_SUCCESS)
    {
        returnStatus = sendHttpRequest(&transportInterface, HTTP_METHOD_GET,
                                       HTTP_METHOD_GET_LENGTH,
                                       mqttd_httpc);
    }
    else
    {
    }

    if (returnStatus == EXIT_SUCCESS)
    {
        /* Log message indicating an iteration completed successfully. */
        sys_httpc_debug("Demo completed successfully.\n");
    }

    /************************** Disconnect. *****************************/

    /* Close TCP connection. */
    (void)Plaintext_Disconnect(&networkContext);

    sys_httpc_debug("Short delay before starting the next iteration....\n");

    return returnStatus;
}

int8_t g_http_task_status = -1;

int8_t mqttd_http_task_status_get()
{
    return g_http_task_status;
}

void mqttd_httpc_dump(mqttd_http_t *mqttd_httpc){
    osapi_printf("mqttd_httpc_dump: %s \n", mqttd_httpc->host);
    osapi_printf("  http_path: %s \n", mqttd_httpc->http_path);
    osapi_printf("  http_port: %d \n", mqttd_httpc->http_port);
    osapi_printf("  status: %d \n", mqttd_httpc->status);
}

static void _mqttd_http_main(void *ptr_pvParameters)
{
    //mqttd_http_t *mqttd_httpc = (mqttd_http_t *)ptr_pvParameters;
    osapi_printf("mqttd_http_main start.\n");
    mqttd_http_t *mqttd_httpc = NULL;
    g_http_task_status = -1;
    int ret = EXIT_SUCCESS;
    while(1) {
        mqttd_httpc_queue_recv(&mqttd_httpc);
        if(mqttd_httpc != NULL) {            
            mqttd_httpc_dump(mqttd_httpc);
            ret = mqttd_http_update(mqttd_httpc);
            osapi_printf("mqttd_http_update end %d.\n", mqttd_httpc->status);
            if (ret == EXIT_SUCCESS)
            {
                g_http_task_status = 0;
                mqttd_httpc->status = 0;
            }
            else
            {
                g_http_task_status = 1;
                mqttd_httpc->status = 1;
            }
            break;
        }
    }
    osapi_printf("mqttd_http_update done %d.\n", mqttd_httpc->status);
}

#define MQTTD_HTTPC_TASK_NAME "mqttd_httpc"
threadhandle_t mqttd_httpc_task_handle;
void mqttd_httpc_thread_create()
{
    osapi_memset(&mqttd_httpc_task_handle, 0, sizeof(mqttd_httpc_task_handle));
    if (osapi_threadCreate(MQTTD_HTTPC_TASK_NAME,
                           configMINIMAL_STACK_SIZE * 2,
                           MW_TASK_PRIORITY_SFP,
                           _mqttd_http_main,
                           NULL,
                           &mqttd_httpc_task_handle) != MW_E_OK)
    {
        osapi_printf("create httpc task failed!\n");
        osapi_threadDelete(mqttd_httpc_task_handle);
        return MW_E_NO_MEMORY;
    }
    return;
}

#endif /* AIR_SUPPORT_MQTTD */
