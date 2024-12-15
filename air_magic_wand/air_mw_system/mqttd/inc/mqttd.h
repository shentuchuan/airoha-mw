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

/* FILE NAME:  mqttd.h
 * PURPOSE:
 *      It provides mqtt client daemon.
 *
 * NOTES:
 */

#ifndef _MQTTD_H_
#define _MQTTD_H_

/* INCLUDE FILE DECLARATIONS
 */
#include "mw_error.h"
#include "mw_types.h"
/* NAMING CONSTANT DECLARATIONS
*/

/* MACRO FUNCTION DECLARATIONS
*/

#ifdef MQTT_EASY_DUMP

#define mqttd_debug      	printf
#define mqttd_debug_pkt     printf
#define mqttd_debug_db      printf


#else

#define mqttd_debug(fmt, ...)  do { \
                                if (mqttd_debug_level == MQTTD_DEBUG_ALL) { \
                                osapi_printf("<%s:%d>(%s)" fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
                                }} while (0)

#define mqttd_debug_pkt(fmt, ...)  do { \
                                if (mqttd_debug_level & MQTTD_DEBUG_PKT) { \
                                osapi_printf("(%s)" fmt "\n", __func__, ##__VA_ARGS__ ); \
                                }} while (0)

#define mqttd_debug_db(fmt, ...)  do { \
                                if (mqttd_debug_level & MQTTD_DEBUG_DB) { \
                                osapi_printf("(%s)" fmt "\n", __func__, ##__VA_ARGS__ ); \
                                }} while (0)

#endif

typedef enum
{
    MQTTD_DEBUG_DISABLE = 0,
    MQTTD_DEBUG_PKT,
    MQTTD_DEBUG_DB,
    MQTTD_DEBUG_ALL,
    MQTTD_DEBUG_LAST
} MQTTD_DEBUG_LEVEL_T;

typedef enum
{
    MQTTD_PORT_VLAN_NONE = 0,
    MQTTD_PORT_VLAN_ACCESS,
    MQTTD_PORT_VLAN_TRUNK,
    MQTTD_PORT_VLAN_HYBRID,
    MQTTD_PORT_VLAN_TNUNEL,
    MQTTD_PORT_VLAN_LAST
} MQTTD_PORT_VLAN_E;

/* DATA TYPE DECLARATIONS
*/
UI8_T mqttd_debug_level;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
/* FUNCTION NAME: mqttd_init
 * PURPOSE:
 *      Initialize MQTT client daemon
 *
 * INPUT:
 *      arg  --  the remote server IP
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_NOT_INITED
 *
 * NOTES:
 *      If connect to default remote MQTT server,  then set arg to NULL.
 */
MW_ERROR_NO_T
mqttd_init(
    void *arg);

/* FUNCTION NAME: mqttd_dump_topic
 * PURPOSE:
 *      Dump all mqtt supported topics
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      None
 *
 * NOTES:
 *      For debug using only
 */
void
mqttd_dump_topic(
    void);

/* FUNCTION NAME: mqttd_debug_enable
 * PURPOSE:
 *      To enable or disable to print debug message
 *
 * INPUT:
 *      level    --  The debugging level
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      None
 *
 * NOTES:
 *      None
 */
void
mqttd_debug_enable(
    UI8_T level);

/* FUNCTION NAME: mqttd_show_state
 * PURPOSE:
 *      To show the mqttd status
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      None
 *
 * NOTES:
 *      None
 */
void
mqttd_show_state(
    void);

/* FUNCTION NAME: mqttd_shutdown
 * PURPOSE:
 *      To shutdown the MQTTD
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      None
 *
 * NOTES:
 *      None
 */
void
mqttd_shutdown(
    void);

/* FUNCTION NAME: mqttd_get_state
 * PURPOSE:
 *      To get the MQTTD status
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      BOOL_T
 *
 * NOTES:
 *      None
 */
UI8_T
mqttd_get_state(
    void);

void *mqtt_malloc(UI32_T size);
void mqtt_free(void *ptr);
void *mqtt_realloc(void *ptr, UI32_T size);



#endif  /*_MQTTD_H_*/
