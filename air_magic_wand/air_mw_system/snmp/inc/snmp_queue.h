/*******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Airoha Technology Corp. (C) 2023
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

/* FILE NAME:  snmp_queue.h
 * PURPOSE:
 *      It provides snmp internal queue functions.
 *
 * NOTES:
 */

#ifndef _SNMP_QUEUE_H_
#define _SNMP_QUEUE_H_

/* INCLUDE FILE DECLARATIONS
 */
#include "mw_error.h"
#include "db_api.h"
/* NAMING CONSTANT DECLARATIONS
*/
#define SNMP_QUEUE_CLI         "snm"
#define SNMP_QUEUE_LEN         (128)
#define SNMP_QUEUE_TIMEOUT     (0xFFFFFFFF)
#define SNMP_ACCEPTMBOX_SIZE   (4)

/* MACRO FUNCTION DECLARATIONS
*/

/* DATA TYPE DECLARATIONS
*/

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/* FUNCTION NAME:   snmp_queue_init
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
 *      MW_E_NOT_INITED
 *
 * NOTES:
 *      None
 */
MW_ERROR_NO_T inline snmp_queue_init();

/* FUNCTION NAME:   snmp_queue_free
 * PURPOSE:
 *      Release all allocated memory in snmp queue.
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
void snmp_queue_free();

/* FUNCTION NAME:   snmp_queue_recv
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
 *      None
 */
MW_ERROR_NO_T snmp_queue_recv(void **pptr_buf);

/* FUNCTION NAME:   snmp_queue_send
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
MW_ERROR_NO_T snmp_queue_send(const UI8_T method, const UI8_T t_idx, const UI8_T f_idx, const UI16_T e_idx, const void *ptr_data, const UI16_T size, DB_MSG_T **pptr_out_msg);

/* FUNCTION NAME:   snmp_queue_setData
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
MW_ERROR_NO_T snmp_queue_setData(const UI8_T method, const UI8_T t_idx, const UI8_T f_idx, const UI16_T e_idx, const void *ptr_data, const UI16_T size);

/* FUNCTION NAME:   snmp_queue_getData
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
 */
MW_ERROR_NO_T snmp_queue_getData(const UI8_T in_t_idx, const UI8_T in_f_idx, const UI16_T in_e_idx, DB_MSG_T **pptr_out_msg, UI16_T *ptr_out_size, void **pptr_out_data);

#endif  /*_SNMP_QUEUE_H_*/
