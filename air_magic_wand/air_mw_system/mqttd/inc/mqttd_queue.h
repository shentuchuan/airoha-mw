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

/* FILE NAME:  mqttd_queue.h
 * PURPOSE:
 *      It provides mqttd internal queue functions.
 *
 * NOTES:
 */

#ifndef _MQTTD_QUEUE_H_
#define _MQTTD_QUEUE_H_

/* INCLUDE FILE DECLARATIONS
 */
#include "mw_error.h"
#include "db_api.h"
/* NAMING CONSTANT DECLARATIONS
 */
#define MQTTD_QUEUE_NAME "mqd"
#define MQTTD_GET_QUEUE_NAME "mqg"
#define MQTTD_TO_HTTPC_QUEUE_NAME "mth"
#define HTTPC_TO_MQTTD_QUEUE_NAME "htm"
#define MQTTD_TIMER_QUEUE_NAME "mqt"
#define MQTTD_QUEUE_LEN (18)
#define MQTTD_HTTPC_QUEUE_LEN (8)
#define MQTTD_QUEUE_BLOCKTIMEOUT (0xFFFFFFFF)
#define MQTTD_QUEUE_TIMEOUT (100)
#define MQTTD_ACCEPTMBOX_SIZE (4)

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

MW_ERROR_NO_T inline mqttd_queue_init();
void mqttd_queue_free();
MW_ERROR_NO_T mqttd_httpc_queue_recv(void **ptr_buf);
MW_ERROR_NO_T mqttd_queue_recv(void **ptr_buf);
MW_ERROR_NO_T mqttd_queue_send(const UI8_T method, const UI8_T t_idx, const UI8_T f_idx, const UI16_T e_idx, const void *ptr_data, const UI16_T size, DB_MSG_T **pptr_out_msg);
MW_ERROR_NO_T mqttd_queue_setData(const UI8_T method, const UI8_T t_idx, const UI8_T f_idx, const UI16_T e_idx, const void *ptr_data, const UI16_T size);
MW_ERROR_NO_T mqttd_queue_getData(const UI8_T in_t_idx, const UI8_T in_f_idx, const UI16_T in_e_idx, DB_MSG_T **pptr_out_msg, UI16_T *ptr_out_size, void **pptr_out_data);
MW_ERROR_NO_T mqttd_tmr_queue_setData(const UI8_T method, const UI8_T t_idx, const UI8_T f_idx, const UI16_T e_idx, const void *ptr_data, const UI16_T size);
MW_ERROR_NO_T mqttd_tmr_queue_getData(const UI8_T in_t_idx, const UI8_T in_f_idx, const UI16_T in_e_idx, DB_MSG_T **pptr_out_msg, UI16_T *ptr_out_size, void **pptr_out_data);
MW_ERROR_NO_T mqttd2httpc_queue_recv(void **ptr_buf);
MW_ERROR_NO_T mqttd2httpc_queue_send(UI8_T *ptr_msg);
MW_ERROR_NO_T httpc2mqttd_queue_recv(void **ptr_buf);
MW_ERROR_NO_T httpc2mqttd_queue_send(UI8_T *ptr_msg);

#endif /*_MQTTD_QUEUE_H_*/
