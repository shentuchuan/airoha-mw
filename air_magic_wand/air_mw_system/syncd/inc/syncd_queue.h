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

/* FILE NAME:  syncd_queue.h
 * PURPOSE:
 *      It provides syncd internal queue functions.
 *
 * NOTES:
 */

#ifndef _SYNCD_QUEUE_H_
#define _SYNCD_QUEUE_H_

/* INCLUDE FILE DECLARATIONS
 */
#include "mw_error.h"
#include "osapi_message.h"
#include "syncd_msg.h"

/* NAMING CONSTANT DECLARATIONS
*/
#define SYNCD_QUEUE_SET_NAME                  "SQset"

/* The name of the queue for the interaction with the SYNCD task. */
#define SYNCD_QUEUE_NAME_TASK                 "ScT"
/* The name of the queue for non-blocking waiting for the response from DB. */
#define SYNCD_QUEUE_NAME_DB_NBLOCK_WAITRSP    "SDN"
/* The name of the queue for blocking waiting for the response from DB. */
#define SYNCD_QUEUE_NAME_DB_BLOCK_WAITRSP     "SDB"

#define SYNCD_QUEUE_TASK_LEN                  (2)
#define SYNCD_QUEUE_DB_NBLOCK_WAITRSP_LEN     (128)
#define SYNCD_QUEUE_DB_BLOCK_WAITRSP_LEN      (1)
#define SYNCD_QUEUE_SET_NAME_LEN              (SYNCD_QUEUE_TASK_LEN + SYNCD_QUEUE_DB_NBLOCK_WAITRSP_LEN)

/* MACRO FUNCTION DECLARATIONS
*/
#define SYNCD_OPER_CHECK_CHANGE(old, new, method, tid, fid, eid)          \
({                                                                              \
    BOOL_T _is_change = FALSE;                                                  \
    if ((old) != (new))                                                         \
    {                                                                           \
        if (MW_E_OK == syncd_queue_db_send(NULL, (method), (tid), (fid), (eid), &(new), MSG_TIMEOUT_RETRUN_IMMEDIATELY)) \
        {                                                                       \
            (old) = (new);                                                      \
            _is_change = TRUE;                                                  \
        }                                                                       \
    }                                                                           \
    _is_change;                                                                 \
})

/* DATA TYPE DECLARATIONS
*/
typedef enum
{
    /* DB will send no response back. */
    SYNCD_QUEUE_DB_NONBLOCK_WITHOUTRSP,
    /* The response is indicated to be sent to SYNCD_QUEUE_NAME_DB_NBLOCK_WAITRSP. */
    SYNCD_QUEUE_DB_NONBLOCK_WITHRSP,
    /* The response is indicated to be sent to SYNCD_QUEUE_NAME_DB_BLOCK_WAITRSP. */
    SYNCD_QUEUE_DB_BLOCK,

    SYNCD_QUEUE_DB_LAST
} SYNCD_QUEUE_DB_BLOCK_ENUM;

typedef struct
{
    msghandle_t queue_handle_task;
    msghandle_t queue_handle_dbnb;
    staticMsghandle_t taskQueueStruct; /* Task queue is a static queue. */
    /* ptr_taskQueueStorage must be 4-bytes aligned. */
    MSG_SYNCD_T *taskQueueStorage[0]; /* Task queue is a static queue. */
} SYNCD_QUEUE_CONTEXT_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

#endif  /*_SYNCD_QUEUE_H_*/
