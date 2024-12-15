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

/* FILE NAME:  mqttd_queue.c
 * PURPOSE:
 *  Implement internal queue function of mqttd daemon.
 *
 * NOTES:
 *
 */

#include <string.h>
#include "mqttd.h"
#include "mqttd_queue.h"

#include "mw_error.h"
#include "osapi.h"
#include "osapi_memory.h"
#include "osapi_message.h"
#include "osapi_string.h"
#include "db_api.h"

/* NAMING CONSTANT DECLARATIONS
*/

/* MACRO FUNCTION DECLARATIONS
 */
/* MQTTD Client Daemon Queue
*/

/* DATA TYPE DECLARATIONS
*/

/* GLOBAL VARIABLE DECLARATIONS
*/

/* LOCAL SUBPROGRAM SPECIFICATIONS
*/

/* STATIC VARIABLE DECLARATIONS
 */

/* LOCAL SUBPROGRAM BODIES
 */

/* EXPORTED SUBPROGRAM BODIES
 */
/* FUNCTION NAME: mqttd_queue_init
 * PURPOSE:
 *      Initialize DB communication message receiver.
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
 *      MW_E_NOT_INITED
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T inline
mqttd_queue_init()
{
    MW_ERROR_NO_T rc = MW_E_OK;

    /* Create a queue for the non-blocking interaction with the DB task */
    rc = osapi_msgCreate(
        MQTTD_QUEUE_NAME,
        MQTTD_QUEUE_LEN,
        MQTTD_ACCEPTMBOX_SIZE);
    if (MW_E_OK != rc)
    {
        return MW_E_NOT_INITED;
    }
    
    return MW_E_OK;
}
MW_ERROR_NO_T inline
mqttd_get_queue_init()
{
    MW_ERROR_NO_T rc = MW_E_OK;
    
    /* Create a queue for the blocking interaction with the DB task */
    rc = osapi_msgCreate(
        MQTTD_GET_QUEUE_NAME,
        MQTTD_QUEUE_LEN,
        MQTTD_ACCEPTMBOX_SIZE);
    if (MW_E_OK != rc)
    {
        return MW_E_NOT_INITED;
    }
    return MW_E_OK;
}

/* FUNCTION NAME: mqttd_queue_free
 * PURPOSE:
 *      Release all allocated memory in mqttd queue.
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
mqttd_queue_free()
{
    MW_ERROR_NO_T rc = MW_E_OK;
    UI8_T *ptr_msg = NULL;

    /* Flush the queue message */
    do
    {
        rc = dbapi_recvMsg(
                MQTTD_QUEUE_NAME,
                &ptr_msg,
                MQTTD_QUEUE_TIMEOUT);
        if (MW_E_OK == rc)
        {
            osapi_free(ptr_msg);
        }
    }while(MW_E_OK == rc);
    osapi_msgDelete(MQTTD_QUEUE_NAME);
}

void
mqttd_get_queue_free()
{
    MW_ERROR_NO_T rc = MW_E_OK;
    UI8_T *ptr_msg = NULL;

    /* Flush the queue message */
    do
    {
        rc = dbapi_recvMsg(
                MQTTD_GET_QUEUE_NAME,
                &ptr_msg,
                MQTTD_QUEUE_TIMEOUT);
        if (MW_E_OK == rc)
        {
            osapi_free(ptr_msg);
        }
    }while(MW_E_OK == rc);
    osapi_msgDelete(MQTTD_GET_QUEUE_NAME);
}

/* FUNCTION NAME: mqttd_queue_recv
 * PURPOSE:
 *      Receive DB communication message from DB.
 *
 * INPUT:
 *      None
 *
 * OUTPUT:
 *      ptr_buf     --  pointer to pointer of receiving buffer
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_BAD_PARAMETER
 *      MW_E_ENTRY_NOT_FOUND
 *
 * NOTES:
 *      This function uses blocking mode to wait the message sent from DB.
 */
MW_ERROR_NO_T
mqttd_queue_recv(
    void **ptr_buf)
{
    MW_ERROR_NO_T rc;
    UI8_T *ptr_msg = NULL;

    rc = dbapi_recvMsg(
        MQTTD_QUEUE_NAME,
        &ptr_msg,
        MQTTD_QUEUE_BLOCKTIMEOUT);
    if (MW_E_OK != rc)
    {
        return rc;
    }

    //mqttd_debug_db("ptr_msg=%p", ptr_msg);
    (*ptr_buf) = ptr_msg;

    return MW_E_OK;
}

MW_ERROR_NO_T
mqttd_get_queue_recv(
    void **ptr_buf)
{
    MW_ERROR_NO_T rc;
    UI8_T *ptr_msg = NULL;

    rc = dbapi_recvMsg(
        MQTTD_GET_QUEUE_NAME,
        &ptr_msg,
        MQTTD_QUEUE_BLOCKTIMEOUT);
    if (MW_E_OK != rc)
    {
        return rc;
    }

    //mqttd_debug_db("ptr_msg=%p", ptr_msg);
    (*ptr_buf) = ptr_msg;

    return MW_E_OK;
}

/* FUNCTION NAME: mqttd_queue_send
 * PURPOSE:
 *      package message and call sending function to DB.
 *
 * INPUT:
 *      method          --  the method bitmap
 *      t_idx           --  the enum of the table
 *      f_idx           --  the enum of the field
 *      e_idx           --  the entry index in the table
 *      ptr_data        --  pointer to message data
 *      size            --  size of ptr_data
 *
 * OUTPUT:
 *      pptr_out_msg    -- double pointer to db message
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_BAD_PARAMETER
 *      MW_E_OP_INCOMPLETE
 *      MW_E_NO_MEMORY
 *
 * NOTES:
 *      The input parameters are depend on structure of DB.
 *      Please refer to db_api.h
 */
MW_ERROR_NO_T
mqttd_queue_send(
    const UI8_T method,
    const UI8_T t_idx,
    const UI8_T f_idx,
    const UI16_T e_idx,
    const void *ptr_data,
    const UI16_T size,
    DB_MSG_T **pptr_out_msg)
{
    MW_ERROR_NO_T   rc = MW_E_OK;
    DB_MSG_T        *ptr_msg = NULL;
    DB_PAYLOAD_T    *ptr_payload = NULL;
    UI32_T          msg_size;
    UI16_T          total_size = 0;

    if (NULL == osapi_msgFindHandle(MQTTD_QUEUE_NAME))
    {
        mqttd_debug_db("mqttd queue does not exist");
        return MW_E_NOT_INITED;
    }
    MW_PARAM_CHK((t_idx >= TABLES_LAST), MW_E_BAD_PARAMETER);
    if (size > 0 && method != M_GET)
    {
        DB_REQUEST_TYPE_T request = {
            .t_idx = t_idx,
            .f_idx = f_idx,
            .e_idx = e_idx
        };

        rc = dbapi_getDataSize(request, &total_size);
        if (MW_E_OK != rc)
        {
           return rc;
        }

        if (size > total_size)
        {
           return MW_E_OP_INCOMPLETE;
        }
        msg_size = DB_MSG_HEADER_SIZE + DB_MSG_PAYLOAD_SIZE + total_size;
    }
    else
    {
        msg_size = DB_MSG_HEADER_SIZE + DB_MSG_PAYLOAD_SIZE + size;
    }
    rc = osapi_calloc(
            msg_size,
            MQTTD_QUEUE_NAME,
            (void **)&ptr_msg);
    if (MW_E_OK != rc)
    {
        osapi_printf("%s: allocate memory failed(%d)\n", __func__, rc);
        return MW_E_NO_MEMORY;
    }

    /* message */
    dbapi_setMsgHeader(ptr_msg, MQTTD_QUEUE_NAME, method, 1);
    //mqttd_debug_db("method=0x%X", ptr_msg ->method);

    /* payload */
    ptr_payload = (DB_PAYLOAD_T *)&(ptr_msg ->ptr_payload);
    dbapi_setMsgPayload(method, t_idx, f_idx, e_idx, ptr_data, (void *)ptr_payload);
    /*mqttd_debug_db("T/F/E=%u/%u/%u", ptr_payload ->request.t_idx,
                                    ptr_payload ->request.f_idx,
                                    ptr_payload ->request.e_idx);
    mqttd_debug_db("data_size=%u", ptr_payload ->data_size);*/

    /* Send message to DB */

    rc = dbapi_sendMsg(ptr_msg, MQTTD_QUEUE_TIMEOUT);
    if (MW_E_OK != rc)
    {
        osapi_printf("%s: Send message to DB failed(%d)\n", __func__, rc);
        return rc;
    }

    (*pptr_out_msg) = ptr_msg;
    return MW_E_OK;
}

MW_ERROR_NO_T
mqttd_get_queue_send(
    const UI8_T method,
    const UI8_T t_idx,
    const UI8_T f_idx,
    const UI16_T e_idx,
    const void *ptr_data,
    const UI16_T size,
    DB_MSG_T **pptr_out_msg)
{
    MW_ERROR_NO_T   rc = MW_E_OK;
    DB_MSG_T        *ptr_msg = NULL;
    DB_PAYLOAD_T    *ptr_payload = NULL;
    UI32_T          msg_size;
    UI16_T          total_size = 0;

    if (NULL == osapi_msgFindHandle(MQTTD_GET_QUEUE_NAME))
    {
        mqttd_debug_db("mqttd queue does not exist");
        return MW_E_NOT_INITED;
    }
    MW_PARAM_CHK((t_idx >= TABLES_LAST), MW_E_BAD_PARAMETER);
    if (size > 0 && method != M_GET)
    {
        DB_REQUEST_TYPE_T request = {
            .t_idx = t_idx,
            .f_idx = f_idx,
            .e_idx = e_idx
        };

        rc = dbapi_getDataSize(request, &total_size);
        if (MW_E_OK != rc)
        {
           return rc;
        }

        if (size > total_size)
        {
           return MW_E_OP_INCOMPLETE;
        }
        msg_size = DB_MSG_HEADER_SIZE + DB_MSG_PAYLOAD_SIZE + total_size;
    }
    else
    {
        msg_size = DB_MSG_HEADER_SIZE + DB_MSG_PAYLOAD_SIZE + size;
    }
    rc = osapi_calloc(
            msg_size,
            MQTTD_GET_QUEUE_NAME,
            (void **)&ptr_msg);
    if (MW_E_OK != rc)
    {
        osapi_printf("%s: allocate memory failed(%d)\n", __func__, rc);
        return MW_E_NO_MEMORY;
    }

    /* message */
    dbapi_setMsgHeader(ptr_msg, MQTTD_GET_QUEUE_NAME, method, 1);
    //mqttd_debug_db("method=0x%X", ptr_msg ->method);

    /* payload */
    ptr_payload = (DB_PAYLOAD_T *)&(ptr_msg ->ptr_payload);
    dbapi_setMsgPayload(method, t_idx, f_idx, e_idx, ptr_data, (void *)ptr_payload);
    /*mqttd_debug_db("T/F/E=%u/%u/%u", ptr_payload ->request.t_idx,
                                    ptr_payload ->request.f_idx,
                                    ptr_payload ->request.e_idx);
    mqttd_debug_db("data_size=%u", ptr_payload ->data_size);*/

    /* Send message to DB */

    rc = dbapi_sendMsg(ptr_msg, MQTTD_QUEUE_TIMEOUT);
    if (MW_E_OK != rc)
    {
        osapi_printf("%s: Send message to DB failed(%d)\n", __func__, rc);
        return rc;
    }

    (*pptr_out_msg) = ptr_msg;
    return MW_E_OK;
}

/* FUNCTION NAME: mqttd_queue_setData
 * PURPOSE:
 *      package message and call sending function to DB directly.
 *
 * INPUT:
 *      method      --  the method bitmap
 *      t_idx       --  the enum of the table
 *      f_idx       --  the enum of the field
 *      e_idx       --  the entry index in the table
 *      ptr_data    --  pointer to message data
 *      size        --  size of ptr_data
 *
 * OUTPUT:
 *      None
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_BAD_PARAMETER
 *      MW_E_OP_INCOMPLETE
 *      MW_E_NO_MEMORY
 *
 * NOTES:
 *      The input parameters are depend on structure of DB.
 *      Please refer to db_api.h
 */
MW_ERROR_NO_T
mqttd_queue_setData(
    const UI8_T method,
    const UI8_T t_idx,
    const UI8_T f_idx,
    const UI16_T e_idx,
    const void *ptr_data,
    const UI16_T size)
{
    MW_ERROR_NO_T   rc = MW_E_OK;
    DB_MSG_T        *ptr_msg = NULL;

    rc = mqttd_get_queue_send(method, t_idx, f_idx, e_idx, ptr_data, size, &ptr_msg);
    if (MW_E_OK != rc)
    {
        osapi_printf("%s: mqttd_queue_send failed(%d)\n", __func__, rc);
    }
    return rc;
}

/* FUNCTION NAME: mqttd_queue_getData
 * PURPOSE:
 *      1. Calculate db data size based on tid,fid,eid and then alloc memory
 *      2. Send db queue and wait db response
 *
 * INPUT:
 *      t_idx           --  the enum of the table
 *      f_idx           --  the enum of the field
 *      e_idx           --  the entry index in the table
 *
 * OUTPUT:
 *      pptr_out_msg    --  double pointer to db message
 *      ptr_out_size    --  pointer to size of ptr_data
 *      pptr_out_data   --  double pointer to db data in db payload
 *
 * RETURN:
 *      MW_E_OK
 *      MW_E_ENTRY_NOT_FOUND
 *      MW_E_TIMEOUT
 *      MW_E_OP_INCOMPLETE
 *      MW_E_NO_MEMORY
 *      MW_E_OTHERS
 *
 * NOTES:
 *      When return MW_E_OK, caller need to free the memory which pointed by ptr_out_msg!
 *      This function should only be called before the MQTTD Running State
 */
MW_ERROR_NO_T
mqttd_queue_getData(
    const UI8_T in_t_idx,
    const UI8_T in_f_idx,
    const UI16_T in_e_idx,
    DB_MSG_T **pptr_out_msg,
    UI16_T *ptr_out_size,
    void **pptr_out_data)
{
    MW_ERROR_NO_T   rc = MW_E_OK;
    DB_MSG_T        *ptr_msg = NULL;
    UI16_T           total_size = 0;
    DB_PAYLOAD_T    *ptr_pload = NULL;

    DB_REQUEST_TYPE_T request = {
        .t_idx = in_t_idx,
        .f_idx = in_f_idx,
        .e_idx = in_e_idx
    };

    rc = dbapi_getDataSize(request, &total_size);
    if (MW_E_OK != rc)
    {
       mqttd_debug_db("dbapi_getDataSize failed(%d)\n", rc);
       return rc;
    }

    rc = mqttd_get_queue_send(M_GET, in_t_idx, in_f_idx, in_e_idx, NULL, total_size, &ptr_msg);
    if (MW_E_OK != rc)
    {
       mqttd_debug_db("mqttd_queue_send failed(%d)\n", rc);
       return rc;
    }
	//osapi_printf("mqttd_queue_send: %p\n", ptr_msg);
    /* wait for DB response messgae */
    rc = mqttd_get_queue_recv((void **)&ptr_msg);
    if(MW_E_OK == rc)
    {
        //mqttd_debug_db("mqttd_queue_recv success \n");
    }
    else
    {
        mqttd_debug_db("mqttd_queue_recv failed(%d) \n", rc);
        osapi_free(ptr_msg);
        return rc;
    }

    (*pptr_out_msg) = ptr_msg;
    (*ptr_out_size) = total_size;

    ptr_pload = (DB_PAYLOAD_T *)&(ptr_msg->ptr_payload);
    (*pptr_out_data) = &(ptr_pload->ptr_data);

    //mqttd_debug_db("*pptr_out_msg = %p, *pptr_out_data = %p \n", *pptr_out_msg, *pptr_out_data);

    return MW_E_OK;
}

