/**
 * @file
 * LWIP HTTP server implementation
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *         Simon Goldschmidt
 *
 */

/**
 * @defgroup httpd HTTP server
 * @ingroup apps
 *
 * This httpd supports for a
 * rudimentary server-side-include facility which will replace tags of the form
 * <!--#tag--> in any file whose extension is .shtml, .shtm or .ssi with
 * strings provided by an include handler whose pointer is provided to the
 * module via function http_set_ssi_handler().
 * Additionally, a simple common
 * gateway interface (CGI) handling mechanism has been added to allow clients
 * to hook functions to particular request URIs.
 *
 * To enable SSI support, define label LWIP_HTTPD_SSI in lwipopts.h.
 * To enable CGI support, define label LWIP_HTTPD_CGI in lwipopts.h.
 *
 * By default, the server assumes that HTTP headers are already present in
 * each file stored in the file system.  By defining LWIP_HTTPD_DYNAMIC_HEADERS in
 * lwipopts.h, this behavior can be changed such that the server inserts the
 * headers automatically based on the extension of the file being served.  If
 * this mode is used, be careful to ensure that the file system image used
 * does not already contain the header information.
 *
 * File system images without headers can be created using the makefsfile
 * tool with the -h command line option.
 *
 *
 * Notes about valid SSI tags
 * --------------------------
 *
 * The following assumptions are made about tags used in SSI markers:
 *
 * 1. No tag may contain '-' or whitespace characters within the tag name.
 * 2. Whitespace is allowed between the tag leadin "<!--#" and the start of
 *    the tag name and between the tag name and the leadout string "-->".
 * 3. The maximum tag name length is LWIP_HTTPD_MAX_TAG_NAME_LEN, currently 8 characters.
 *
 * Notes on CGI usage
 * ------------------
 *
 * The simple CGI support offered here works with GET method requests only
 * and can handle up to 16 parameters encoded into the URI. The handler
 * function may not write directly to the HTTP output but must return a
 * filename that the HTTP server will send to the browser as a response to
 * the incoming CGI request.
 *
 *
 *
 * The list of supported file types is quite short, so if makefsdata complains
 * about an unknown extension, make sure to add it (and its doctype) to
 * the 'g_psHTTPHeaders' list.
 */
#include "lwip/init.h"
#include "lwip/apps/httpd.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/apps/fs.h"
#include "httpd_structs.h"
#include "lwip/def.h"

#include "lwip/altcp.h"
#include "lwip/altcp_tcp.h"
#if HTTPD_ENABLE_HTTPS
#include "lwip/altcp_tls.h"
#include "mbedtls/certs.h"
#include <mbedtls/base64.h>
#include <arch/cc.h>
#endif
#include "web.h"

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif
#if LWIP_HTTPD_TIMING
#include "lwip/sys.h"
#endif /* LWIP_HTTPD_TIMING */

#include <string.h> /* memset */
#include <stdlib.h> /* atoi */
#include <stdio.h>
#include <stdarg.h>	/* va_list, va_arg() */

#if LWIP_TCP && LWIP_CALLBACK_API

/** Minimum length for a valid HTTP/0.9 request: "GET /\r\n" -> 7 bytes */
#define MIN_REQ_LEN   7

#define CRLF "\r\n"
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
#define HTTP11_CONNECTIONKEEPALIVE  "Connection: keep-alive"
#define HTTP11_CONNECTIONKEEPALIVE2 "Connection: Keep-Alive"
#endif

#if LWIP_HTTPD_DYNAMIC_FILE_READ
#define HTTP_IS_DYNAMIC_FILE(hs) ((hs)->buf != NULL)
#else
#define HTTP_IS_DYNAMIC_FILE(hs) 0
#endif

/* This defines checks whether tcp_write has to copy data or not */

#ifndef HTTP_IS_DATA_VOLATILE
/** tcp_write does not have to copy data when sent from rom-file-system directly */
#define HTTP_IS_DATA_VOLATILE(hs)       (HTTP_IS_DYNAMIC_FILE(hs) ? TCP_WRITE_FLAG_COPY : 0)
#endif
/** Default: dynamic headers are sent from ROM (non-dynamic headers are handled like file data) */
#ifndef HTTP_IS_HDR_VOLATILE
#define HTTP_IS_HDR_VOLATILE(hs, ptr)   0
#endif

/* Return values for http_send_*() */
#define HTTP_DATA_TO_SEND_FREED    3
#define HTTP_DATA_TO_SEND_BREAK    2
#define HTTP_DATA_TO_SEND_CONTINUE 1
#define HTTP_NO_DATA_TO_SEND       0

typedef struct {
	const char *name;
	u8_t shtml;
} default_filename;

#define HTTP_UPGRADE_PAGE_LEN               (13)
#define HTTP_RESTORE_PAGE_LEN               (17)
#define HTTP_MAX_UPGRADE_CONTEXT_LEN        (15)
#define LWIP_HTTPD_POST_MAX_BOUNDARY_LEN    (64)
typedef struct HTTP_UPGRADE_CONTEXT_T
{
    char desc[HTTP_MAX_UPGRADE_CONTEXT_LEN];
    char offt;
}HTTP_UPGRADE_CONTEXT_S;
/* GLOBAL VARIABLE DECLARATIONS
*/
static const default_filename httpd_default_filenames[] = {
	{"/index.shtml", 1 },
	{"/index.ssi",   1 },
	{"/index.shtm",  1 },
	{"/index.html",  1 },
	{"/index.htm",   0 }
};

enum HTTP_POST_UPGRADE_STAT
{
    HTTP_POST_UPGRADE_INIT_STAT = 0,
    HTTP_POST_UPGRADE_FILENAME_STAT,
    HTTP_POST_UPGRADE_CONTENTTYPE_STAT,
    HTTP_POST_UPGRADE_CRLFCRLF_STAT,
    HTTP_POST_UPGRADE_FILESIZE_H_STAT,
    HTTP_POST_UPGRADE_FILESIZE_L_STAT,
    HTTP_POST_UPGRADE_DATATRANS_STAT,
    HTTP_POST_UPGRADE_STAT_LAST
};

static HTTP_UPGRADE_CONTEXT_S http_upgrade_context[] =
{
    {"boundary=",    0},
    {"filename=",    0},
    {"Content-Type", 0},
    {"\r\n\r\n",     0},
    {"File-Size: ",  0},
    {"file-size: ",  0}
};

unsigned int _chunkSize = LWIP_HTTPD_MAX_SESSION_LEN;
unsigned char http_cookie_index = 0;
HTTP_COOKIE_INFO_T http_cookies[HTTPD_MAX_LOGIN_NUM];
static const char common_xml[] = {"/common.xml"};
#ifdef AIR_SUPPORT_MQTTD
SemaphoreHandle_t cgiMutex = NULL;
#endif

/* LOCAL SUBPROGRAM DECLARATIONS
 */
BOOL_T
_http_find_cookie(
    char *ptr_data);

unsigned char
_httpd_get_empty_cookie_index(
    void);

int
_httpd_generate_cookies(
    char *ptr_outBuf,
    int *ptr_outLen);


#define NUM_DEFAULT_FILENAMES LWIP_ARRAYSIZE(httpd_default_filenames)

#if LWIP_HTTPD_SUPPORT_REQUESTLIST
/** HTTP request is copied here from pbufs for simple parsing */
static char httpd_req_buf[LWIP_HTTPD_MAX_REQ_LENGTH + 1];
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */

#if LWIP_HTTPD_SUPPORT_POST
#if LWIP_HTTPD_POST_MAX_RESPONSE_URI_LEN > LWIP_HTTPD_MAX_REQUEST_URI_LEN
#define LWIP_HTTPD_URI_BUF_LEN LWIP_HTTPD_POST_MAX_RESPONSE_URI_LEN
#endif
#endif
#ifndef LWIP_HTTPD_URI_BUF_LEN
#define LWIP_HTTPD_URI_BUF_LEN LWIP_HTTPD_MAX_REQUEST_URI_LEN
#endif
#if LWIP_HTTPD_URI_BUF_LEN
/* Filename for response file to send when POST is finished or
 * search for default files when a directory is requested. */
static char http_uri_buf[LWIP_HTTPD_URI_BUF_LEN + 1];
#endif

#if LWIP_HTTPD_DYNAMIC_HEADERS
/* The number of individual strings that comprise the headers sent before each
 * requested file.
 */
#define NUM_FILE_HDR_STRINGS 5
#define HDR_STRINGS_IDX_HTTP_STATUS           0 /* e.g. "HTTP/1.0 200 OK\r\n" */
#define HDR_STRINGS_IDX_SERVER_NAME           1 /* e.g. "Server: "HTTPD_SERVER_AGENT"\r\n" */
#define HDR_STRINGS_IDX_CONTENT_LEN_KEEPALIVE 2 /* e.g. "Content-Length: xy\r\n" and/or "Connection: keep-alive\r\n" */
#define HDR_STRINGS_IDX_CONTENT_LEN_NR        3 /* the byte count, when content-length is used */
#define HDR_STRINGS_IDX_CONTENT_TYPE          4 /* the content type (or default answer content type including default document) */

/* The dynamically generated Content-Length buffer needs space for CRLF + NULL */
#define LWIP_HTTPD_MAX_CONTENT_LEN_OFFSET 3
#ifndef LWIP_HTTPD_MAX_CONTENT_LEN_SIZE
/* The dynamically generated Content-Length buffer shall be able to work with
   ~953 MB (9 digits) */
#define LWIP_HTTPD_MAX_CONTENT_LEN_SIZE   (9 + LWIP_HTTPD_MAX_CONTENT_LEN_OFFSET)
#endif
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */

#if LWIP_HTTPD_SSI

#define HTTPD_LAST_TAG_PART 0xFFFF

enum tag_check_state {
	TAG_NONE,       /* Not processing an SSI tag */
	TAG_LEADIN,     /* Tag lead in "<!--#" being processed */
	TAG_FOUND,      /* Tag name being read, looking for lead-out start */
	TAG_LEADOUT,    /* Tag lead out "-->" being processed */
	TAG_SENDING     /* Sending tag replacement string */
};

struct http_ssi_state {
	const char *parsed;     /* Pointer to the first unparsed byte in buf. */
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
	const char *tag_started;/* Pointer to the first opening '<' of the tag. */
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG */
	const char *tag_end;    /* Pointer to char after the closing '>' of the tag. */
	u32_t parse_left; /* Number of unparsed bytes in buf. */
	u16_t tag_index;   /* Counter used by tag parsing state machine */
	/** AIR modification */
	u16_t tag_loopidx;   /* The index of the tag in loop */
	u16_t tag_insert_len; /* Length of insert in string tag_insert */
#if LWIP_HTTPD_SSI_MULTIPART
	u16_t tag_part; /* Counter passed to and changed by tag insertion function to insert multiple times */
#endif /* LWIP_HTTPD_SSI_MULTIPART */
	u8_t tag_type; /* index into http_ssi_tag_desc array */
	u8_t tag_name_len; /* Length of the tag name in string tag_name */
	char tag_name[LWIP_HTTPD_MAX_TAG_NAME_LEN + 1]; /* Last tag name extracted */
	/** AIR modification */
	//char tag_insert[LWIP_HTTPD_MAX_TAG_INSERT_LEN + 1]; /* Insert string for tag_name */
	char *tag_insert; /* Insert string for tag_name */
	enum tag_check_state tag_state; /* State of the tag processor */
};

struct http_ssi_tag_description {
	const char *lead_in;
	const char *lead_out;
};
#endif /* LWIP_HTTPD_SSI */

struct http_state {
#if LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED
	struct http_state *next;
#endif /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */
	struct fs_file file_handle;
	struct fs_file *handle;
	const char *file;       /* Pointer to first unsent byte in buf. */

	struct altcp_pcb *pcb;
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
	struct pbuf *req;
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */

#if LWIP_HTTPD_DYNAMIC_FILE_READ
	char *buf;        /* File read buffer. */
	int buf_len;      /* Size of file read buffer, buf. */
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ */
	u32_t left;       /* Number of unsent bytes in buf. */
	u8_t retries;
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
	u8_t keepalive;
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
#if LWIP_HTTPD_SSI
	struct http_ssi_state *ssi;
#endif /* LWIP_HTTPD_SSI */
#if LWIP_HTTPD_CGI
	char *params[LWIP_HTTPD_MAX_CGI_PARAMETERS]; /* Params extracted from the request URI */
	char *param_vals[LWIP_HTTPD_MAX_CGI_PARAMETERS]; /* Values for each extracted param */
#endif /* LWIP_HTTPD_CGI */
#if LWIP_HTTPD_DYNAMIC_HEADERS
	const char *hdrs[NUM_FILE_HDR_STRINGS]; /* HTTP headers to be sent. */
	char hdr_content_len[LWIP_HTTPD_MAX_CONTENT_LEN_SIZE];
	u16_t hdr_pos;     /* The position of the first unsent header byte in the
                        current string */
	u16_t hdr_index;   /* The index of the hdr string currently being sent. */
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */
#if LWIP_HTTPD_TIMING
	u32_t time_started;
#endif /* LWIP_HTTPD_TIMING */
#if LWIP_HTTPD_SUPPORT_POST
	char *post_boundary_string;
	u8_t receive_state;
	u32_t post_content_len_left;
#if LWIP_HTTPD_POST_MANUAL_WND
	u32_t unrecved_bytes;
	u8_t no_auto_wnd;
	u8_t post_finished;
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
	u32_t cgi_handler_index;
	char *response_file;
#endif /* LWIP_HTTPD_SUPPORT_POST*/
};

#if HTTPD_USE_MEM_POOL
LWIP_MEMPOOL_DECLARE(HTTPD_STATE,     MEMP_NUM_PARALLEL_HTTPD_CONNS,     sizeof(struct http_state),     "HTTPD_STATE")
#if LWIP_HTTPD_SSI
LWIP_MEMPOOL_DECLARE(HTTPD_SSI_STATE, MEMP_NUM_PARALLEL_HTTPD_SSI_CONNS, sizeof(struct http_ssi_state), "HTTPD_SSI_STATE")
#define HTTP_FREE_SSI_STATE(x)  LWIP_MEMPOOL_FREE(HTTPD_SSI_STATE, (x))
#define HTTP_ALLOC_SSI_STATE()  (struct http_ssi_state *)LWIP_MEMPOOL_ALLOC(HTTPD_SSI_STATE)
#endif /* LWIP_HTTPD_SSI */
#define HTTP_ALLOC_HTTP_STATE() (struct http_state *)LWIP_MEMPOOL_ALLOC(HTTPD_STATE)
#define HTTP_FREE_HTTP_STATE(x) LWIP_MEMPOOL_FREE(HTTPD_STATE, (x))
#else /* HTTPD_USE_MEM_POOL */
#define HTTP_ALLOC_HTTP_STATE() (struct http_state *)mem_malloc(sizeof(struct http_state))
#define HTTP_FREE_HTTP_STATE(x) mem_free(x)
#if LWIP_HTTPD_SSI
#define HTTP_ALLOC_SSI_STATE()  (struct http_ssi_state *)mem_malloc(sizeof(struct http_ssi_state))
#define HTTP_FREE_SSI_STATE(x)  mem_free(x)
#endif /* LWIP_HTTPD_SSI */
#endif /* HTTPD_USE_MEM_POOL */

static err_t http_close_conn(struct altcp_pcb *pcb, struct http_state *hs);
static err_t http_close_or_abort_conn(struct altcp_pcb *pcb, struct http_state *hs, u8_t abort_conn);
static err_t http_find_file(struct http_state *hs, const char *uri, int is_09);
static err_t http_init_file(struct http_state *hs, struct fs_file *file, int is_09, const char *uri, u8_t tag_check, char *params);
static err_t http_poll(void *arg, struct altcp_pcb *pcb);
static u8_t http_check_eof(struct altcp_pcb *pcb, struct http_state *hs);
#if LWIP_HTTPD_FS_ASYNC_READ
static void http_continue(void *connection);
#endif /* LWIP_HTTPD_FS_ASYNC_READ */
unsigned int _needLen = 0;
unsigned int _rcvLen = 0;

#if LWIP_HTTPD_SSI
/* SSI insert handler function pointer. */
static tSSIHandler httpd_ssi_handler;
#if !LWIP_HTTPD_SSI_RAW
static int httpd_num_tags;
static const char **httpd_tags;
#endif /* !LWIP_HTTPD_SSI_RAW */

/* Define the available tag lead-ins and corresponding lead-outs.
 * ATTENTION: for the algorithm below using this array, it is essential
 * that the lead in differs in the first character! */
const struct http_ssi_tag_description http_ssi_tag_desc[] = {
	{"<!--#", "-->"},
	{"/*#", "*/"}
};

#endif /* LWIP_HTTPD_SSI */

#if LWIP_HTTPD_CGI
/* CGI handler information */
static const tCGI *httpd_cgis;
static int httpd_num_cgis;
static int http_cgi_paramcount;
#define http_cgi_params     hs->params
#define http_cgi_param_vals hs->param_vals
#elif LWIP_HTTPD_CGI_SSI
static char *http_cgi_params[LWIP_HTTPD_MAX_CGI_PARAMETERS]; /* Params extracted from the request URI */
static char *http_cgi_param_vals[LWIP_HTTPD_MAX_CGI_PARAMETERS]; /* Values for each extracted param */
#endif /* LWIP_HTTPD_CGI */

#if LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED
/** global list of active HTTP connections, use to kill the oldest when
    running out of memory */
static struct http_state *http_connections;

static void
http_add_connection(struct http_state *hs)
{
  /* add the connection to the list */
	hs->next = http_connections;
	http_connections = hs;
}

static void
http_remove_connection(struct http_state *hs)
{
	/* take the connection off the list */
	if (http_connections) {
		if (http_connections == hs) {
			http_connections = hs->next;
		} else {
			struct http_state *last;
			for (last = http_connections; last->next != NULL; last = last->next) {
				if (last->next == hs) {
					last->next = hs->next;
					break;
				}
			}
		}
	}
}

static void
http_kill_oldest_connection(u8_t ssi_required)
{
	struct http_state *hs = http_connections;
	struct http_state *hs_free_next = NULL;
	while (hs && hs->next) {
#if LWIP_HTTPD_SSI
		if (ssi_required) {
			if (hs->next->ssi != NULL) {
				hs_free_next = hs;
			}
		} else
#else /* LWIP_HTTPD_SSI */
		LWIP_UNUSED_ARG(ssi_required);
#endif /* LWIP_HTTPD_SSI */
		{
			hs_free_next = hs;
		}
		LWIP_ASSERT("broken list", hs != hs->next);
		hs = hs->next;
	}
	if (hs_free_next != NULL) {
		LWIP_ASSERT("hs_free_next->next != NULL", hs_free_next->next != NULL);
		LWIP_ASSERT("hs_free_next->next->pcb != NULL", hs_free_next->next->pcb != NULL);
		/* send RST when killing a connection because of memory shortage */
		http_close_or_abort_conn(hs_free_next->next->pcb, hs_free_next->next, 1); /* this also unlinks the http_state from the list */
	}
}
#else /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */

#define http_add_connection(hs)
#define http_remove_connection(hs)

#endif /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */

#if LWIP_HTTPD_SSI
/** Allocate as struct http_ssi_state. */
static struct http_ssi_state *
http_ssi_state_alloc(void)
{
	struct http_ssi_state *ret = HTTP_ALLOC_SSI_STATE();
#if LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED
	if (ret == NULL) {
		http_kill_oldest_connection(1);
		ret = HTTP_ALLOC_SSI_STATE();
	}
#endif /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */
	if (ret != NULL) {
		memset(ret, 0, sizeof(struct http_ssi_state));
	}
	return ret;
}

/** Free a struct http_ssi_state. */
static void
http_ssi_state_free(struct http_ssi_state *ssi)
{
	if (ssi != NULL) {
		HTTP_FREE_SSI_STATE(ssi);
	}
}
#endif /* LWIP_HTTPD_SSI */

/** Initialize a struct http_state.
 */
static void
http_state_init(struct http_state *hs)
{
	/* Initialize the structure. */
	memset(hs, 0, sizeof(struct http_state));
#if LWIP_HTTPD_DYNAMIC_HEADERS
	/* Indicate that the headers are not yet valid */
	hs->hdr_index = NUM_FILE_HDR_STRINGS;
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */
}

/** Allocate a struct http_state. */
static struct http_state *
http_state_alloc(void)
{
	struct http_state *ret = HTTP_ALLOC_HTTP_STATE();
#if LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED
	if (ret == NULL) {
		http_kill_oldest_connection(0);
		ret = HTTP_ALLOC_HTTP_STATE();
	}
#endif /* LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED */
	if (ret != NULL) {
		http_state_init(ret);
		http_add_connection(ret);
	}
	return ret;
}

extern char *data;
/** Free a struct http_state.
 * Also frees the file data if dynamic.
 */
static void
http_state_eof(struct http_state *hs)
{
	if (hs->handle) {
#if LWIP_HTTPD_TIMING
		u32_t ms_needed = sys_now() - hs->time_started;
		u32_t needed = LWIP_MAX(1, (ms_needed / 100));
		LWIP_DEBUGF(HTTPD_DEBUG_TIMING, ("httpd: needed %"U32_F" ms to send file of %d bytes -> %"U32_F" bytes/sec\n",
			ms_needed, hs->handle->len, ((((u32_t)hs->handle->len) * 10) / needed)));
#endif /* LWIP_HTTPD_TIMING */
		fs_close(hs->handle);
		hs->handle = NULL;
	}
#if LWIP_HTTPD_DYNAMIC_FILE_READ
	if (hs->buf != NULL) {
		mem_free(hs->buf);
		hs->buf = NULL;
	}
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ */
#if LWIP_HTTPD_SSI
	if (hs->ssi) {
		http_ssi_state_free(hs->ssi);
		hs->ssi = NULL;
	}
#endif /* LWIP_HTTPD_SSI */
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
	if (hs->req) {
		pbuf_free(hs->req);
		hs->req = NULL;
	}
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
}

/** Free a struct http_state.
 * Also frees the file data if dynamic.
 */
static void
http_state_free(struct http_state *hs)
{
	if (hs != NULL) {
#if LWIP_HTTPD_SUPPORT_POST
        if (hs->post_boundary_string != NULL)
        {
            mem_free(hs->post_boundary_string);
        }
#endif
		http_state_eof(hs);
		http_remove_connection(hs);
		HTTP_FREE_HTTP_STATE(hs);
		if(data)
		{
			vPortFree(data);
			data = NULL;
		}
	}
}

/** Call tcp_write() in a loop trying smaller and smaller length
 *
 * @param pcb altcp_pcb to send
 * @param ptr Data to send
 * @param length Length of data to send (in/out: on return, contains the
 *        amount of data sent)
 * @param apiflags directly passed to tcp_write
 * @return the return value of tcp_write
 */
static err_t
http_write(struct altcp_pcb *pcb, const void *ptr, u16_t *length, u8_t apiflags)
{
	u16_t len, max_len;
	err_t err;
	LWIP_ASSERT("length != NULL", length != NULL);
	len = *length;
	if (len == 0) {
		return ERR_OK;
	}
	/* We cannot send more data than space available in the send buffer. */
	max_len = altcp_sndbuf(pcb);
	if (max_len < len) {
		len = max_len;
	}
#ifdef HTTPD_MAX_WRITE_LEN
	/* Additional limitation: e.g. don't enqueue more than 2*mss at once */
	max_len = HTTPD_MAX_WRITE_LEN(pcb);
	if (len > max_len) {
		len = max_len;
	}
#endif /* HTTPD_MAX_WRITE_LEN */
	do {
		LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Trying to send %d bytes\n", len));
		err = altcp_write(pcb, ptr, len, apiflags);
		if (err == ERR_MEM) {
			if ((altcp_sndbuf(pcb) == 0) ||
				(altcp_sndqueuelen(pcb) >= TCP_SND_QUEUELEN)) {
				/* no need to try smaller sizes */
				len = 1;
			} else {
				len /= 2;
			}
			LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE,
				("Send failed, trying less (%d bytes)\n", len));
		}
	} while ((err == ERR_MEM) && (len > 1));

	if (err == ERR_OK) {
		LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Sent %d bytes\n", len));
		*length = len;
	} else {
		LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Send failed with err %d (\"%s\")\n", err, lwip_strerr(err)));
		*length = 0;
  }

#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
  /* ensure nagle is normally enabled (only disabled for persistent connections
     when all data has been enqueued but the connection stays open for the next
     request */
	altcp_nagle_enable(pcb);
#endif

	return err;
}

/**
 * The connection shall be actively closed (using RST to close from fault states).
 * Reset the sent- and recv-callbacks.
 *
 * @param pcb the tcp pcb to reset callbacks
 * @param hs connection state to free
 */
static err_t
http_close_or_abort_conn(struct altcp_pcb *pcb, struct http_state *hs, u8_t abort_conn)
{
	err_t err;
	LWIP_DEBUGF(HTTPD_DEBUG, ("Closing connection %p\n", (void *)pcb));

#if LWIP_HTTPD_SUPPORT_POST
	if (hs != NULL) {
		if ((hs->post_content_len_left != 0)
#if LWIP_HTTPD_POST_MANUAL_WND
			|| ((hs->no_auto_wnd != 0) && (hs->unrecved_bytes != 0))
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
		) {
			/* make sure the post code knows that the connection is closed */
			http_uri_buf[0] = 0;
			httpd_post_finished(hs, http_uri_buf, LWIP_HTTPD_URI_BUF_LEN);
		}
	}
#endif /* LWIP_HTTPD_SUPPORT_POST*/


	altcp_arg(pcb, NULL);
	altcp_recv(pcb, NULL);
	altcp_err(pcb, NULL);
	altcp_poll(pcb, NULL, 0);
	altcp_sent(pcb, NULL);
	if (hs != NULL) {
		http_state_free(hs);
	}

	if (abort_conn) {
		altcp_abort(pcb);
		return ERR_OK;
	}
	err = altcp_close(pcb);
	if (err != ERR_OK) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("Error %d closing %p\n", err, (void *)pcb));
		/* error closing, try again later in poll */
		altcp_poll(pcb, http_poll, HTTPD_POLL_INTERVAL);
	}
	return err;
}

/**
 * The connection shall be actively closed.
 * Reset the sent- and recv-callbacks.
 *
 * @param pcb the tcp pcb to reset callbacks
 * @param hs connection state to free
 */
static err_t
http_close_conn(struct altcp_pcb *pcb, struct http_state *hs)
{
	return http_close_or_abort_conn(pcb, hs, 0);
}

/** End of file: either close the connection (Connection: close) or
 * close the file (Connection: keep-alive)
 */
static void
http_eof(struct altcp_pcb *pcb, struct http_state *hs)
{
	/* HTTP/1.1 persistent connection? (Not supported for SSI) */
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
	if (hs->keepalive) {
		http_remove_connection(hs);

		http_state_eof(hs);
		http_state_init(hs);
		/* restore state: */
		hs->pcb = pcb;
		hs->keepalive = 1;
		http_add_connection(hs);
		/* ensure nagle doesn't interfere with sending all data as fast as possible: */
		altcp_nagle_disable(pcb);
	} else
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
	{
		http_close_conn(pcb, hs);
	}
}

#if LWIP_HTTPD_CGI || LWIP_HTTPD_CGI_SSI
/**
 * Extract URI parameters from the parameter-part of an URI in the form
 * "test.cgi?x=y" @todo: better explanation!
 * Pointers to the parameters are stored in hs->param_vals.
 *
 * @param hs http connection state
 * @param params pointer to the NULL-terminated parameter string from the URI
 * @return number of parameters extracted
 */
static int
extract_uri_parameters(struct http_state *hs, char *params)
{
	char *pair;
	char *equals;
	int loop;

	LWIP_UNUSED_ARG(hs);

	/* If we have no parameters at all, return immediately. */
	if (!params || (params[0] == '\0')) {
		return (0);
	}

	/* Get a pointer to our first parameter */
	pair = params;

	/* Parse up to LWIP_HTTPD_MAX_CGI_PARAMETERS from the passed string and ignore the
	 * remainder (if any) */
	for (loop = 0; (loop < LWIP_HTTPD_MAX_CGI_PARAMETERS) && pair; loop++) {

		/* Save the name of the parameter */
		http_cgi_params[loop] = pair;

		/* Remember the start of this name=value pair */
		equals = pair;

		/* Find the start of the next name=value pair and replace the delimiter
		 * with a 0 to terminate the previous pair string. */
		pair = strchr(pair, '&');
		if (pair) {
			*pair = '\0';
			pair++;
		} else {
			/* We didn't find a new parameter so find the end of the URI and
			 * replace the space with a '\0' */
			pair = strchr(equals, ' ');
			if (pair) {
				*pair = '\0';
			}

			/* Revert to NULL so that we exit the loop as expected. */
			pair = NULL;
		}

		/* Now find the '=' in the previous pair, replace it with '\0' and save
		 * the parameter value string. */
		equals = strchr(equals, '=');
		if (equals) {
			*equals = '\0';
			http_cgi_param_vals[loop] = equals + 1;
		} else {
			http_cgi_param_vals[loop] = NULL;
		}
	}

	return loop;
}
#endif /* LWIP_HTTPD_CGI || LWIP_HTTPD_CGI_SSI */

#if LWIP_HTTPD_SSI
/**
 * Insert a tag (found in an shtml in the form of "<!--#tagname-->" into the file.
 * The tag's name is stored in ssi->tag_name (NULL-terminated), the replacement
 * should be written to hs->tag_insert (up to a length of LWIP_HTTPD_MAX_TAG_INSERT_LEN).
 * The amount of data written is stored to ssi->tag_insert_len.
 *
 * @todo: return tag_insert_len - maybe it can be removed from struct http_state?
 *
 * @param hs http connection state
 */
static void
get_tag_insert(struct http_state *hs)
{
#if LWIP_HTTPD_SSI_RAW
	const char *tag;
#else /* LWIP_HTTPD_SSI_RAW */
	int tag;
#endif /* LWIP_HTTPD_SSI_RAW */
	size_t len;
	struct http_ssi_state *ssi;
#if LWIP_HTTPD_SSI_MULTIPART
	u16_t current_tag_part;
#endif /* LWIP_HTTPD_SSI_MULTIPART */

	LWIP_ASSERT("hs != NULL", hs != NULL);
	ssi = hs->ssi;
	LWIP_ASSERT("ssi != NULL", ssi != NULL);
#if LWIP_HTTPD_SSI_MULTIPART
	current_tag_part = ssi->tag_part;
	ssi->tag_part = HTTPD_LAST_TAG_PART;
#endif /* LWIP_HTTPD_SSI_MULTIPART */
#if LWIP_HTTPD_SSI_RAW
	tag = ssi->tag_name;
#endif

	if (httpd_ssi_handler
#if !LWIP_HTTPD_SSI_RAW
		&& httpd_tags && httpd_num_tags
#endif /* !LWIP_HTTPD_SSI_RAW */
	) {

		/* Find this tag in the list we have been provided. */
#if LWIP_HTTPD_SSI_RAW
		{
#else /* LWIP_HTTPD_SSI_RAW */
		for (tag = 0; tag < httpd_num_tags; tag++) {
			if (strcmp(ssi->tag_name, httpd_tags[tag]) == 0)
#endif /* LWIP_HTTPD_SSI_RAW */
			{
				/** AIR modification */
				ssi->tag_loopidx = tag;
				ssi->tag_insert_len = 1;
#if 0
				ssi->tag_insert_len = httpd_ssi_handler(tag, ssi->tag_insert,
					LWIP_HTTPD_MAX_TAG_INSERT_LEN
#if LWIP_HTTPD_SSI_MULTIPART
					, current_tag_part, &ssi->tag_part
#endif /* LWIP_HTTPD_SSI_MULTIPART */
#if LWIP_HTTPD_FILE_STATE
					, (hs->handle ? hs->handle->state : NULL)
#endif /* LWIP_HTTPD_FILE_STATE */
				);
#endif
#if LWIP_HTTPD_SSI_RAW
				if (ssi->tag_insert_len != HTTPD_SSI_TAG_UNKNOWN)
#endif /* LWIP_HTTPD_SSI_RAW */
				{
					return;
				}
			}
		}
	}

	/* If we drop out, we were asked to serve a page which contains tags that
	 * we don't have a handler for. Merely echo back the tags with an error
	 * marker. */
#define UNKNOWN_TAG1_TEXT "<b>***UNKNOWN TAG "
#define UNKNOWN_TAG1_LEN	18
#define UNKNOWN_TAG2_TEXT "***</b>"
#define UNKNOWN_TAG2_LEN	7
	len = LWIP_MIN(sizeof(ssi->tag_name), LWIP_MIN(strlen(ssi->tag_name),
				LWIP_HTTPD_MAX_TAG_INSERT_LEN - (UNKNOWN_TAG1_LEN + UNKNOWN_TAG2_LEN)));
	MEMCPY(ssi->tag_insert, UNKNOWN_TAG1_TEXT, UNKNOWN_TAG1_LEN);
	MEMCPY(&ssi->tag_insert[UNKNOWN_TAG1_LEN], ssi->tag_name, len);
	MEMCPY(&ssi->tag_insert[UNKNOWN_TAG1_LEN + len], UNKNOWN_TAG2_TEXT, UNKNOWN_TAG2_LEN);
	ssi->tag_insert[UNKNOWN_TAG1_LEN + len + UNKNOWN_TAG2_LEN] = 0;

	len = strlen(ssi->tag_insert);
	LWIP_ASSERT("len <= 0xffff", len <= 0xffff);
	ssi->tag_insert_len = (u16_t)len;
}
#endif /* LWIP_HTTPD_SSI */

#if LWIP_HTTPD_DYNAMIC_HEADERS
/**
 * Generate the relevant HTTP headers for the given filename and write
 * them into the supplied buffer.
 */
static void
get_http_headers(struct http_state *hs, const char *uri)
{
	size_t content_type;
	char *tmp;
	char *ext;
	char *vars;

	/* In all cases, the second header we send is the server identification
		so set it here. */
	hs->hdrs[HDR_STRINGS_IDX_SERVER_NAME] = g_psHTTPHeaderStrings[HTTP_HDR_SERVER];
	hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEEPALIVE] = NULL;
	hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_NR] = NULL;

	/* Is this a normal file or the special case we use to send back the
		default "404: Page not found" response? */
	if (uri == NULL) {
		hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_NOT_FOUND];
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
		if (hs->keepalive) {
			hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = g_psHTTPHeaderStrings[DEFAULT_404_HTML_PERSISTENT];
		} else
#endif
		{
			hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = g_psHTTPHeaderStrings[DEFAULT_404_HTML];
		}

		/* Set up to send the first header string. */
		hs->hdr_index = 0;
		hs->hdr_pos = 0;
		return;
	}
	/* We are dealing with a particular filename. Look for one other
      special case.  We assume that any filename with "404" in it must be
      indicative of a 404 server error whereas all other files require
      the 200 OK header. */
	if (strstr(uri, "404")) {
		hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_NOT_FOUND];
	} else if (strstr(uri, "400")) {
		hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_BAD_REQUEST];
	} else if (strstr(uri, "501")) {
		hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_NOT_IMPL];
	} else {
		hs->hdrs[HDR_STRINGS_IDX_HTTP_STATUS] = g_psHTTPHeaderStrings[HTTP_HDR_OK];
	}

	/* Determine if the URI has any variables and, if so, temporarily remove
		them. */
	vars = strchr(uri, '?');
	if (vars) {
		*vars = '\0';
	}

	/* Get a pointer to the file extension.	We find this by looking for the
		last occurrence of "." in the filename passed. */
	ext = NULL;
	tmp = strchr(uri, '.');
	while (tmp) {
		ext = tmp + 1;
		tmp = strchr(ext, '.');
	}
	if (ext != NULL) {
		/* Now determine the content type and add the relevant header for that. */
		for (content_type = 0; content_type < NUM_HTTP_HEADERS; content_type++) {
			/* Have we found a matching extension? */
			if (!lwip_stricmp(g_psHTTPHeaders[content_type].extension, ext)) {
				break;
			}
		}
	} else {
		content_type = NUM_HTTP_HEADERS;
	}

	/* Reinstate the parameter marker if there was one in the original URI. */
	if (vars) {
		*vars = '?';
	}

#if LWIP_HTTPD_OMIT_HEADER_FOR_EXTENSIONLESS_URI
	/* Does the URL passed have any file extension?	If not, we assume it
		is a special-case URL used for control state notification and we do
		not send any HTTP headers with the response. */
	if (!ext) {
		/* Force the header index to a value indicating that all headers
			have already been sent. */
		hs->hdr_index = NUM_FILE_HDR_STRINGS;
		return;
	}
#endif /* LWIP_HTTPD_OMIT_HEADER_FOR_EXTENSIONLESS_URI */
	/* Did we find a matching extension? */
	if (content_type < NUM_HTTP_HEADERS) {
		/* yes, store it */
		hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = g_psHTTPHeaders[content_type].content_type;
	} else if (!ext) {
		/* no, no extension found -> use binary transfer to prevent the browser adding '.txt' on save */
		hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = HTTP_HDR_APP;
	} else {
		/* No - use the default, plain text file type. */
		hs->hdrs[HDR_STRINGS_IDX_CONTENT_TYPE] = HTTP_HDR_DEFAULT_TYPE;
	}
	/* Set up to send the first header string. */
	hs->hdr_index = 0;
	hs->hdr_pos = 0;
}

/* Add content-length header? */
static void
get_http_content_length(struct http_state *hs)
{
	u8_t add_content_len = 0;

	LWIP_ASSERT("already been here?", hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEEPALIVE] == NULL);

	add_content_len = 0;
#if LWIP_HTTPD_SSI
	if (hs->ssi == NULL) /* @todo: get maximum file length from SSI */
#endif /* LWIP_HTTPD_SSI */
	{
		if ((hs->handle != NULL) && (hs->handle->flags & FS_FILE_FLAGS_HEADER_PERSISTENT)) {
			add_content_len = 1;
		}
	}
	if (add_content_len) {
		size_t len;
		lwip_itoa(hs->hdr_content_len, (size_t)LWIP_HTTPD_MAX_CONTENT_LEN_SIZE,
					hs->handle->len);
		len = strlen(hs->hdr_content_len);
		if (len <= LWIP_HTTPD_MAX_CONTENT_LEN_SIZE - LWIP_HTTPD_MAX_CONTENT_LEN_OFFSET) {
			SMEMCPY(&hs->hdr_content_len[len], CRLF, 3);
			hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_NR] = hs->hdr_content_len;
		} else {
			add_content_len = 0;
		}
	}
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
	if (add_content_len) {
		hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEEPALIVE] = g_psHTTPHeaderStrings[HTTP_HDR_KEEPALIVE_LEN];
	} else {
		hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEEPALIVE] = g_psHTTPHeaderStrings[HTTP_HDR_CONN_CLOSE];
		hs->keepalive = 0;
	}
#else /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
	if (add_content_len) {
		hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEEPALIVE] = g_psHTTPHeaderStrings[HTTP_HDR_CONTENT_LENGTH];
  }
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
}

/** Sub-function of http_send(): send dynamic headers
 *
 * @returns: - HTTP_NO_DATA_TO_SEND: no new data has been enqueued
 *           - HTTP_DATA_TO_SEND_CONTINUE: continue with sending HTTP body
 *           - HTTP_DATA_TO_SEND_BREAK: data has been enqueued, headers pending,
 *                                      so don't send HTTP body yet
 *           - HTTP_DATA_TO_SEND_FREED: http_state and pcb are already freed
 */
static u8_t
http_send_headers(struct altcp_pcb *pcb, struct http_state *hs)
{
	err_t err;
	u16_t len;
	u8_t data_to_send = HTTP_NO_DATA_TO_SEND;
	u16_t hdrlen, sendlen;

	if (hs->hdrs[HDR_STRINGS_IDX_CONTENT_LEN_KEEPALIVE] == NULL) {
		/* set up "content-length" and "connection:" headers */
		get_http_content_length(hs);
	}

	/* How much data can we send? */
	len = altcp_sndbuf(pcb);
	sendlen = len;

	while (len && (hs->hdr_index < NUM_FILE_HDR_STRINGS) && sendlen) {
		const void *ptr;
		u16_t old_sendlen;
		u8_t apiflags;
		/* How much do we have to send from the current header? */
		hdrlen = (u16_t)strlen(hs->hdrs[hs->hdr_index]);

		/* How much of this can we send? */
		sendlen = (len < (hdrlen - hs->hdr_pos)) ? len : (hdrlen - hs->hdr_pos);

		/* Send this amount of data or as much as we can given memory
		 * constraints. */
		ptr = (const void *)(hs->hdrs[hs->hdr_index] + hs->hdr_pos);
		old_sendlen = sendlen;
		apiflags = HTTP_IS_HDR_VOLATILE(hs, ptr);
		if (hs->hdr_index == HDR_STRINGS_IDX_CONTENT_LEN_NR) {
			/* content-length is always volatile */
			apiflags |= TCP_WRITE_FLAG_COPY;
		}
		if (hs->hdr_index < NUM_FILE_HDR_STRINGS - 1) {
			apiflags |= TCP_WRITE_FLAG_MORE;
		}
		err = http_write(pcb, ptr, &sendlen, apiflags);
		if ((err == ERR_OK) && (old_sendlen != sendlen)) {
			/* Remember that we added some more data to be transmitted. */
			data_to_send = HTTP_DATA_TO_SEND_CONTINUE;
		} else if (err != ERR_OK) {
			/* special case: http_write does not try to send 1 byte */
			sendlen = 0;
		}

		/* Fix up the header position for the next time round. */
		hs->hdr_pos += sendlen;
		len -= sendlen;

		/* Have we finished sending this string? */
		if (hs->hdr_pos == hdrlen) {
			/* Yes - move on to the next one */
			hs->hdr_index++;
			/* skip headers that are NULL (not all headers are required) */
			while ((hs->hdr_index < NUM_FILE_HDR_STRINGS) &&
					(hs->hdrs[hs->hdr_index] == NULL)) {
				hs->hdr_index++;
			}
			hs->hdr_pos = 0;
		}
	}

	if ((hs->hdr_index >= NUM_FILE_HDR_STRINGS) && (hs->file == NULL)) {
		/* When we are at the end of the headers, check for data to send
		 * instead of waiting for ACK from remote side to continue
		 * (which would happen when sending files from async read). */
		if (http_check_eof(pcb, hs)) {
			data_to_send = HTTP_DATA_TO_SEND_BREAK;
		} else {
			/* At this point, for non-keepalive connections, hs is deallocated an
				pcb is closed. */
			return HTTP_DATA_TO_SEND_FREED;
		}
	}
	/* If we get here and there are still header bytes to send, we send
	 * the header information we just wrote immediately. If there are no
	 * more headers to send, but we do have file data to send, drop through
	 * to try to send some file data too. */
	if ((hs->hdr_index < NUM_FILE_HDR_STRINGS) || !hs->file) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("tcp_output\n"));
		return HTTP_DATA_TO_SEND_BREAK;
	}
	return data_to_send;
}
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */

/** Sub-function of http_send(): end-of-file (or block) is reached,
 * either close the file or read the next block (if supported).
 *
 * @returns: 0 if the file is finished or no data has been read
 *           1 if the file is not finished and data has been read
 */
static u8_t
http_check_eof(struct altcp_pcb *pcb, struct http_state *hs)
{
	int bytes_left;
#if LWIP_HTTPD_DYNAMIC_FILE_READ
	int count;
#ifdef HTTPD_MAX_WRITE_LEN
	int max_write_len;
#endif /* HTTPD_MAX_WRITE_LEN */
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ */

	/* Do we have a valid file handle? */
	if (hs->handle == NULL) {
		/* No - close the connection. */
		http_eof(pcb, hs);
		return 0;
	}
	bytes_left = fs_bytes_left(hs->handle);
	if (bytes_left <= 0) {
		/* We reached the end of the file so this request is done. */
		LWIP_DEBUGF(HTTPD_DEBUG, ("End of file.\n"));
		http_eof(pcb, hs);
		return 0;
	}
#if LWIP_HTTPD_DYNAMIC_FILE_READ
	/* Do we already have a send buffer allocated? */
	if (hs->buf) {
		/* Yes - get the length of the buffer */
		count = LWIP_MIN(hs->buf_len, bytes_left);
	} else {
		/* We don't have a send buffer so allocate one now */
		count = altcp_sndbuf(pcb);
		if (bytes_left < count) {
			count = bytes_left;
		}
#ifdef HTTPD_MAX_WRITE_LEN
		/* Additional limitation: e.g. don't enqueue more than 2*mss at once */
		max_write_len = HTTPD_MAX_WRITE_LEN(pcb);
		if (count > max_write_len) {
			count = max_write_len;
		}
#endif /* HTTPD_MAX_WRITE_LEN */
		do {
			hs->buf = (char *)mem_malloc((mem_size_t)count);
			if (hs->buf != NULL) {
				hs->buf_len = count;
				break;
			}
			count = count / 2;
		} while (count > 100);

		/* Did we get a send buffer? If not, return immediately. */
		if (hs->buf == NULL) {
			LWIP_DEBUGF(HTTPD_DEBUG, ("No buff\n"));
			return 0;
		}
	}

	/* Read a block of data from the file. */
	LWIP_DEBUGF(HTTPD_DEBUG, ("Trying to read %d bytes.\n", count));

#if LWIP_HTTPD_FS_ASYNC_READ
	count = fs_read_async(hs->handle, hs->buf, count, http_continue, hs);
#else /* LWIP_HTTPD_FS_ASYNC_READ */
	count = fs_read(hs->handle, hs->buf, count);
#endif /* LWIP_HTTPD_FS_ASYNC_READ */
	if (count < 0) {
		if (count == FS_READ_DELAYED) {
			/* Delayed read, wait for FS to unblock us */
			return 0;
		}
		/* We reached the end of the file so this request is done.
		 * @todo: close here for HTTP/1.1 when reading file fails */
		LWIP_DEBUGF(HTTPD_DEBUG, ("End of file.\n"));
		http_eof(pcb, hs);
		return 0;
	}

	/* Set up to send the block of data we just read */
	LWIP_DEBUGF(HTTPD_DEBUG, ("Read %d bytes.\n", count));
	hs->left = count;
	hs->file = hs->buf;
#if LWIP_HTTPD_SSI
	if (hs->ssi) {
		hs->ssi->parse_left = count;
		hs->ssi->parsed = hs->buf;
	}
#endif /* LWIP_HTTPD_SSI */
#else /* LWIP_HTTPD_DYNAMIC_FILE_READ */
	LWIP_ASSERT("SSI and DYNAMIC_HEADERS turned off but eof not reached", 0);
#endif /* LWIP_HTTPD_SSI || LWIP_HTTPD_DYNAMIC_HEADERS */
	return 1;
}

char
send_format_response_no_chunk(
    unsigned int *ptr_length,
    struct tcp_pcb *ptr_pcb,
    unsigned int apiflags,
    const char *ptr_format, ...)
{
    char *ptr_buffer = NULL;
    char err = ERR_OK;
    u16_t len = 0;
    va_list args;

    ptr_buffer = pvPortMalloc(HTTPD_MAX_RESPONSE_BUFF_LEN, HTTPD_MEMORY_ALLOC_NAME);
    if(NULL != ptr_buffer)
    {
        va_start(args, ptr_format);
        vsprintf(ptr_buffer, ptr_format, args);
        va_end(args);

        len = strlen(ptr_buffer);
        err = http_write((struct altcp_pcb *)ptr_pcb, ptr_buffer, &len, apiflags);

        vPortFree(ptr_buffer);
        *ptr_length = len;
    }
    else
    {
        err = ERR_MEM;
    }
    return err;
}

char send_format_response(unsigned int *length, struct tcp_pcb *pcb, unsigned int apiflags, const char *format, ...)
{
    char *ptr_sendBuf = NULL;
    char *ptr_buffer = NULL;
    char err = ERR_OK;
    u16_t len = 0, tmplen = 0;
    va_list args;
    struct altcp_pcb *ptr_altcp = (struct altcp_pcb*)pcb;

    ptr_buffer = pvPortMalloc(HTTPD_MAX_RESPONSE_CHUNKBUFF_LEN, HTTPD_MEMORY_ALLOC_NAME);
    if(NULL == ptr_buffer)
    {
        return ERR_MEM;
    }

    va_start(args, format);
    vsprintf(ptr_buffer, format, args);
    va_end(args);

    len = strlen(ptr_buffer);
    ptr_sendBuf = pvPortMalloc((len + 8), HTTPD_MEMORY_ALLOC_NAME);
    if(NULL == ptr_sendBuf)
    {
        vPortFree(ptr_buffer);
        return ERR_MEM;
    }

#ifdef HTTP_TRANSFER_ENCODING_CHUNKED
    sprintf(ptr_sendBuf, "%x\r\n", len);
    tmplen = strlen(ptr_sendBuf);
#endif
    memcpy(&ptr_sendBuf[tmplen], ptr_buffer, len);
    tmplen += len;
#ifdef HTTP_TRANSFER_ENCODING_CHUNKED
    sprintf(&ptr_sendBuf[tmplen], "\r\n", 2);
    tmplen += 2;
#endif

    vPortFree(ptr_buffer);
    if((ptr_altcp->ssi_record_offt + tmplen) <= ptr_altcp->ssi_record_len)
    {
        ptr_altcp->ssi_record_offt += tmplen;
        tmplen = 0;
    }
    else
    {
        if(tmplen <= altcp_sndbuf(pcb))
        {
            err = http_write((struct altcp_pcb *)pcb, ptr_sendBuf, &tmplen, apiflags);
            if(ERR_OK == err)
            {
                ptr_altcp->ssi_record_len += tmplen;
                ptr_altcp->ssi_record_offt += tmplen;
            }
        }
        else
        {
            tmplen = 0;
            err = ERR_MEM;
        }
    }
    vPortFree(ptr_sendBuf);
    *length = tmplen;
    return err;
}

/** Sub-function of http_send(): This is the normal send-routine for non-ssi files
 *
 * @returns: - 1: data has been written (so call tcp_ouput)
 *           - 0: no data has been written (no need to call tcp_output)
 */
static u8_t
http_send_data_nonssi(struct altcp_pcb *pcb, struct http_state *hs)
{
	err_t err;
	u16_t len;
	u8_t data_to_send = 0;

	/* We are not processing an SHTML file so no tag checking is necessary.
	 * Just send the data as we received it from the file. */
	len = (u16_t)LWIP_MIN(hs->left, 0xffff);

	err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
	if (err == ERR_OK) {
		data_to_send = 1;
		hs->file += len;
		hs->left -= len;
	}

	return data_to_send;
}

#if LWIP_HTTPD_SSI
/** Sub-function of http_send(): This is the send-routine for ssi files
 *
 * @returns: - 1: data has been written (so call tcp_ouput)
 *	         - 0: no data has been written (no need to call tcp_output)
 */
static u8_t
http_send_data_ssi(struct altcp_pcb *pcb, struct http_state *hs)
{
	err_t err = ERR_OK;
	u16_t len;
	u8_t data_to_send = 0;
	u8_t tag_type;

	struct http_ssi_state *ssi = hs->ssi;
	LWIP_ASSERT("ssi != NULL", ssi != NULL);
	/* We are processing an SHTML file so need to scan for tags and replace
	 * them with insert strings. We need to be careful here since a tag may
	 * straddle the boundary of two blocks read from the file and we may also
	 * have to split the insert string between two tcp_write operations. */

	/* How much data could we send? */
	len = altcp_sndbuf(pcb);

	/* Do we have remaining data to send before parsing more? */
	if (ssi->parsed > hs->file) {
		len = (u16_t)LWIP_MIN(ssi->parsed - hs->file, 0xffff);

		err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
		if (err == ERR_OK) {
			data_to_send = 1;
			hs->file += len;
			hs->left -= len;
		}

		/* If the send buffer is full, return now. */
		if (altcp_sndbuf(pcb) == 0) {
			return data_to_send;
		}
	}

	LWIP_DEBUGF(HTTPD_DEBUG, ("State %d, %d left\n", ssi->tag_state, (int)ssi->parse_left));

	/* We have sent all the data that was already parsed so continue parsing
	 * the buffer contents looking for SSI tags. */
	while (((ssi->tag_state == TAG_SENDING) || ssi->parse_left) && (err == ERR_OK)) {
		if (len == 0) {
			return data_to_send;
		}

		switch (ssi->tag_state) {
			case TAG_NONE:
				/* We are not currently processing an SSI tag so scan for the
				 * start of the lead-in marker. */
				for (tag_type = 0; tag_type < LWIP_ARRAYSIZE(http_ssi_tag_desc); tag_type++) {
					if (*ssi->parsed == http_ssi_tag_desc[tag_type].lead_in[0]) {
						/* We found what could be the lead-in for a new tag so change
						 * state appropriately. */
						ssi->tag_type = tag_type;
						ssi->tag_state = TAG_LEADIN;
						ssi->tag_index = 1;
	#if !LWIP_HTTPD_SSI_INCLUDE_TAG
						ssi->tag_started = ssi->parsed;
	#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG */
						break;
					}
				}

				/* Move on to the next character in the buffer */
				ssi->parse_left--;
				ssi->parsed++;
				break;

			case TAG_LEADIN:
				/* We are processing the lead-in marker, looking for the start of
				 * the tag name. */

				/* Have we reached the end of the leadin? */
				if (http_ssi_tag_desc[ssi->tag_type].lead_in[ssi->tag_index] == 0) {
					ssi->tag_index = 0;
					ssi->tag_state = TAG_FOUND;
				} else {
					/* Have we found the next character we expect for the tag leadin? */
					if (*ssi->parsed == http_ssi_tag_desc[ssi->tag_type].lead_in[ssi->tag_index]) {
						/* Yes - move to the next one unless we have found the complete
						 * leadin, in which case we start looking for the tag itself */
						ssi->tag_index++;
					} else {
						/* We found an unexpected character so this is not a tag. Move
						 * back to idle state. */
						ssi->tag_state = TAG_NONE;
					}

					/* Move on to the next character in the buffer */
					ssi->parse_left--;
					ssi->parsed++;
				}
				break;

			case TAG_FOUND:
				/* We are reading the tag name, looking for the start of the
				 * lead-out marker and removing any whitespace found. */

				/* Remove leading whitespace between the tag leading and the first
				 * tag name character. */
				if ((ssi->tag_index == 0) && ((*ssi->parsed == ' ') ||
						(*ssi->parsed == '\t') || (*ssi->parsed == '\n') ||
						(*ssi->parsed == '\r'))) {
					/* Move on to the next character in the buffer */
					ssi->parse_left--;
					ssi->parsed++;
					break;
				}

				/* Have we found the end of the tag name? This is signalled by
				 * us finding the first leadout character or whitespace */
				if ((*ssi->parsed == http_ssi_tag_desc[ssi->tag_type].lead_out[0]) ||
						(*ssi->parsed == ' ')	|| (*ssi->parsed == '\t') ||
						(*ssi->parsed == '\n') || (*ssi->parsed == '\r')) {

					if (ssi->tag_index == 0) {
						/* We read a zero length tag so ignore it. */
						ssi->tag_state = TAG_NONE;
					} else {
						/* We read a non-empty tag so go ahead and look for the
						 * leadout string. */
						ssi->tag_state = TAG_LEADOUT;
						LWIP_ASSERT("ssi->tag_index <= 0xff", ssi->tag_index <= 0xff);
                        /* AIR modify: when tag_name_len != 0, use xml file name for tag name */
                        if(0 == ssi->tag_name_len)
                        {
    						ssi->tag_name_len = (u8_t)ssi->tag_index;
	    					ssi->tag_name[ssi->tag_index] = '\0';
                        }
						if (*ssi->parsed == http_ssi_tag_desc[ssi->tag_type].lead_out[0]) {
							ssi->tag_index = 1;
						} else {
							ssi->tag_index = 0;
						}
					}
				} else {
					/* This character is part of the tag name so save it */
					if (ssi->tag_index < LWIP_HTTPD_MAX_TAG_NAME_LEN) {
                        /* AIR modify: when tag_name_len != 0, use xml file name for tag name */
                        if(0 == ssi->tag_name_len)
                        {
    						ssi->tag_name[ssi->tag_index++] = *ssi->parsed;
                        }
                        else
                        {
                            ssi->tag_index++;
                        }
					} else {
						/* The tag was too long so ignore it. */
						ssi->tag_state = TAG_NONE;
					}
				}

				/* Move on to the next character in the buffer */
				ssi->parse_left--;
				ssi->parsed++;

				break;

			/* We are looking for the end of the lead-out marker. */
			case TAG_LEADOUT:
				/* Remove leading whitespace between the tag leading and the first
				 * tag leadout character. */
				if ((ssi->tag_index == 0) && ((*ssi->parsed == ' ') ||
						(*ssi->parsed == '\t') || (*ssi->parsed == '\n') ||
						(*ssi->parsed == '\r'))) {
					/* Move on to the next character in the buffer */
					ssi->parse_left--;
					ssi->parsed++;
					break;
				}

				/* Have we found the next character we expect for the tag leadout? */
				if (*ssi->parsed == http_ssi_tag_desc[ssi->tag_type].lead_out[ssi->tag_index]) {
					/* Yes - move to the next one unless we have found the complete
					 * leadout, in which case we need to call the client to process
					 * the tag. */

					/* Move on to the next character in the buffer */
					ssi->parse_left--;
					ssi->parsed++;
					ssi->tag_index++;

					if (http_ssi_tag_desc[ssi->tag_type].lead_out[ssi->tag_index] == 0) {
						/* Call the client to ask for the insert string for the
						 * tag we just found. */
#if LWIP_HTTPD_SSI_MULTIPART
						ssi->tag_part = 0; /* start with tag part 0 */
#endif /* LWIP_HTTPD_SSI_MULTIPART */
						get_tag_insert(hs);

						/* Next time through, we are going to be sending data
						 * immediately, either the end of the block we start
						 * sending here or the insert string. */
						ssi->tag_index = 0;
						ssi->tag_state = TAG_SENDING;
						ssi->tag_end = ssi->parsed;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
						ssi->parsed = ssi->tag_started;
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG*/

						/* If there is any unsent data in the buffer prior to the
						 * tag, we need to send it now. */
						if (ssi->tag_end > hs->file) {
							/* How much of the data can we send? */
#if LWIP_HTTPD_SSI_INCLUDE_TAG
							len = (u16_t)LWIP_MIN(ssi->tag_end - hs->file, 0xffff);
#else /* LWIP_HTTPD_SSI_INCLUDE_TAG*/
							/* we would include the tag in sending */
							len = (u16_t)LWIP_MIN(ssi->tag_started - hs->file, 0xffff);
#endif /* LWIP_HTTPD_SSI_INCLUDE_TAG*/

							err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
							if (err == ERR_OK) {
								data_to_send = 1;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
								if (ssi->tag_started <= hs->file) {
									/* pretend to have sent the tag, too */
									len += (u16_t)(ssi->tag_end - ssi->tag_started);
								}
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG*/
								hs->file += len;
								hs->left -= len;
							}
						}
					}
				} else {
					/* We found an unexpected character so this is not a tag. Move
					 * back to idle state. */
					ssi->parse_left--;
					ssi->parsed++;
					ssi->tag_state = TAG_NONE;
				}
				break;

			/*
			 * We have found a valid tag and are in the process of sending
			 * data as a result of that discovery. We send either remaining data
			 * from the file prior to the insert point or the insert string itself.
			 */
			case TAG_SENDING:
				/* Do we have any remaining file data to send from the buffer prior
				 * to the tag? */
				if (ssi->tag_end > hs->file) {
					/* How much of the data can we send? */
#if LWIP_HTTPD_SSI_INCLUDE_TAG
					len = (u16_t)LWIP_MIN(ssi->tag_end - hs->file, 0xffff);
#else /* LWIP_HTTPD_SSI_INCLUDE_TAG*/
					LWIP_ASSERT("hs->started >= hs->file", ssi->tag_started >= hs->file);
					/* we would include the tag in sending */
					len = (u16_t)LWIP_MIN(ssi->tag_started - hs->file, 0xffff);
#endif /* LWIP_HTTPD_SSI_INCLUDE_TAG*/
					if (len != 0) {
						err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
					} else {
						err = ERR_OK;
					}
					if (err == ERR_OK) {
						data_to_send = 1;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
						if (ssi->tag_started <= hs->file) {
							/* pretend to have sent the tag, too */
							len += (u16_t)(ssi->tag_end - ssi->tag_started);
						}
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG*/
						hs->file += len;
						hs->left -= len;
					}
				} else {
#if LWIP_HTTPD_SSI_MULTIPART
					if (ssi->tag_index >= ssi->tag_insert_len) {
						/* Did the last SSIHandler have more to send? */
						if (ssi->tag_part != HTTPD_LAST_TAG_PART) {
							/* If so, call it again */
							ssi->tag_index = 0;
							get_tag_insert(hs);
						}
					}
#endif /* LWIP_HTTPD_SSI_MULTIPART */

					/* Do we still have insert data left to send? */
					if (ssi->tag_insert_len/*ssi->tag_index < ssi->tag_insert_len*/) {
						/* We are sending the insert string itself. How much of the
						 * insert can we send? */
						/** AIR modification */
#if 0
						len = (ssi->tag_insert_len - ssi->tag_index);
#endif

						/* Note that we set the copy flag here since we only have a
						 * single tag insert buffer per connection. If we don't do
						 * this, insert corruption can occur if more than one insert
						 * is processed before we call tcp_output. */
						err = httpd_ssi_handler(ssi->tag_loopidx, &len, pcb, HTTP_IS_TAG_VOLATILE(hs));

						if (err == ERR_OK) {
                            ssi->tag_insert_len = 0;
							data_to_send = 1;
							ssi->tag_index += len;
                            /* AIR modify: for multiple tag to restart check */
                            ssi->tag_name_len = 0;
							/* Don't return here: keep on sending data */
                            pcb->ssi_record_len = 0;
                            pcb->ssi_record_offt = 0;
						}
                        else if (err == ERR_MEM)
                        {
                            /* Send buffer is not enough for tag value.
                             * Send packet first, then try again */
                            len = 0;
                            data_to_send = 1;
                            pcb->ssi_record_offt = 0;
                        }
					} else {
#if LWIP_HTTPD_SSI_MULTIPART
						if (ssi->tag_part == HTTPD_LAST_TAG_PART)
#endif /* LWIP_HTTPD_SSI_MULTIPART */
						{
							/* We have sent all the insert data so go back to looking for
							 * a new tag. */
							LWIP_DEBUGF(HTTPD_DEBUG, ("Everything sent.\n"));
							ssi->tag_index = 0;
							ssi->tag_state = TAG_NONE;
#if !LWIP_HTTPD_SSI_INCLUDE_TAG
							ssi->parsed = ssi->tag_end;
#endif /* !LWIP_HTTPD_SSI_INCLUDE_TAG*/
						}
					}
					break;
                default:
                    break;
				}
		}
	}

	/* If we drop out of the end of the for loop, this implies we must have
	 * file data to send so send it now. In TAG_SENDING state, we've already
	 * handled this so skip the send if that's the case. */
	if ((ssi->tag_state != TAG_SENDING) && (ssi->parsed > hs->file)) {
#if LWIP_HTTPD_DYNAMIC_FILE_READ && !LWIP_HTTPD_SSI_INCLUDE_TAG
		if ((ssi->tag_state != TAG_NONE) && (ssi->tag_started > ssi->tag_end)) {
			/* If we found tag on the edge of the read buffer: just throw away the first part
				(we have copied/saved everything required for parsing on later). */
			len = (u16_t)(ssi->tag_started - hs->file);
			hs->left -= (ssi->parsed - ssi->tag_started);
			ssi->parsed = ssi->tag_started;
			ssi->tag_started = hs->buf;
		} else
#endif /* LWIP_HTTPD_DYNAMIC_FILE_READ && !LWIP_HTTPD_SSI_INCLUDE_TAG */
		{
			len = (u16_t)LWIP_MIN(ssi->parsed - hs->file, 0xffff);
		}

		err = http_write(pcb, hs->file, &len, HTTP_IS_DATA_VOLATILE(hs));
		if (err == ERR_OK) {
			data_to_send = 1;
			hs->file += len;
			hs->left -= len;
		}
	}
	return data_to_send;
}
#endif /* LWIP_HTTPD_SSI */

/**
 * Try to send more data on this pcb.
 *
 * @param pcb the pcb to send data
 * @param hs connection state
 */
static u8_t
http_send(struct altcp_pcb *pcb, struct http_state *hs)
{
	u8_t data_to_send = HTTP_NO_DATA_TO_SEND;

	LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_send: pcb=%p hs=%p left=%d\n", (void *)pcb,
				(void *)hs, hs != NULL ? (int)hs->left : 0));

#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
	if (hs->unrecved_bytes != 0) {
		return 0;
	}
#endif /* LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND */

	/* If we were passed a NULL state structure pointer, ignore the call. */
	if (hs == NULL) {
		return 0;
	}

#if LWIP_HTTPD_FS_ASYNC_READ
	/* Check if we are allowed to read from this file.
		(e.g. SSI might want to delay sending until data is available) */
	if (!fs_is_file_ready(hs->handle, http_continue, hs)) {
		return 0;
	}
#endif /* LWIP_HTTPD_FS_ASYNC_READ */

#if LWIP_HTTPD_DYNAMIC_HEADERS
	/* Do we have any more header data to send for this file? */
	if (hs->hdr_index < NUM_FILE_HDR_STRINGS) {
		data_to_send = http_send_headers(pcb, hs);
		if ((data_to_send == HTTP_DATA_TO_SEND_FREED) ||
				((data_to_send != HTTP_DATA_TO_SEND_CONTINUE) &&
				(hs->hdr_index < NUM_FILE_HDR_STRINGS))) {
			return data_to_send;
		}
	}
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */

	/* Have we run out of file data to send? If so, we need to read the next
	 * block from the file. */
	if (hs->left == 0) {
		if (!http_check_eof(pcb, hs)) {
			return 0;
		}
	}

#if LWIP_HTTPD_SSI
	if (hs->ssi) {
		data_to_send = http_send_data_ssi(pcb, hs);
	} else
#endif /* LWIP_HTTPD_SSI */
	{
		data_to_send = http_send_data_nonssi(pcb, hs);
	}

	if ((hs->left == 0) && (fs_bytes_left(hs->handle) <= 0)) {
		/* We reached the end of the file so this request is done.
		 * This adds the FIN flag right into the last data segment. */
		LWIP_DEBUGF(HTTPD_DEBUG, ("End of file.\n"));
		http_eof(pcb, hs);
		return 0;
	}
	LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("send_data end.\n"));
	return data_to_send;
}

#if LWIP_HTTPD_SUPPORT_EXTSTATUS
/** Initialize a http connection with a file to send for an error message
 *
 * @param hs http connection state
 * @param error_nr HTTP error number
 * @return ERR_OK if file was found and hs has been initialized correctly
 *         another err_t otherwise
 */
static err_t
http_find_error_file(struct http_state *hs, u16_t error_nr)
{
	const char *uri, *uri1, *uri2, *uri3;

	if (error_nr == 501) {
		uri1 = "/501.html";
		uri2 = "/501.htm";
		uri3 = "/501.shtml";
	} else {
		/* 400 (bad request is the default) */
		uri1 = "/400.html";
		uri2 = "/400.htm";
		uri3 = "/400.shtml";
	}
	if (fs_open(&hs->file_handle, uri1) == ERR_OK) {
		uri = uri1;
	} else if (fs_open(&hs->file_handle, uri2) == ERR_OK) {
		uri = uri2;
	} else if (fs_open(&hs->file_handle, uri3) == ERR_OK) {
		uri = uri3;
	} else {
		LWIP_DEBUGF(HTTPD_DEBUG, ("Error page for error %"U16_F" not found\n",
					error_nr));
		return ERR_ARG;
	}
	return http_init_file(hs, &hs->file_handle, 0, uri, 0, NULL);
}
#else /* LWIP_HTTPD_SUPPORT_EXTSTATUS */
#define http_find_error_file(hs, error_nr) ERR_ARG
#endif /* LWIP_HTTPD_SUPPORT_EXTSTATUS */

/**
 * Get the file struct for a 404 error page.
 * Tries some file names and returns NULL if none found.
 *
 * @param uri pointer that receives the actual file name URI
 * @return file struct for the error page or NULL no matching file was found
 */
static struct fs_file *
http_get_404_file(struct http_state *hs, const char **uri)
{
	err_t err;

	*uri = "/404.html";
	err = fs_open(&hs->file_handle, *uri);
	if (err != ERR_OK) {
		/* 404.html doesn't exist. Try 404.htm instead. */
		*uri = "/404.htm";
		err = fs_open(&hs->file_handle, *uri);
		if (err != ERR_OK) {
			/* 404.htm doesn't exist either. Try 404.shtml instead. */
			*uri = "/404.shtml";
			err = fs_open(&hs->file_handle, *uri);
			if (err != ERR_OK) {
				/* 404.htm doesn't exist either. Indicate to the caller that it should
				 * send back a default 404 page.
				 */
				*uri = NULL;
				return NULL;
			}
		}
	}

	return &hs->file_handle;
}

#if LWIP_HTTPD_SUPPORT_POST
static err_t
http_handle_post_finished(struct http_state *hs)
{
#if LWIP_HTTPD_POST_MANUAL_WND
	/* Prevent multiple calls to httpd_post_finished, since it might have already
		been called before from httpd_post_data_recved(). */
	if (hs->post_finished) {
		return ERR_OK;
	}
	hs->post_finished = 1;
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
	/* application error or POST finished */
	/* NULL-terminate the buffer */
	http_uri_buf[0] = 0;
	httpd_post_finished(hs, http_uri_buf, LWIP_HTTPD_URI_BUF_LEN);
	return http_find_file(hs, http_uri_buf, 0);
}

/** Pass received POST body data to the application and correctly handle
 * returning a response document or closing the connection.
 * ATTENTION: The application is responsible for the pbuf now, so don't free it!
 *
 * @param hs http connection state
 * @param p pbuf to pass to the application
 * @return ERR_OK if passed successfully, another err_t if the response file
 *         hasn't been found (after POST finished)
 */
static err_t
http_post_rxpbuf(struct http_state *hs, struct pbuf *p)
{
	err_t err;

	if (p != NULL) {
		/* adjust remaining Content-Length */
		if (hs->post_content_len_left < p->tot_len) {
			hs->post_content_len_left = 0;
		} else {
			hs->post_content_len_left -= p->tot_len;
		}
	}
#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
	/* prevent connection being closed if httpd_post_data_recved() is called nested */
	hs->unrecved_bytes++;
#endif
	if (p != NULL) {
		err = httpd_post_receive_data(hs, p);
	} else {
		err = ERR_OK;
	}
#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
	hs->unrecved_bytes--;
#endif
	if (err != ERR_OK) {
		/* Ignore remaining content in case of application error */
		hs->post_content_len_left = 0;
	}
	if (hs->post_content_len_left == 0) {
#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
		if (hs->unrecved_bytes != 0) {
			return ERR_OK;
		}
#endif /* LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND */
		/* application error or POST finished */
		return http_handle_post_finished(hs);
	}

	return ERR_OK;
}

#define LWIP_HTTPD_POST_MAX_PAYLOAD_LEN     512
static char http_post_payload[LWIP_HTTPD_POST_MAX_PAYLOAD_LEN];
static u16_t http_post_payload_len = 0;
static char http_upload_contentType[] = "Content-Type";

/** Handle a post request. Called from http_parse_request when method 'POST'
 * is found.
 *
 * @param p The input pbuf (containing the POST header and body).
 * @param hs The http connection state.
 * @param data HTTP request (header and part of body) from input pbuf(s).
 * @param data_len Size of 'data'.
 * @param uri The HTTP URI parsed from input pbuf(s).
 * @param uri_end Pointer to the end of 'uri' (here, the rest of the HTTP
 *                header starts).
 * @return ERR_OK: POST correctly parsed and accepted by the application.
 *         ERR_INPROGRESS: POST not completely parsed (no error yet)
 *         another err_t: Error parsing POST or denied by the application
 */
static err_t
http_post_request(struct pbuf *inp, struct http_state *hs,
                  char *data, u16_t data_len, char *uri, char *uri_end)
{
	err_t err;
	/* search for end-of-header (first double-CRLF) */
	char *crlfcrlf = lwip_strnstr(uri_end + 1, CRLF CRLF, data_len - (uri_end + 1 - data));
    char *seq = NULL;

	if (crlfcrlf != NULL) {
		char *boundary_start = lwip_strnstr(uri_end + 1, http_upgrade_context[hs->receive_state].desc, crlfcrlf - (uri_end + 1));
		if(boundary_start != NULL)
		{
			boundary_start += strlen(http_upgrade_context[hs->receive_state].desc);
			char *boundary_end = lwip_strnstr(boundary_start, CRLF, crlfcrlf - (boundary_start));
			if(boundary_end != NULL)
			{
				int boundary_len = boundary_end - boundary_start;
				if(boundary_len < LWIP_HTTPD_POST_MAX_BOUNDARY_LEN)
				{
#if LWIP_HTTPD_POST_MANUAL_WND
					hs->no_auto_wnd = 1;
#endif
                    hs->post_boundary_string = (char *)mem_malloc(LWIP_HTTPD_POST_MAX_BOUNDARY_LEN);
                    if (NULL != hs->post_boundary_string)
                    {
                        memset(hs->post_boundary_string, 0, LWIP_HTTPD_POST_MAX_BOUNDARY_LEN);
                        hs->post_boundary_string[0] = '\r';
                        hs->post_boundary_string[1] = '\n';
                        hs->post_boundary_string[2] = hs->post_boundary_string[3] = '-';
                        MEMCPY(hs->post_boundary_string + 4, boundary_start, boundary_len);
                        hs->receive_state = HTTP_POST_UPGRADE_FILESIZE_H_STAT;
                        LWIP_DEBUGF(HTTPD_DEBUG, ("found boundary=[%s]\n", hs->post_boundary_string + 2));
                        if(0 == _needLen)
                        {
                            char *fileSize_start = lwip_strnstr(uri_end + 1, http_upgrade_context[hs->receive_state].desc, crlfcrlf - (uri_end + 1));
                            if(NULL == fileSize_start)
                            {
                                hs->receive_state = HTTP_POST_UPGRADE_FILESIZE_L_STAT;
                                fileSize_start = lwip_strnstr(uri_end + 1, http_upgrade_context[hs->receive_state].desc, crlfcrlf - (uri_end + 1));
                            }
                            if(NULL != fileSize_start)
                            {
                                fileSize_start += strlen(http_upgrade_context[hs->receive_state].desc);
                                char *fileSize_end = lwip_strnstr(fileSize_start, CRLF, crlfcrlf - fileSize_start);
                                if(NULL != fileSize_end)
                                {
                                    int fileSize_len = fileSize_end - fileSize_start;
                                    char tmpbuf[20] = {0};
                                    memcpy(tmpbuf, fileSize_start, fileSize_len);
                                    _needLen = atol(tmpbuf);
                                    _rcvLen = 0;
                                    WriteBufferInit((unsigned char *) TempSystemBase);
                                }
                            }
                        }
                        hs->receive_state = HTTP_POST_UPGRADE_FILENAME_STAT;
                    }
                    else
                    {
                        LWIP_DEBUGF(HTTPD_DEBUG, ("No memory\n"));

                        return ERR_MEM;
                    }
                }
                else
                {
                    LWIP_DEBUGF(HTTPD_DEBUG, ("boundary oversize\n"));

                    return ERR_ARG;
                }
                /* Parse upload cgi sequence */
                seq = strstr(uri, "?");
                if(seq != NULL)
                {
                    seq++;
                    memset(http_post_payload, 0, LWIP_HTTPD_POST_MAX_PAYLOAD_LEN);
                    memcpy(http_post_payload, seq, strlen(seq));
                    http_post_payload_len = strlen(seq);
                }
            }
        }
        else
        {
            /* For login initial http_post_payload */
            memset(http_post_payload, 0, LWIP_HTTPD_POST_MAX_PAYLOAD_LEN);
            http_post_payload_len = 0;
        }

		/* search for "Content-Length: " */
#define HTTP_HDR_CONTENT_LEN                "Content-Length: "
#define HTTP_HDR_CONTENT_LEN_LEN            16
#define HTTP_HDR_CONTENT_LEN_DIGIT_MAX_LEN  10
		char *scontent_len = lwip_strnstr(uri_end + 1, HTTP_HDR_CONTENT_LEN, crlfcrlf - (uri_end + 1));
		if (scontent_len != NULL) {
			char *scontent_len_end = lwip_strnstr(scontent_len + HTTP_HDR_CONTENT_LEN_LEN, CRLF, HTTP_HDR_CONTENT_LEN_DIGIT_MAX_LEN);
			if (scontent_len_end != NULL) {
				int content_len;
				char *content_len_num = scontent_len + HTTP_HDR_CONTENT_LEN_LEN;
				content_len = atoi(content_len_num);
				if (content_len == 0) {
					/* if atoi returns 0 on error, fix this */
					if ((content_len_num[0] != '0') || (content_len_num[1] != '\r')) {
						content_len = -1;
					}
				}
				if(content_len >= 0) {
					/* adjust length of HTTP header passed to application */
					const char *hdr_start_after_uri = uri_end + 1;
					u16_t hdr_len = (u16_t)LWIP_MIN(data_len, crlfcrlf + 4 - data);
					u16_t hdr_data_len = (u16_t)LWIP_MIN(data_len, crlfcrlf + 4 - hdr_start_after_uri);
					u8_t post_auto_wnd = 1;
					http_uri_buf[0] = 0;
					/* trim http header */
					*crlfcrlf = 0;
					err = httpd_post_begin(hs, uri, hdr_start_after_uri, hdr_data_len, content_len,
						http_uri_buf, LWIP_HTTPD_URI_BUF_LEN, &post_auto_wnd);
					if (err == ERR_OK) {
						/* try to pass in data of the first pbuf(s) */
						struct pbuf *q = inp;
						u16_t start_offset = hdr_len;
#if LWIP_HTTPD_POST_MANUAL_WND
						hs->no_auto_wnd = !post_auto_wnd;
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
						/* set the Content-Length to be received for this POST */
						hs->post_content_len_left = (u32_t)content_len;
						/* get to the pbuf where the body starts */
						while ((q != NULL) && (q->len <= start_offset)) {
							start_offset -= q->len;
							q = q->next;
						}
						if (q != NULL) {
							/* hide the remaining HTTP header */
							pbuf_remove_header(q, start_offset);
#if LWIP_HTTPD_POST_MANUAL_WND
							if (!post_auto_wnd) {
								/* already tcp_recved() this data... */
								hs->unrecved_bytes = q->tot_len;
							}
#endif /* LWIP_HTTPD_POST_MANUAL_WND */
							pbuf_ref(q);
							return http_post_rxpbuf(hs, q);
						} else if (hs->post_content_len_left == 0) {
							q = pbuf_alloc(PBUF_RAW, 0, PBUF_REF);
							return http_post_rxpbuf(hs, q);
						} else {
							return ERR_OK;
						}
					} else {
						/* return file passed from application */
						return http_find_file(hs, http_uri_buf, 0);
					}
				} else {
					LWIP_DEBUGF(HTTPD_DEBUG, ("POST received invalid Content-Length: %s\n",
						content_len_num));
					return ERR_ARG;
				}
			}
		}
		/* If we come here, headers are fully received (double-crlf), but Content-Length
			was not included. Since this is currently the only supported method, we have
			to fail in this case! */
		LWIP_DEBUGF(HTTPD_DEBUG, ("Error when parsing Content-Length\n"));
		return ERR_ARG;
	}
	/* if we come here, the POST is incomplete */
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
	return ERR_INPROGRESS;
#else /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
	return ERR_ARG;
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
}

#if LWIP_HTTPD_POST_MANUAL_WND
/**
 * @ingroup httpd
 * A POST implementation can call this function to update the TCP window.
 * This can be used to throttle data reception (e.g. when received data is
 * programmed to flash and data is received faster than programmed).
 *
 * @param connection A connection handle passed to httpd_post_begin for which
 *        httpd_post_finished has *NOT* been called yet!
 * @param recved_len Length of data received (for window update)
 */
void httpd_post_data_recved(void *connection, u16_t recved_len)
{
	struct http_state *hs = (struct http_state *)connection;
	if (hs != NULL) {
		if (hs->no_auto_wnd) {
			u16_t len = recved_len;
			if (hs->unrecved_bytes >= recved_len) {
				hs->unrecved_bytes -= recved_len;
			} else {
				LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("httpd_post_data_recved: recved_len too big\n"));
				len = (u16_t)hs->unrecved_bytes;
				hs->unrecved_bytes = 0;
			}
			if (hs->pcb != NULL) {
				if (len != 0) {
					altcp_recved(hs->pcb, len);
				}
				if ((hs->post_content_len_left == 0) && (hs->unrecved_bytes == 0)) {
					/* finished handling POST */
					http_handle_post_finished(hs);
					http_send(hs->pcb, hs);
				}
			}
		}
	}
}
#endif /* LWIP_HTTPD_POST_MANUAL_WND */

#endif /* LWIP_HTTPD_SUPPORT_POST */

#if LWIP_HTTPD_FS_ASYNC_READ
/** Try to send more data if file has been blocked before
 * This is a callback function passed to fs_read_async().
 */
static void
http_continue(void *connection)
{
	struct http_state *hs = (struct http_state *)connection;
	LWIP_ASSERT_CORE_LOCKED();
	if (hs && (hs->pcb) && (hs->handle)) {
		LWIP_ASSERT("hs->pcb != NULL", hs->pcb != NULL);
		LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("httpd_continue: try to send more data\n"));
		if (http_send(hs->pcb, hs)) {
			/* If we wrote anything to be sent, go ahead and send it now. */
			LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("tcp_output\n"));
			altcp_output(hs->pcb);
		}
  }
}
#endif /* LWIP_HTTPD_FS_ASYNC_READ */

/**
 * When data has been received in the correct state, try to parse it
 * as a HTTP request.
 *
 * @param inp the received pbuf
 * @param hs the connection state
 * @param pcb the altcp_pcb which received this packet
 * @return ERR_OK if request was OK and hs has been initialized correctly
 *         ERR_INPROGRESS if request was OK so far but not fully received
 *         another err_t otherwise
 */
static err_t
http_parse_request(struct pbuf *inp, struct http_state *hs, struct altcp_pcb *pcb)
{
	char *data;
	char *crlf;
	u16_t data_len;
	struct pbuf *p = inp;
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
	u16_t clen;
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
#if LWIP_HTTPD_SUPPORT_POST
	err_t err;
#endif /* LWIP_HTTPD_SUPPORT_POST */

	LWIP_UNUSED_ARG(pcb); /* only used for post */
	LWIP_ASSERT("p != NULL", p != NULL);
	LWIP_ASSERT("hs != NULL", hs != NULL);

	if ((hs->handle != NULL) || (hs->file != NULL)) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("Received data while sending a file\n"));
		/* already sending a file */
		/* @todo: abort? */
		return ERR_USE;
	}

#if LWIP_HTTPD_SUPPORT_REQUESTLIST

	LWIP_DEBUGF(HTTPD_DEBUG, ("Received %"U16_F" bytes\n", p->tot_len));
	/* first check allowed characters in this pbuf? */

	/* enqueue the pbuf */
	if (hs->req == NULL) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("First pbuf\n"));
		hs->req = p;
	} else {
		LWIP_DEBUGF(HTTPD_DEBUG, ("pbuf enqueued\n"));
		pbuf_cat(hs->req, p);
	}
	/* increase pbuf ref counter as it is freed when we return but we want to
		keep it on the req list */
	pbuf_ref(p);

	if (hs->req->next != NULL) {
		data_len = LWIP_MIN(hs->req->tot_len, LWIP_HTTPD_MAX_REQ_LENGTH);
		pbuf_copy_partial(hs->req, httpd_req_buf, data_len, 0);
		data = httpd_req_buf;
	} else
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
	{
		data = (char *)p->payload;
		data_len = p->len;
		if (p->len != p->tot_len) {
			LWIP_DEBUGF(HTTPD_DEBUG, ("Warning: incomplete header due to chained pbufs\n"));
		}
	}

	/* received enough data for minimal request? */
	if (data_len >= MIN_REQ_LEN) {
		/* wait for CRLF before parsing anything */
		crlf = lwip_strnstr(data, CRLF, data_len);
		if (crlf != NULL) {
#if LWIP_HTTPD_SUPPORT_POST
			int is_post = 0;
#endif /* LWIP_HTTPD_SUPPORT_POST */
			int is_09 = 0;
			char *sp1, *sp2;
			u16_t left_len, uri_len;
			LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("CRLF received, parsing request\n"));
			/* parse method */
			if (!strncmp(data, "GET ", 4)) {
				sp1 = data + 3;
				/* received GET request */
				LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Received GET request\"\n"));
#if LWIP_HTTPD_SUPPORT_POST
			} else if (!strncmp(data, "POST ", 5)) {
				/* store request type */
				is_post = 1;
				sp1 = data + 4;
				/* received GET request */
				LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Received POST request\n"));
#endif /* LWIP_HTTPD_SUPPORT_POST */
			} else {
				/* null-terminate the METHOD (pbuf is freed anyway wen returning) */
				data[4] = 0;
				/* unsupported method! */
				LWIP_DEBUGF(HTTPD_DEBUG, ("Unsupported request method (not implemented): \"%s\"\n",
								data));
				return http_find_error_file(hs, 501);
			}
			/* if we come here, method is OK, parse URI */
			left_len = (u16_t)(data_len - ((sp1 + 1) - data));
			sp2 = lwip_strnstr(sp1 + 1, " ", left_len);
#if LWIP_HTTPD_SUPPORT_V09
			if (sp2 == NULL) {
				/* HTTP 0.9: respond with correct protocol version */
				sp2 = lwip_strnstr(sp1 + 1, CRLF, left_len);
				is_09 = 1;
#if LWIP_HTTPD_SUPPORT_POST
				if (is_post) {
					/* HTTP/0.9 does not support POST */
					goto badrequest;
				}
#endif /* LWIP_HTTPD_SUPPORT_POST */
			}
#endif /* LWIP_HTTPD_SUPPORT_V09 */
			uri_len = (u16_t)(sp2 - (sp1 + 1));
			if ((sp2 != 0) && (sp2 > sp1)) {
				/* wait for CRLFCRLF (indicating end of HTTP headers) before parsing anything */
				if (lwip_strnstr(data, CRLF CRLF, data_len) != NULL) {
					char *uri = sp1 + 1;
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
					/* This is HTTP/1.0 compatible: for strict 1.1, a connection
						 would always be persistent unless "close" was specified. */
					if (!is_09 && (lwip_strnstr(data, HTTP11_CONNECTIONKEEPALIVE, data_len) ||
						lwip_strnstr(data, HTTP11_CONNECTIONKEEPALIVE2, data_len))) {
						hs->keepalive = 1;
					} else {
						hs->keepalive = 0;
					}
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
					/* null-terminate the METHOD (pbuf is freed anyway wen returning) */
					*sp1 = 0;
					uri[uri_len] = 0;
                    /* Check cookie value except index.html */
                    if((0 != strcmp(uri, "/index.html")) && (0 != strcmp(uri, "/logon.cgi")) && (uri_len > 1)
                        && ((NULL != strstr(uri, ".html")) || (NULL != strstr(uri, ".cgi")))
                        && (NULL == strstr(uri, "languagechange.cgi")))
                    {
                        if(FALSE == _http_find_cookie((sp2 + 1)))
                        {
                            /* Check cookie fail, set page to login */
                            http_cookie_index = HTTPD_CONNECTION_TIMEOUT;
						    return http_find_file(hs, "/index_re.html", is_09);
                        }
                    }
                    if(0 != _needLen)
                    {
                        if((0 != strncmp(uri, "/fupgrade.cgi", HTTP_UPGRADE_PAGE_LEN)) && (0 != strncmp(uri, "/conf_restore.cgi", HTTP_RESTORE_PAGE_LEN)))
                        {
                            /*
                             * To switch web pages during a firmware update,
                             * such as when refreshing the page, reset the update
                             * information.
                             */
                            _needLen = 0;
                            hs->receive_state = HTTP_POST_UPGRADE_INIT_STAT;
                            if(NULL != hs->post_boundary_string)
                            {
                                mem_free(hs->post_boundary_string);
                                hs->post_boundary_string = NULL;
                            }
                        }
                    }
					LWIP_DEBUGF(HTTPD_DEBUG, ("Received \"%s\" request for URI: \"%s\"\n",
						data, uri));
#if LWIP_HTTPD_SUPPORT_POST
					if (is_post) {
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
						struct pbuf *q = hs->req;
#else /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
						struct pbuf *q = inp;
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
						err = http_post_request(q, hs, data, data_len, uri, sp2);
						if (err != ERR_OK) {
							/* restore header for next try */
							*sp1 = ' ';
							*sp2 = ' ';
							uri[uri_len] = ' ';
						}
						if (err == ERR_ARG) {
							goto badrequest;
						}
						return err;
					} else
#endif /* LWIP_HTTPD_SUPPORT_POST */
					{
						return http_find_file(hs, uri, is_09);
					}
				}
			} else {
				LWIP_DEBUGF(HTTPD_DEBUG, ("invalid URI\n"));
			}
		}
	}

#if LWIP_HTTPD_SUPPORT_REQUESTLIST
	clen = pbuf_clen(hs->req);
	if ((hs->req->tot_len <= LWIP_HTTPD_REQ_BUFSIZE) &&
			(clen <= LWIP_HTTPD_REQ_QUEUELEN)) {
		/* request not fully received (too short or CRLF is missing) */
		return ERR_INPROGRESS;
	} else
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
	{
#if LWIP_HTTPD_SUPPORT_POST
badrequest:
#endif /* LWIP_HTTPD_SUPPORT_POST */
		LWIP_DEBUGF(HTTPD_DEBUG, ("bad request\n"));
		/* could not parse request */
		return http_find_error_file(hs, 400);
	}
}

#if LWIP_HTTPD_SSI && (LWIP_HTTPD_SSI_BY_FILE_EXTENSION == 1)
/* Check if SSI should be parsed for this file/URL
 * (With LWIP_HTTPD_SSI_BY_FILE_EXTENSION == 2, this function can be
 * overridden by an external implementation.)
 *
 * @return 1 for SSI, 0 for standard files
 */
static u8_t
http_uri_is_ssi(struct fs_file *file, const char *uri)
{
	size_t loop;
	u8_t tag_check = 0;
	if (file != NULL) {
		/* See if we have been asked for an shtml file and, if so,
			enable tag checking. */
		const char *ext = NULL, *sub;
		char *param = (char *)strstr(uri, "?");
		if (param != NULL) {
			/* separate uri from parameters for now, set back later */
			*param = 0;
		}
		sub = uri;
		ext = uri;
		for (sub = strstr(sub, "."); sub != NULL; sub = strstr(sub, ".")) {
			ext = sub;
			sub++;
		}
		for (loop = 0; loop < NUM_SHTML_EXTENSIONS; loop++) {
			if (!lwip_stricmp(ext, g_pcSSIExtensions[loop]))
            {
				tag_check = 1;
				break;
			}
		}
		if (param != NULL) {
			*param = '?';
		}
	}
  return tag_check;
}
#endif /* LWIP_HTTPD_SSI */

/** Try to find the file specified by uri and, if found, initialize hs
 * accordingly.
 *
 * @param hs the connection state
 * @param uri the HTTP header URI
 * @param is_09 1 if the request is HTTP/0.9 (no HTTP headers in response)
 * @return ERR_OK if file was found and hs has been initialized correctly
 *         another err_t otherwise
 */
static err_t
http_find_file(struct http_state *hs, const char *uri, int is_09)
{
	size_t loop;
	struct fs_file *file = NULL;
	char *params = NULL;
	err_t err;
#if LWIP_HTTPD_CGI
	/** AIR modification */
	int i = 0;
#endif /* LWIP_HTTPD_CGI */
#if !LWIP_HTTPD_SSI
	const
#endif /* !LWIP_HTTPD_SSI */
	/* By default, assume we will not be processing server-side-includes tags */
	u8_t tag_check = 0;

	/* Have we been asked for the default file (in root or a directory) ? */
#if LWIP_HTTPD_MAX_REQUEST_URI_LEN
	size_t uri_len = strlen(uri);
	if ((uri_len > 0) && (uri[uri_len - 1] == '/') &&
		((uri != http_uri_buf) || (uri_len == 1))) {
		size_t copy_len = LWIP_MIN(sizeof(http_uri_buf) - 1, uri_len - 1);
		if (copy_len > 0) {
			MEMCPY(http_uri_buf, uri, copy_len);
			http_uri_buf[copy_len] = 0;
		}
#else /* LWIP_HTTPD_MAX_REQUEST_URI_LEN */
	if ((uri[0] == '/') &&	(uri[1] == 0)) {
#endif /* LWIP_HTTPD_MAX_REQUEST_URI_LEN */
		/* Try each of the configured default filenames until we find one
			 that exists. */
		for (loop = 0; loop < NUM_DEFAULT_FILENAMES; loop++) {
			const char *file_name;
#if LWIP_HTTPD_MAX_REQUEST_URI_LEN
			if (copy_len > 0) {
				size_t len_left = sizeof(http_uri_buf) - copy_len - 1;
				if (len_left > 0) {
					size_t name_len = strlen(httpd_default_filenames[loop].name);
					size_t name_copy_len = LWIP_MIN(len_left, name_len);
					MEMCPY(&http_uri_buf[copy_len], httpd_default_filenames[loop].name, name_copy_len);
					http_uri_buf[copy_len + name_copy_len] = 0;
				}
				file_name = http_uri_buf;
			} else
#endif /* LWIP_HTTPD_MAX_REQUEST_URI_LEN */
			{
				file_name = httpd_default_filenames[loop].name;
			}
			LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Looking for %s...\n", file_name));
			err = fs_open(&hs->file_handle, file_name);
			if (err == ERR_OK) {
				uri = file_name;
				file = &hs->file_handle;
				LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Opened.\n"));
#if LWIP_HTTPD_SSI
				tag_check = httpd_default_filenames[loop].shtml;
#endif /* LWIP_HTTPD_SSI */
				break;
			}
		}
	}
	if (file == NULL) {
		/* No - we've been asked for a specific file. */
		/* First, isolate the base URI (without any parameters) */
		params = (char *)strchr(uri, '?');
		if (params != NULL) {
			/* URI contains parameters. NULL-terminate the base URI */
			*params = '\0';
			params++;
		}

#if LWIP_HTTPD_CGI
        http_cgi_paramcount = -1;
        /* Does the base URI we have isolated correspond to a CGI handler? */
        if (httpd_num_cgis && httpd_cgis) {
            for (i = 0; i < httpd_num_cgis; i++) {
                if (strcmp(uri, httpd_cgis[i].pcCGIName) == 0) {
                    /*
                     * We found a CGI that handles this URI so extract the
                     * parameters and call the handler.
                     */
                    http_cgi_paramcount = extract_uri_parameters(hs, params);
                    /** AIR modification */
                    /* CGI handle function */
#if HTTPD_DBG_ON
                    DEBUG(debugflags, "[%s] line [%d] i=[%d] paramCnt=[%d]\n", __FUNCTION__, __LINE__, i, http_cgi_paramcount);
#endif

#ifdef AIR_SUPPORT_MQTTD
                    if (cgiMutex)
                    {
                       xSemaphoreTake(cgiMutex, (1000 / portTICK_RATE_MS));
                    }
#endif
                    httpd_cgis[i].pfnCGIHandler(i, http_cgi_paramcount, hs->params, hs->param_vals);
#ifdef AIR_SUPPORT_MQTTD
                    if (cgiMutex)
                    {
                       xSemaphoreGive(cgiMutex);
                    }
#endif

                    uri = httpd_cgis[i].retUri;
#if HTTPD_DBG_ON
                    DEBUG(debugflags, "[%s] line [%d] return uri=[%s]\n", __FUNCTION__, __LINE__, uri);
#endif
                    break;
                }
            }
		}
#endif /* LWIP_HTTPD_CGI */

		LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("Opening %s\n", uri));

        if(!strcmp(uri, "/configfile.json"))
        {
            err = cgi_set_handle_conf_backup(&hs->file_handle);
        }
		else
        {
            /* AIR modify: Replace receive xml file name with common.xml */
            if(NULL != strstr(uri, ".xml"))
            {
                err = fs_open(&hs->file_handle, common_xml);
            }
            else
            {
                err = fs_open(&hs->file_handle, uri);
            }
        }
		if (err == ERR_OK) {
			file = &hs->file_handle;
		} else {
			file = http_get_404_file(hs, &uri);
		}
#if LWIP_HTTPD_SSI
		if (file != NULL) {
			if (file->flags & FS_FILE_FLAGS_SSI) {
				tag_check = 1;
			} else {
#if LWIP_HTTPD_SSI_BY_FILE_EXTENSION
				tag_check = http_uri_is_ssi(file, uri);
#endif /* LWIP_HTTPD_SSI_BY_FILE_EXTENSION */
			}
		}
#endif /* LWIP_HTTPD_SSI */
	}
	if (file == NULL) {
		/* None of the default filenames exist so send back a 404 page */
		file = http_get_404_file(hs, &uri);
	}
	return http_init_file(hs, file, is_09, uri, tag_check, params);
}

/** Initialize a http connection with a file to send (if found).
 * Called by http_find_file and http_find_error_file.
 *
 * @param hs http connection state
 * @param file file structure to send (or NULL if not found)
 * @param is_09 1 if the request is HTTP/0.9 (no HTTP headers in response)
 * @param uri the HTTP header URI
 * @param tag_check enable SSI tag checking
 * @param params != NULL if URI has parameters (separated by '?')
 * @return ERR_OK if file was found and hs has been initialized correctly
 *         another err_t otherwise
 */
static err_t
http_init_file(struct http_state *hs, struct fs_file *file, int is_09, const char *uri,
               u8_t tag_check, char *params)
{
    char *ptr = NULL;
#if !LWIP_HTTPD_SUPPORT_V09
	LWIP_UNUSED_ARG(is_09);
#endif
	if (file != NULL) {
		/* file opened, initialise struct http_state */
#if !LWIP_HTTPD_DYNAMIC_FILE_READ
		/* If dynamic read is disabled, file data must be in one piece and available now */
		LWIP_ASSERT("file->data != NULL", file->data != NULL);
#endif

#if LWIP_HTTPD_SSI
		if (tag_check) {
			struct http_ssi_state *ssi = http_ssi_state_alloc();
			if (ssi != NULL) {
				ssi->tag_index = 0;
				ssi->tag_state = TAG_NONE;
				ssi->parsed = file->data;
				ssi->parse_left = file->len;
				ssi->tag_end = file->data;
                /* AIR modify for xml file */
                ptr = strstr(uri, ".xml");
                /* uri is supposed to begin with '/'. */
                if((NULL != ptr) && (ptr > uri + 1))
                {
                    ssi->tag_name_len = ptr - uri - 1;
                    if(LWIP_HTTPD_MAX_TAG_NAME_LEN > ssi->tag_name_len)
                    {
                        strncpy(ssi->tag_name, &uri[1], ssi->tag_name_len);
                    }
                    else
                    {
                        ssi->tag_name_len = 0;
                    }
                }
				hs->ssi = ssi;
			}
		}
#else /* LWIP_HTTPD_SSI */
		LWIP_UNUSED_ARG(tag_check);
#endif /* LWIP_HTTPD_SSI */
		hs->handle = file;
#if LWIP_HTTPD_CGI_SSI
		if (params != NULL) {
			/* URI contains parameters, call generic CGI handler */
			int count;
#if LWIP_HTTPD_CGI
			if (http_cgi_paramcount >= 0) {
				count = http_cgi_paramcount;
			} else
#endif
			{
				count = extract_uri_parameters(hs, params);
			}
			httpd_cgi_handler(file, uri, count, http_cgi_params, http_cgi_param_vals
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
												, file->state
#endif /* LWIP_HTTPD_FILE_STATE */
											 );
		}
#else /* LWIP_HTTPD_CGI_SSI */
		LWIP_UNUSED_ARG(params);
#endif /* LWIP_HTTPD_CGI_SSI */
		hs->file = file->data;
		LWIP_ASSERT("File length must be positive!", (file->len >= 0));
#if LWIP_HTTPD_CUSTOM_FILES
		if (file->is_custom_file && (file->data == NULL)) {
			/* custom file, need to read data first (via fs_read_custom) */
			hs->left = 0;
		} else
#endif /* LWIP_HTTPD_CUSTOM_FILES */
		{
			hs->left = (u32_t)file->len;
		}
		hs->retries = 0;
#if LWIP_HTTPD_TIMING
		hs->time_started = sys_now();
#endif /* LWIP_HTTPD_TIMING */
#if !LWIP_HTTPD_DYNAMIC_HEADERS
		LWIP_ASSERT("HTTP headers not included in file system",
			(hs->handle->flags & FS_FILE_FLAGS_HEADER_INCLUDED) != 0);
#endif /* !LWIP_HTTPD_DYNAMIC_HEADERS */
#if LWIP_HTTPD_SUPPORT_V09
		if (is_09 && ((hs->handle->flags & FS_FILE_FLAGS_HEADER_INCLUDED) != 0)) {
			/* HTTP/0.9 responses are sent without HTTP header,
				search for the end of the header. */
			char *file_start = lwip_strnstr(hs->file, CRLF CRLF, hs->left);
			if (file_start != NULL) {
				int diff = file_start + 4 - hs->file;
				hs->file += diff;
				hs->left -= (u32_t)diff;
			}
		}
#endif /* LWIP_HTTPD_SUPPORT_V09*/
	} else {
		hs->handle = NULL;
		hs->file = NULL;
		hs->left = 0;
		hs->retries = 0;
	}
#if LWIP_HTTPD_DYNAMIC_HEADERS
	/* Determine the HTTP headers to send based on the file extension of
	 * the requested URI. */
	if ((hs->handle == NULL) || ((hs->handle->flags & FS_FILE_FLAGS_HEADER_INCLUDED) == 0)) {
		get_http_headers(hs, uri);
	}
#else /* LWIP_HTTPD_DYNAMIC_HEADERS */
	LWIP_UNUSED_ARG(uri);
#endif /* LWIP_HTTPD_DYNAMIC_HEADERS */
#if LWIP_HTTPD_SUPPORT_11_KEEPALIVE
	if (hs->keepalive) {
#if LWIP_HTTPD_SSI
        if ((hs->ssi != NULL) && ((hs->handle != NULL) && ((hs->handle->flags & FS_FILE_FLAGS_HEADER_CHUNKED) == 0)))
        {
            hs->keepalive = 0;
        }
        else
#endif /* LWIP_HTTPD_SSI */
        {
            if ((hs->handle != NULL))
            {
                if((hs->handle->flags & FS_FILE_FLAGS_HEADER_CHUNKED) == 0)
                {
                    if(((hs->handle->flags & (FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT)) == FS_FILE_FLAGS_HEADER_INCLUDED))
                    {
                        hs->keepalive = 0;
                    }
                }
            }
        }
    }
#endif /* LWIP_HTTPD_SUPPORT_11_KEEPALIVE */
    return ERR_OK;
}

/**
 * The pcb had an error and is already deallocated.
 * The argument might still be valid (if != NULL).
 */
static void
http_err(void *arg, err_t err)
{
	struct http_state *hs = (struct http_state *)arg;
	LWIP_UNUSED_ARG(err);

	LWIP_DEBUGF(HTTPD_DEBUG, ("http_err: %s", lwip_strerr(err)));

	if (hs != NULL) {
		http_state_free(hs);
	}
}

/**
 * Data has been sent and acknowledged by the remote host.
 * This means that more data can be sent.
 */
static err_t
http_sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
	struct http_state *hs = (struct http_state *)arg;

	LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_sent %p\n", (void *)pcb));

	LWIP_UNUSED_ARG(len);

	if (hs == NULL) {
		return ERR_OK;
	}

	hs->retries = 0;

	http_send(pcb, hs);

	return ERR_OK;
}

/**
 * The poll function is called every 2nd second.
 * If there has been no data sent (which resets the retries) in 8 seconds, close.
 * If the last portion of a file has not been sent in 2 seconds, close.
 *
 * This could be increased, but we don't want to waste resources for bad connections.
 */
static err_t
http_poll(void *arg, struct altcp_pcb *pcb)
{
	struct http_state *hs = (struct http_state *)arg;
	LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_poll: pcb=%p hs=%p pcb_state=%s\n",
		(void *)pcb, (void *)hs, tcp_debug_state_str(altcp_dbg_get_tcp_state(pcb))));

	if (hs == NULL) {
		err_t closed;
		/* arg is null, close. */
		LWIP_DEBUGF(HTTPD_DEBUG, ("http_poll: arg is NULL, close\n"));
		closed = http_close_conn(pcb, NULL);
		LWIP_UNUSED_ARG(closed);
#if LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR
		if (closed == ERR_MEM) {
			altcp_abort(pcb);
			return ERR_ABRT;
		}
#endif /* LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR */
		return ERR_OK;
	} else {
		hs->retries++;
		if (hs->retries == HTTPD_MAX_RETRIES) {
			LWIP_DEBUGF(HTTPD_DEBUG, ("http_poll: too many retries, close\n"));
			http_close_conn(pcb, hs);
			return ERR_OK;
		}

		/* If this connection has a file open, try to send some more data. If
		 * it has not yet received a GET request, don't do this since it will
		 * cause the connection to close immediately. */
		if (hs->handle) {
			LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_poll: try to send more data\n"));
			if (http_send(pcb, hs)) {
				/* If we wrote anything to be sent, go ahead and send it now. */
				LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("tcp_output\n"));
				altcp_output(pcb);
			}
		}
	}

	return ERR_OK;
}

/**
 * Data has been received on this pcb.
 * For HTTP 1.0, this should normally only happen once (if the request fits in one packet).
 */
static err_t
http_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct http_state *hs = (struct http_state *)arg;
	LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_recv: pcb=%p pbuf=%p err=%s\n", (void *)pcb,
		(void *)p, lwip_strerr(err)));

	if ((err != ERR_OK) || (p == NULL) || (hs == NULL)) {
		/* error or closed by other side? */
		if (p != NULL) {
			/* Inform TCP that we have taken the data. */
			altcp_recved(pcb, p->tot_len);
			pbuf_free(p);
		}
		if (hs == NULL) {
			/* this should not happen, only to be robust */
			LWIP_DEBUGF(HTTPD_DEBUG, ("Error, http_recv: hs is NULL, close\n"));
		}
		http_close_conn(pcb, hs);
		return ERR_OK;
	}

#if LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND
	if (hs->no_auto_wnd) {
		hs->unrecved_bytes += p->tot_len;
	} else
#endif /* LWIP_HTTPD_SUPPORT_POST && LWIP_HTTPD_POST_MANUAL_WND */
	{
		/* Inform TCP that we have taken the data. */
		altcp_recved(pcb, p->tot_len);
	}

#if LWIP_HTTPD_SUPPORT_POST
	if (hs->post_content_len_left > 0) {
		/* reset idle counter when POST data is received */
		hs->retries = 0;
		/* this is data for a POST, pass the complete pbuf to the application */
		http_post_rxpbuf(hs, p);
		/* pbuf is passed to the application, don't free it! */
		if (hs->post_content_len_left == 0) {
			/* all data received, send response or close connection */
			http_send(pcb, hs);
		}
		return ERR_OK;
	} else
#endif /* LWIP_HTTPD_SUPPORT_POST */
	{
		if (hs->handle == NULL) {
			err_t parsed = http_parse_request(p, hs, pcb);
			LWIP_ASSERT("http_parse_request: unexpected return value", parsed == ERR_OK
				|| parsed == ERR_INPROGRESS || parsed == ERR_ARG || parsed == ERR_USE);
#if LWIP_HTTPD_SUPPORT_REQUESTLIST
			if (parsed != ERR_INPROGRESS) {
				/* request fully parsed or error */
				if (hs->req != NULL) {
					pbuf_free(hs->req);
					hs->req = NULL;
				}
			}
#endif /* LWIP_HTTPD_SUPPORT_REQUESTLIST */
			pbuf_free(p);
			if (parsed == ERR_OK) {
#if LWIP_HTTPD_SUPPORT_POST
				if (hs->post_content_len_left == 0)
#endif /* LWIP_HTTPD_SUPPORT_POST */
				{
					LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_recv: data %p len %"S32_F"\n", (const void *)hs->file, hs->left));
					http_send(pcb, hs);
				}
			} else if (parsed == ERR_ARG) {
				/* @todo: close on ERR_USE? */
				http_close_conn(pcb, hs);
			}
		} else {
			LWIP_DEBUGF(HTTPD_DEBUG, ("http_recv: already sending data\n"));
			/* already sending but still receiving data, we might want to RST here? */
			pbuf_free(p);
		}
	}
	return ERR_OK;
}

/**
 * A new incoming connection has been accepted.
 */
static err_t
http_accept(void *arg, struct altcp_pcb *pcb, err_t err)
{
	struct http_state *hs;
	LWIP_UNUSED_ARG(err);
	LWIP_UNUSED_ARG(arg);
	LWIP_DEBUGF(HTTPD_DEBUG, ("http_accept %p / %p\n", (void *)pcb, arg));

	if ((err != ERR_OK) || (pcb == NULL)) {
		return ERR_VAL;
	}

	/* Set priority */
	altcp_setprio(pcb, HTTPD_TCP_PRIO);

	/* Allocate memory for the structure that holds the state of the
		 connection - initialized by that function. */
	hs = http_state_alloc();
	if (hs == NULL) {
		LWIP_DEBUGF(HTTPD_DEBUG, ("http_accept: Out of memory, RST\n"));
		return ERR_MEM;
	}
	hs->pcb = pcb;

	/* Tell TCP that this is the structure we wish to be passed for our
		 callbacks. */
	altcp_arg(pcb, hs);

	/* Set up the various callback functions */
	altcp_recv(pcb, http_recv);
	altcp_err(pcb, http_err);
	altcp_poll(pcb, http_poll, HTTPD_POLL_INTERVAL);
	altcp_sent(pcb, http_sent);

	return ERR_OK;
}

static void
httpd_init_pcb(struct altcp_pcb *pcb, u16_t port)
{
	err_t err;

	if (pcb) {
		altcp_setprio(pcb, HTTPD_TCP_PRIO);
		/* set SOF_REUSEADDR here to explicitly bind httpd to multiple interfaces */
		err = altcp_bind(pcb, IP_ANY_TYPE, port);
		LWIP_UNUSED_ARG(err); /* in case of LWIP_NOASSERT */
		LWIP_ASSERT("httpd_init: tcp_bind failed", err == ERR_OK);
		pcb = altcp_listen(pcb);
		LWIP_ASSERT("httpd_init: tcp_listen failed", pcb != NULL);
		altcp_accept(pcb, http_accept);
	}
}

/**
 * @ingroup httpd
 * Initialize the httpd: set up a listening PCB and bind it to the defined port
 */
void
httpd_init(void)
{
    int i = 0;

#if HTTPD_ENABLE_HTTPS
    struct altcp_tls_config *tls_conf = NULL;
#endif
	struct altcp_pcb *pcb;

#if HTTPD_USE_MEM_POOL
	LWIP_MEMPOOL_INIT(HTTPD_STATE);
#if LWIP_HTTPD_SSI
	LWIP_MEMPOOL_INIT(HTTPD_SSI_STATE);
#endif
#endif
	/** AIR modification */
#if HTTPD_ENABLE_HTTPS
	LWIP_DEBUGF(HTTPD_DEBUG, ("httpd_init HTTPS\n"));

    httpd_inits(tls_conf);
#endif
	LWIP_DEBUGF(HTTPD_DEBUG, ("httpd_init\n"));

	/* LWIP_ASSERT_CORE_LOCKED(); is checked by tcp_new() */

	pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_ANY);
	LWIP_ASSERT("httpd_init: tcp_new failed", pcb != NULL);
	httpd_init_pcb(pcb, HTTPD_SERVER_PORT);

	/** AIR modification */
#if LWIP_HTTPD_CGI
	httpd_cgi_init();
#ifdef AIR_SUPPORT_MQTTD
    /* Create the mutex to protect the CGI function call */
    cgiMutex = xSemaphoreCreateMutex("cgi");
    if (NULL == cgiMutex)
    {
        return ERR_MEM;
    }
#endif
#endif
#if LWIP_HTTPD_SSI
	httpd_ssi_init();
#endif

    for(i = 0; i < HTTPD_MAX_LOGIN_NUM; i ++)
    {
        memset(http_cookies[i].cookie, 0, HTTPD_MAX_COOKIE_LEN);
        http_cookies[i].idleTime = 0;
    }
}

#if HTTPD_ENABLE_HTTPS
#define CRLF		"\r\n"
static unsigned char AIROHA_KEY[] =
"-----BEGIN PRIVATE KEY-----"CRLF
"MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQDHdrD5wEBinf2F"CRLF
"MBvbM6O1exwFoFKZzh49PlCLQ+0kC3vDJHC4YoTNDP/tjcOwPTFmj3Mc6fKyGpIN"CRLF
"0JOy6AsmkIwWkHmrNfiimu8/d3pzS0+DKTGfrhArshDdd+8sHzLDetchIYDASel6"CRLF
"toDXXWgzF8LABtsi9V6sUC7LiIVaKp9iBZL++zaOgyI+WSIJ5179nF29LC2EMMt/"CRLF
"u6uw4W5RGc2w5FvWtewCkF/mewZfqSD88uKYB0syno1tOnL/aVHCFhM8jaqTBnKs"CRLF
"mX01/nLAFIcLreAQvG2IUAaf6s6vLlJ/c5h+m82ck7o4Cjsdj8bpePxZcJanjwRt"CRLF
"B/fMWw/3AgMBAAECggEAf63TZVuSG05kvT/pZOxuS7otWtCgb0HIi417A1qMzvjm"CRLF
"1ShbWzv9JEDBBnArpoHVQIBswEJlD2sAuQUdtnTgxmPauIrsxxK6QGQK58Z0RJ8d"CRLF
"m6jf0gYa5c2LDCk8mTKzTPwx/0wx3WSyptFyMenhzGrWSBNbbMpCOuzst26BZhAm"CRLF
"R6elcdJ1xFI2l4lVY+TWJ5bS/VkPzFEGrGGTsC74dskLOiQDORJ0Ip5gqTPkbKLX"CRLF
"8GxsmfyK0cF3xCjhbyKkXTkOSiqgcfJlRAQ5Bc4syj7SkOh2PyhJvJShZvLkjYtc"CRLF
"A8AO25DLrrO3B7V1zk8JwAsU9h6BnBPW3Byy+ENMSQKBgQD1QAMrIQWbHwcQ7RX7"CRLF
"VVGlzm48i6aqYeBGGy98YzTLmbu4tUmWKET+WrjYejenWQSNDFbcTTHNCZmhADcS"CRLF
"fuFQCyrtvkJ0SztP6Ol9M7efiEQHPC35BiNqJiMDNll/xZJh+Di4Elr1b7jaMH/x"CRLF
"XiPRgMVTcdmDOvi5ldjyHbubnQKBgQDQNOcaab2XDJonHdgkBNwoD8DQiZvXAjYL"CRLF
"Aet14O6zrqy19K7b4yrfPD30kn22RjQSQc4jEYtNMW7A5/WkndJ28Q8qYUWAqiiL"CRLF
"FOm6ZJ1tFI6xTWQeFkSsVYw9oBWS49+a7gekxk1C3zTG1Iov8slE+JWOjB56BQC/"CRLF
"lDaHEIV3owKBgQCyaUrkyTUGz9+YFKF1IXAwKqhPK1ingmRhG83Ds3fo1YGWsWtC"CRLF
"7TCjyOPHt8Fc4IlLArr9sLqV6nH0ie+GaP8H99fW9B3r8/7F9y5EgqcGS18R1Fmt"CRLF
"WyXQRsYZxCP/q379/lFktxWsjwXu1HF+6XktI3xpY91UdkYS3EvidnbQkQKBgQC7"CRLF
"Ux2qcgCV6ky3bO6OWaEKmkHMXkLMC7074hpLEHkzSLEzdFXumFB2UOkdJr/cQwWO"CRLF
"d45TlvFSHmubPBeSaDx3ryMJ6kJyJKYdnE892FCbV6eadhrhxv86Xi2zNFi0tDj2"CRLF
"V7qg0Zmp1NDTI17BDRtw5ocInaC9/8pQk7ULiB3NKQKBgQCM9eRRZ3Ik8iqmz5Au"CRLF
"U8p6xd3VN8EehX3zPlra7wYbSWaG3TipckSnQc1cD5URZUyWaeprZVBDu9CBTO8i"CRLF
"MgIrgDE9LSSmsrfNnOD+wzg5cbmMaYwK2fX2gVXSBX9iVJofUp6HC7c5ZCaIoiw+"CRLF
"Din5AyJxIYxJWgaEk0NLRtmKYw=="CRLF
"-----END PRIVATE KEY-----"CRLF;

static unsigned char AIROHA_CRT[] =
"-----BEGIN CERTIFICATE-----"CRLF
"MIIDvjCCAqagAwIBAgIUbxcoGkZtHh2pwMsOX5Tv2BJehq8wDQYJKoZIhvcNAQEL"CRLF
"BQAwgYYxCzAJBgNVBAYTAlRXMQ8wDQYDVQQIDAZUYWl3YW4xDzANBgNVBAcMBkNo"CRLF
"dXBlaTEPMA0GA1UECgwGQWlyb2hhMQ0wCwYDVQQLDARTU0QxMSEwHwYJKoZIhvcN"CRLF
"AQkBFhJzc2QxQGFpcm9oYS5jb20udHcxEjAQBgNVBAMMCW1hZ2ljd2FuZDAeFw0y"CRLF
"MjA5MDYwMzIwMzNaFw0zMjA5MDMwMzIwMzNaMIGGMQswCQYDVQQGEwJUVzEPMA0G"CRLF
"A1UECAwGVGFpd2FuMQ8wDQYDVQQHDAZDaHVwZWkxDzANBgNVBAoMBkFpcm9oYTEN"CRLF
"MAsGA1UECwwEU1NEMTEhMB8GCSqGSIb3DQEJARYSc3NkMUBhaXJvaGEuY29tLnR3"CRLF
"MRIwEAYDVQQDDAltYWdpY3dhbmQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK"CRLF
"AoIBAQDHdrD5wEBinf2FMBvbM6O1exwFoFKZzh49PlCLQ+0kC3vDJHC4YoTNDP/t"CRLF
"jcOwPTFmj3Mc6fKyGpIN0JOy6AsmkIwWkHmrNfiimu8/d3pzS0+DKTGfrhArshDd"CRLF
"d+8sHzLDetchIYDASel6toDXXWgzF8LABtsi9V6sUC7LiIVaKp9iBZL++zaOgyI+"CRLF
"WSIJ5179nF29LC2EMMt/u6uw4W5RGc2w5FvWtewCkF/mewZfqSD88uKYB0syno1t"CRLF
"OnL/aVHCFhM8jaqTBnKsmX01/nLAFIcLreAQvG2IUAaf6s6vLlJ/c5h+m82ck7o4"CRLF
"Cjsdj8bpePxZcJanjwRtB/fMWw/3AgMBAAGjIjAgMB4GA1UdEQQXMBWCDTE5Mi4x"CRLF
"NjguMC4yNTWHBMCoAMgwDQYJKoZIhvcNAQELBQADggEBAGX9a8AK13Bg73VFrISi"CRLF
"fbaLxy6mesR8y5/jL2y0C23AM7P+1C+R5RmCybwxqsjZZLGQRHVTNvOpW7NzwqZJ"CRLF
"tGQf3XXZ9+3LfOvgYg56dXvgQd9KqUwy+9SdRsZh2jz2ABwJodjZXkUk0NoqsakX"CRLF
"/f4LZjRohuEhld31CKboEr9DyuvhzOajotl9DuTwAZFubavtRlbyHj+aVOp3+DbX"CRLF
"xVhxzgSNwXBwW0akQ1FwBY6goviKGZKF0ANMtxsQIlUxRvtvYiBccGg/TF3rDn7e"CRLF
"cdauanD6rSPIRjTU2SvYt/IF/ruJsjnYrG/sxqWFexLlESPCI2wNLrk64+GzbtZP"CRLF
"sbs="CRLF
"-----END CERTIFICATE-----"CRLF;

const char air_mbedtls_srv_pwd_pem[] = "";
const size_t air_mbedtls_srv_key_len = sizeof( AIROHA_KEY );
const size_t air_mbedtls_srv_pwd_len = sizeof( air_mbedtls_srv_pwd_pem ) - 1;
const size_t air_mbedtls_srv_crt_len = sizeof( AIROHA_CRT );
const char *air_mbedtls_srv_key = AIROHA_KEY;
const char *air_mbedtls_srv_pwd = air_mbedtls_srv_pwd_pem;
const char *air_mbedtls_srv_crt = AIROHA_CRT;
/**
 * @ingroup httpd
 * Initialize the httpd: set up a listening PCB and bind it to the defined port.
 * Also set up TLS connection handling (HTTPS).
 */
void
httpd_inits(struct altcp_tls_config *conf)
{
#if LWIP_ALTCP_TLS
    struct altcp_pcb *pcb_tls = NULL;

    LWIP_DEBUGF(HTTPD_DEBUG, ("Setup the server certificate and key\n"));
    conf = altcp_tls_create_config_server_privkey_cert((const u8_t*)air_mbedtls_srv_key, air_mbedtls_srv_key_len, NULL, 0,
            (const u8_t*)air_mbedtls_srv_crt, air_mbedtls_srv_crt_len);
    LWIP_ASSERT("httpd_init: configuring SSL failed", conf != NULL);

    LWIP_DEBUGF(HTTPD_DEBUG, ("Init altcp and mbedtls interfaces\n"));
    pcb_tls = altcp_tls_new(conf, IPADDR_TYPE_ANY);
    LWIP_ASSERT("httpd_init: altcp_tls_new failed", pcb_tls != NULL);
	httpd_init_pcb(pcb_tls, HTTPD_SERVER_PORT_HTTPS);
#else /* LWIP_ALTCP_TLS */
	LWIP_UNUSED_ARG(conf);
#endif /* LWIP_ALTCP_TLS */
}
#endif /* HTTPD_ENABLE_HTTPS */

#if LWIP_HTTPD_SSI
/**
 * @ingroup httpd
 * Set the SSI handler function.
 *
 * @param ssi_handler the SSI handler function
 * @param tags an array of SSI tag strings to search for in SSI-enabled files
 * @param num_tags number of tags in the 'tags' array
 */
void
http_set_ssi_handler(tSSIHandler ssi_handler, const char **tags, int num_tags)
{
	LWIP_DEBUGF(HTTPD_DEBUG, ("http_set_ssi_handler\n"));

	LWIP_ASSERT("no ssi_handler given", ssi_handler != NULL);
	httpd_ssi_handler = ssi_handler;

#if LWIP_HTTPD_SSI_RAW
	LWIP_UNUSED_ARG(tags);
	LWIP_UNUSED_ARG(num_tags);
#else /* LWIP_HTTPD_SSI_RAW */
	LWIP_ASSERT("no tags given", tags != NULL);
	LWIP_ASSERT("invalid number of tags", num_tags > 0);

	httpd_tags = tags;
	httpd_num_tags = num_tags;
#endif /* !LWIP_HTTPD_SSI_RAW */
}
#endif /* LWIP_HTTPD_SSI */

#if LWIP_HTTPD_CGI
/** AIR modification */
/**
 * Set an array of CGI filenames/handler functions
 *
 * @param cgis an array of CGI filenames/handler functions
 * @param num_handlers number of elements in the 'cgis' array
 */
#ifndef AIR_MW_SUPPORT
static const tCGI CGIURLs[] =
{
    {NULL, NULL, NULL}
};

/* This char array table is one-to-one mapping to SSI_TAGS_MAP_T enum table !! */
static const char* ppcTAGs[] =
{
    "NULL",
};

static u16_t SSIHandler(int iIndex, u16_t *length, void *pcb, unsigned int apiflags)
{
    if(pcb || apiflags)
    {
        *length = 0;
    }
    return 0;
}
#endif
/**
 * @ingroup httpd
 * Set an array of CGI filenames/handler functions
 *
 * @param cgis an array of CGI filenames/handler functions
 * @param num_handlers number of elements in the 'cgis' array
 */
void
http_set_cgi_handlers(const tCGI *cgis, int num_handlers)
{
	LWIP_ASSERT("no cgis given", cgis != NULL);
	LWIP_ASSERT("invalid number of handlers", num_handlers > 0);

	httpd_cgis = cgis;
	httpd_num_cgis = num_handlers;
}

/** AIR modification */
void httpd_cgi_init(void)
{
    int Num = 0;
#ifdef AIR_MW_SUPPORT
    Num = get_numCgiHandler();
#else
    Num = (sizeof(CGIURLs)/sizeof(tCGI) - 1);
#endif

    http_set_cgi_handlers(CGIURLs, Num);
    return;
}

#endif /* LWIP_HTTPD_CGI */

err_t httpd_post_begin(void* connection, const char* uri, const char* http_request,
	u16_t http_request_len, int content_len, char* response_uri,
	u16_t response_uri_len, u8_t* post_auto_wnd)
{
#if LWIP_HTTPD_CGI
	int i = 0;
#endif

	struct http_state* hs = (struct http_state*)connection;

	if (!uri || (uri[0] == '\0')) {
		return ERR_ARG;
	}

#if LWIP_HTTPD_CGI
	if(httpd_num_cgis && httpd_cgis)
	{
		for(i = 0; i < httpd_num_cgis; i++)
		{
			if(!strncmp(uri, httpd_cgis[i].pcCGIName, strlen(httpd_cgis[i].pcCGIName)))
			{
				hs->cgi_handler_index = i;
				break;
			}
		}

		if(!strcmp(uri, "/logon.cgi"))
		{
			hs->cgi_handler_index = -2;
		}
        else
        {
            LWIP_DEBUGF(HTTPD_DEBUG, ("%s[%d], Post [%s]\n", __func__, __LINE__, uri));
        }
	}

	if (i == httpd_num_cgis) {
		return ERR_ARG;
	}
#endif
	return ERR_OK;
}

static BOOL_T
_http_find_string_in_pbuf(
    char *ptr_src_string,
    const char *ptr_obj_string,
    struct pbuf *ptr_src_node,
    struct pbuf **pptr_tail_node,
    char **pptr_tail_offset,
    char *ptr_tail_remain)
{
    int ri = 0, si = 0;
    struct pbuf *ptr_node = ptr_src_node;
    char *ptr_cur_string = ptr_src_string;
    int len = 0, obj_len = strlen(ptr_obj_string);
    char remainLen = 0;
    BOOL_T found = FALSE;

    if((NULL == ptr_src_string) ||
        (NULL == ptr_obj_string) ||
        (NULL == ptr_src_node) ||
        (NULL == pptr_tail_node) ||
        (NULL == pptr_tail_offset) ||
        (NULL == ptr_tail_remain))
    {
        return found;
    }

    *pptr_tail_node = ptr_src_node;
    *pptr_tail_offset = ptr_src_string;
    len = ptr_node->len - (ptr_cur_string - (char*)ptr_node->payload);

    while(NULL != ptr_node)
    {
        for(si = 0; si < len; si++)
        {
            for(ri = 0; ri < obj_len; ri++)
            {
                if((ri + *ptr_tail_remain) == obj_len)
                {
                    /* Identify all characters in the second half of the target */
                    found = TRUE;
                    break;
                }
                if((si + ri) == len)
                {
                    /* Identify the first half of the characters in one of pbuf */
                    break;
                }
                if(ptr_obj_string[ri + *ptr_tail_remain] != ptr_cur_string[si + ri])
                {
                    *ptr_tail_remain = 0;
                    remainLen = 0;
                    break;
                }
                remainLen += 1;
            }
            if((TRUE == found) || (ri == obj_len) || ((si + ri) == len))
            {
                if((si + ri) == len)
                {
                    /* Continue comparing the displacement from the next pbuf */
                    *ptr_tail_remain = remainLen;
                }
                else
                {
                    found = TRUE;
                }
                break;
            }
        }
        if(TRUE == found)
        {
            *pptr_tail_node = ptr_node;
            *pptr_tail_offset = ptr_cur_string + si + ri;
            remainLen = 0;
            break;
        }

        ptr_node = ptr_node->next;
        ptr_cur_string = ptr_node->payload;
        len = ptr_node->len;
    }
    *ptr_tail_remain = remainLen;

    return found;
}

struct pbuf*
_http_post_parse_header(
    struct pbuf *ptr_q,
    struct http_state * ptr_hs)
{
    unsigned int head_boundary_len = 0;
    unsigned int intro_boundary_len = 0;
    struct pbuf *ptr_cur_node = ptr_q, *ptr_tail_node = NULL;
    char *ptr_cur_offt = ptr_q->payload, *ptr_tail_offset = NULL;
    BOOL_T b_found = FALSE;

    if (NULL == ptr_q || NULL == ptr_hs)
    {
        return NULL;
    }

    if(HTTP_POST_UPGRADE_DATATRANS_STAT > ptr_hs->receive_state)
    {
        if(HTTP_POST_UPGRADE_FILENAME_STAT == ptr_hs->receive_state)
        {
            /* Try to find filename */
            b_found = _http_find_string_in_pbuf((char*)ptr_cur_offt,
                    http_upgrade_context[ptr_hs->receive_state].desc,
                    ptr_cur_node,
                    &ptr_tail_node,
                    &ptr_tail_offset,
                    &http_upgrade_context[ptr_hs->receive_state].offt);
            if(TRUE == b_found)
            {
                ptr_cur_node = ptr_tail_node;
                ptr_cur_offt = ptr_tail_offset;
                ptr_hs->receive_state = HTTP_POST_UPGRADE_CONTENTTYPE_STAT;
            }
        }

        if(HTTP_POST_UPGRADE_CONTENTTYPE_STAT == ptr_hs->receive_state)
        {
            /* Try to find Content-Type */
            b_found = _http_find_string_in_pbuf((char*)ptr_cur_offt,
                    http_upgrade_context[ptr_hs->receive_state].desc,
                    ptr_cur_node,
                    &ptr_tail_node,
                    &ptr_tail_offset,
                    &http_upgrade_context[ptr_hs->receive_state].offt);
            if(TRUE == b_found)
            {
                ptr_cur_node = ptr_tail_node;
                ptr_cur_offt = ptr_tail_offset;
                ptr_hs->receive_state = HTTP_POST_UPGRADE_CRLFCRLF_STAT;
            }
        }

        if(HTTP_POST_UPGRADE_CRLFCRLF_STAT == ptr_hs->receive_state)
        {
            /* Try to find CRLFCRLF */
            b_found = _http_find_string_in_pbuf((char*)ptr_cur_offt,
                    http_upgrade_context[ptr_hs->receive_state].desc,
                    ptr_cur_node,
                    &ptr_tail_node,
                    &ptr_tail_offset,
                    &http_upgrade_context[ptr_hs->receive_state].offt);
            if(TRUE == b_found)
            {
                ptr_cur_node = ptr_tail_node;
                ptr_cur_offt = ptr_tail_offset;
                ptr_hs->receive_state = HTTP_POST_UPGRADE_DATATRANS_STAT;
                intro_boundary_len = ptr_cur_offt - (char *)ptr_cur_node->payload;
                _imgUploadStatus = FW_E_UPLOAD;
                _chunkSize = LWIP_HTTPD_MAX_SESSION_LEN;
                /* cut off head boundary */
                pbuf_header(ptr_cur_node, -intro_boundary_len);
            }
        }
    }
    return ptr_cur_node;
}

err_t httpd_post_receive_data(void* connection, struct pbuf* p)
{
    struct http_state* hs = (struct http_state*)connection;
    struct pbuf* q = p;
    unsigned int http_post_payload_full_flag = 0;
    int cookieLen = 0, ret = ERR_OK;
    int setLen = 0;

    if(NULL != q && hs->post_boundary_string)
    {
        q = _http_post_parse_header(q, hs);
        /* start rcv data*/
        if(HTTP_POST_UPGRADE_DATATRANS_STAT == hs->receive_state)
        {
            while(NULL != q)
            {
                setLen = (_chunkSize > q->len) ? q->len : _chunkSize;
                air_wdog_kick();
                WriteBuffer((unsigned char*)q->payload, setLen);
                q = q->next;
                _rcvLen += setLen;
                _chunkSize -= setLen;
            }
#ifdef LWIP_HTTPD_UPLOAD_MSG
            if(0 != _needLen)
            {
                printf("\rUploading - %d%%", (int)((_rcvLen * 100) / _needLen));
            }
#endif
            if(0 == hs->post_content_len_left)
            {
                hs->receive_state = HTTP_POST_UPGRADE_INIT_STAT;
                mem_free(hs->post_boundary_string);
                hs->post_boundary_string = NULL;
                _chunkSize = LWIP_HTTPD_MAX_SESSION_LEN;
                if(_rcvLen >= _needLen)
                {
                    _needLen = 0;
                    WriteLastBuffer();
                }
            }
        }
    }
    else
    {
        while (q != NULL)  /* copy data received in buffer to http_post_payload */
        {
            if (http_post_payload_len + q->len <= LWIP_HTTPD_POST_MAX_PAYLOAD_LEN)
            {
                MEMCPY(http_post_payload + http_post_payload_len, q->payload, q->len);
                http_post_payload_len += q->len;
            }
            else {  /* buffer overflow */
                http_post_payload_full_flag = 1;
                break;
            }
            q = q->next;
        }
    }
    pbuf_free(p); /* free pbuf */
    if (http_post_payload_full_flag) /* If buffer overflow, the data will be discarded. */
    {
        LWIP_DEBUGF(HTTPD_DEBUG, ("out of memery\n"));
        http_post_payload_full_flag = 0;
        http_post_payload_len = 0;
        hs->cgi_handler_index = -1;
        hs->response_file = NULL;
    }
    else if (hs->post_content_len_left == 0)
    {  /* POST data has been received  */
        LWIP_DEBUGF(HTTPD_DEBUG, ("http_post_payload=[%s]\n", http_post_payload));
        if(hs->cgi_handler_index != -1)
        {
            http_cgi_paramcount = extract_uri_parameters(hs, http_post_payload);
            if(hs->cgi_handler_index == -2)
            {
                hs->response_file = (char*)cgi_set_logon_info_handle(hs->param_vals);
                if(0 == strcmp(hs->response_file, "/index_re.html"))
                {
                    if(HTTPD_CONNECTION_FULL != _httpd_get_empty_cookie_index())
                    {
                        /* Login success, generate cookie value */
                        if(0 == (ret = _httpd_generate_cookies(http_cookies[http_cookie_index].cookie, &cookieLen)))
                        {
                            http_cookies[http_cookie_index].idleTime = 0;
                            LWIP_DEBUGF(HTTPD_DEBUG, ("New Cookie = %s\n", http_cookies[http_cookie_index].cookie));
                        }
                        else
                        {
                            LWIP_DEBUGF(HTTPD_DEBUG, ("Generate Cookie fail. ret[%d]\n", ret));
                            ret = ERR_ABRT;
                        }
                    }
                }
            }
            else
            {
#ifdef AIR_SUPPORT_MQTTD
                if (cgiMutex)
                {
                   xSemaphoreTake(cgiMutex, (1000 / portTICK_RATE_MS));
                }
#endif
                httpd_cgis[hs->cgi_handler_index].pfnCGIHandler(hs->cgi_handler_index, http_cgi_paramcount, hs->params, hs->param_vals);
#ifdef AIR_SUPPORT_MQTTD
                if (cgiMutex)
                {
                   xSemaphoreGive(cgiMutex);
                }
#endif
                hs->response_file = (char*)httpd_cgis[hs->cgi_handler_index].retUri;
            }
#if HTTPD_DBG_ON
            DEBUG(debugflags, "[%s] line [%d] return uri=[%s]\n", __FUNCTION__, __LINE__, hs->response_file);
#endif
        }
        else
        {
            hs->response_file = NULL;
        }
        http_post_payload_len = 0;
    }
    return ERR_OK;
}

void httpd_post_finished(void* connection, char* response_uri, u16_t response_uri_len)
{
	struct http_state* hs = (struct http_state*)connection;
	if(hs->response_file != NULL)
	{
		strncpy(response_uri, hs->response_file, response_uri_len);
	}
	return;
}

void httpd_ssi_init(void)
{
    int Num = 0;
#ifdef AIR_MW_SUPPORT
    Num = get_numSsiTag();
#else
    Num = sizeof(ppcTAGs)/sizeof(ppcTAGs[0]);
#endif

    http_set_ssi_handler(SSIHandler, ppcTAGs, Num);
    return;
}

/* LOCAL SUBPROGRAM BODIES
*/
BOOL_T
_http_find_cookie(
    char *ptr_data)
{
    int i = 0;
    BOOL_T ret = FALSE;
    char *ptr_req_cookie = NULL;

    if(NULL != (ptr_req_cookie = strstr(ptr_data, "Cookies=")))
    {
        ptr_req_cookie += strlen("Cookies=");
        for(i = 0; i < HTTPD_MAX_LOGIN_NUM; i ++)
        {
            if((0 != strlen(http_cookies[i].cookie)) &&
               (0 == strncmp(ptr_req_cookie, http_cookies[i].cookie, strlen(http_cookies[i].cookie))))
            {
                if(ptr_req_cookie[HTTPD_MAX_COOKIE_LEN - 1] == '\r')
                {
                    /* Reset cookie idle time */
                    http_cookies[i].idleTime = 0;
                    ret = TRUE;
                }
                break;
            }
        }
    }
    else
    {
        LWIP_DEBUGF(HTTPD_DEBUG, ("Can't find Cookies=\n"));
    }

    return ret;
}

unsigned char
_httpd_get_empty_cookie_index(
    void)
{
    for(http_cookie_index = 0; http_cookie_index < HTTPD_MAX_LOGIN_NUM; http_cookie_index ++)
    {
        if(0 == http_cookies[http_cookie_index].cookie[0])
        {
            break;
        }
    }
    if(http_cookie_index == HTTPD_MAX_LOGIN_NUM)
    {
        http_cookie_index = HTTPD_CONNECTION_FULL;
    }

    return http_cookie_index;
}

int
_httpd_generate_cookies(
    char *ptr_outBuf,
    int *ptr_outLen)
{
    unsigned char randomBuf[HTTPD_BASE64_CONVERT_LEN + 1] = {0};
    int i = 0;

    srand(sys_now());
    for(i = 0; i < HTTPD_BASE64_CONVERT_LEN; i ++)
    {
        randomBuf[i] = (rand() % 0xFF);
    }
    return mbedtls_base64_encode(ptr_outBuf, HTTPD_MAX_COOKIE_LEN, ptr_outLen, randomBuf, (HTTPD_BASE64_CONVERT_LEN + 1));
}

void
httpd_tmr(
    void)
{
    int i = 0;
    /* Caculate cookie idle timer */
    for(i = 0; i < HTTPD_MAX_LOGIN_NUM; i++)
    {
        if(0 != http_cookies[i].cookie[0])
        {
            http_cookies[i].idleTime++;
            if(HTTPD_COOKIE_TIMEOUT <= http_cookies[i].idleTime)
            {
                memset(http_cookies[i].cookie, 0, HTTPD_MAX_COOKIE_LEN);
                http_cookies[i].idleTime = 0;
            }
        }
    }
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
