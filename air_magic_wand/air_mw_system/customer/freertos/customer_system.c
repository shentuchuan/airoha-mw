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

/* FILE NAME:  customer_system.c
 * PURPOSE:
 *  Specify customer system configuration.
 *
 * NOTES:
 *
 */

/* INCLUDE FILE DECLARTIONS
*/
#include "customer_system.h"
#include "hwcfg_util.h"
#include "util.h"
#include "air_chipscu.h"
#include "air_gpio.h"
#include "air_error.h"
#include "air_init.h"
#include "air_port.h"
#include <platform.h>
#include <osal/osal_type.h>
#include <osal/osal_lib.h>
#include "mw_utils.h"
#include "mw_tlv.h"
#include "btn_reset.h"
#include "osapi_string.h"
#include "air_perif.h"


/* NAMING CONSTANT DECLARATIONS
*/
/******* EN8853C_RFB start *******/
/* OUTPUT */
#define EN8853C_RFB_PIN_SYS_LED               (GPIO_PIN0)
#define EN8853C_RFB_PIN_RESET_EN8808_1        (GPIO_PIN12)
#define EN8853C_RFB_PIN_RESET_EN8808_2        (GPIO_PIN1)
#define EN8853C_RFB_PIN_RESET_EN8804          (GPIO_PIN7)
#define EN8853C_RFB_PIN_RESET_LED             (GPIO_PIN4)

/* INPUT */
#define EN8853C_RFB_PIN_RESET_SW              (GPIO_PIN9)
#define EN8853C_RFB_PIN_PCA9535_INT           (GPIO_PIN11)
/******* EN8853C_RFB end *******/

/******* EN8853C_24P_EN8804_4P_4SFP start *******/
/* OUTPUT */
#define EN8853C_24P_EN8804_4P_4SFP_PIN_SYS_LED               (GPIO_PIN0)
#define EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_1        (GPIO_PIN12)
#define EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_2        (GPIO_PIN1)
#define EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8804          (GPIO_PIN7)
#define EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_LED             (GPIO_PIN4)

/* INPUT */
#define EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_SW              (GPIO_PIN9)
#define EN8853C_24P_EN8804_4P_4SFP_PIN_PCA9535_INT           (GPIO_PIN11)
/******* EN8853C_24P_EN8804_4P_4SFP end *******/

/******* EN8853C_24P_1SFP start *******/
/* OUTPUT */
#define EN8853C_24P_1SFP_PIN_SYS_LED                         (GPIO_PIN0)
#define EN8853C_24P_1SFP_PIN_RESET_EN8808_1                  (GPIO_PIN12)
#define EN8853C_24P_1SFP_PIN_RESET_EN8808_2                  (GPIO_PIN1)
#define EN8853C_24P_1SFP_PIN_RESET_LED                       (GPIO_PIN4)
#define EN8853C_24P_1SFP_PIN_SFP_LED_1                       (GPIO_PIN5)
#define EN8853C_24P_1SFP_PIN_SFP_LED_2                       (GPIO_PIN8)

/* INPUT */
#define EN8853C_24P_1SFP_PIN_RESET_SW                        (GPIO_PIN9)
#define EN8853C_24P_1SFP_PIN_VLAN                            (GPIO_PIN13)
#define EN8853C_24P_1SFP_PIN_SW_OPT                          (GPIO_PIN15)
#define EN8853C_24P_1SFP_PIN_SFP_RX_LOSS                     (GPIO_PIN17)
/******* EN8853C_24P_1SFP end *******/

/******* EN8853C_8P_4SFP start *******/
/* OUTPUT */
#define EN8853C_8P_4SFP_PIN_SYS_LED                         (GPIO_PIN0 )
#define EN8853C_8P_4SFP_PIN_LAN_SFP_4_LED                   (GPIO_PIN1 )
#define EN8853C_8P_4SFP_PIN_RESET_LED                       (GPIO_PIN4 )
#define EN8853C_8P_4SFP_PIN_LAN_SFP_5_LED                   (GPIO_PIN5 )
#define EN8853C_8P_4SFP_PIN_LAN_SFP_6_LED                   (GPIO_PIN8 )
#define EN8853C_8P_4SFP_PIN_LAN_SFP_7_LED                   (GPIO_PIN10)

/* INPUT */
#define EN8853C_8P_4SFP_PIN_RESET_SW                        (GPIO_PIN9 )
#define EN8853C_8P_4SFP_PIN_SFP_RX_LOSS_P4                  (GPIO_PIN13)
#define EN8853C_8P_4SFP_PIN_SFP_RX_LOSS_P5                  (GPIO_PIN15)
#define EN8853C_8P_4SFP_PIN_SFP_RX_LOSS_P6                  (GPIO_PIN16)
#define EN8853C_8P_4SFP_PIN_SFP_RX_LOSS_P7                  (GPIO_PIN17)
/******* EN8853C_8P_4SFP end *******/

/******* EN8853C_8P_8SFP start *******/
/* OUTPUT */
#define EN8853C_8P_8SFP_PIN_SYS_LED                         (GPIO_PIN0 )
#define EN8853C_8P_8SFP_PIN_LAN_SFP_4_LED                   (GPIO_PIN1 )
#define EN8853C_8P_8SFP_PIN_RESET_LED                       (GPIO_PIN4 )
#define EN8853C_8P_8SFP_PIN_LAN_SFP_5_LED                   (GPIO_PIN5 )
#define EN8853C_8P_8SFP_PIN_RESET_EN8804                    (GPIO_PIN7 )
#define EN8853C_8P_8SFP_PIN_LAN_SFP_6_LED                   (GPIO_PIN8 )
#define EN8853C_8P_8SFP_PIN_LAN_SFP_7_LED                   (GPIO_PIN10)

/* INPUT */
#define EN8853C_8P_8SFP_PIN_RESET_SW                        (GPIO_PIN9 )
#define EN8853C_8P_8SFP_PIN_SFP_RX_LOSS_P4                  (GPIO_PIN13)
#define EN8853C_8P_8SFP_PIN_SFP_RX_LOSS_P5                  (GPIO_PIN15)
#define EN8853C_8P_8SFP_PIN_SFP_RX_LOSS_P6                  (GPIO_PIN16)
#define EN8853C_8P_8SFP_PIN_SFP_RX_LOSS_P7                  (GPIO_PIN17)
/******* EN8853C_8P_8SFP end *******/

/******* EN8853C_16P_2SFP start *******/
/* OUTPUT */
#define EN8853C_16P_2SFP_PIN_SYS_LED                         (GPIO_PIN0)
#define EN8853C_16P_2SFP_PIN_RESET_EN8808_1                  (GPIO_PIN1)
#define EN8853C_16P_2SFP_PIN_RESET_LED                       (GPIO_PIN4)
#define EN8853C_16P_2SFP_PIN_SFP1_LED_1                      (GPIO_PIN8)
//#define EN8853C_16P_2SFP_PIN_SFP1_LED_1                      (GPIO_PIN1 )
//#define EN8853C_16P_2SFP_PIN_SFP1_LED_2                      (GPIO_PIN5 )
#define EN8853C_16P_2SFP_PIN_SFP2_LED_1                      (GPIO_PIN6 )
//#define EN8853C_16P_2SFP_PIN_SFP2_LED_2                      (GPIO_PIN8 )

/* INPUT */
#define EN8853C_16P_2SFP_PIN_RESET_SW                        (GPIO_PIN9 )
//#define EN8853C_16P_2SFP_PIN_SFP1_RX_LOSS                    (GPIO_PIN10)
#define EN8853C_16P_2SFP_PIN_SFP1_TX_DIS                     (GPIO_PIN5)
//#define EN8853C_16P_2SFP_PIN_SFP1_ABS                        (GPIO_PIN11)
//#define EN8853C_16P_2SFP_PIN_SFP2_RX_LOSS                    (GPIO_PIN17)
#define EN8853C_16P_2SFP_PIN_SFP2_TX_DIS                     (GPIO_PIN12)
//#define EN8853C_16P_2SFP_PIN_SFP2_ABS                        (GPIO_PIN15)
#define EN8853C_16P_2SFP_PIN_EXTEND                          (GPIO_PIN10)
#define EN8853C_16P_2SFP_PIN_VLAN                            (GPIO_PIN11)
#define EN8853C_16P_2SFP_PIN_PHY_INT                         (GPIO_PIN13)
/******* EN8853C_16P_2SFP end *******/

/* btn reset isr type */
#define GPIO_DISABLE_EDG_DETECT    (0x0)
#define GPIO_RISING_EDG_DETECT     (0x1)
#define GPIO_FALLING_EDG_DETECT    (0x2)
#define GPIO_BOTH_EDG_DETECT       (0x3)

/*DATA TYPE DECLARATIONS
*/
typedef int (* PTR_CUSTOMER_SYS_INIT)(void);
typedef int (* PTR_CUSTOMER_POST_INIT)(void);

typedef struct MODULE_SYS_INIT_MAP_ITEM_S
{
    UI8_T                           product_id;
    PTR_CUSTOMER_SYS_INIT           ptr_init_func;
    PTR_CUSTOMER_POST_INIT          ptr_post_init_func;
}MODULE_SYS_INIT_MAP_ITEM_T;

/* MACRO FUNCTION DECLARATIONS
*/
#define SET_GPIO_OUTPUT(x, rc)    do                              \
    {                                                             \
        rc |= air_gpio_setValue(x, GPIO_PIN_HIGH);                \
        rc |= air_gpio_setOutputEnable(x, ENABLE_PIN_OE);         \
    }while(0)

#define SET_SFP_GPIO(c, d, a, rc)    do                           \
    {                                                             \
        rc |= air_gpio_setDirection(c, AIR_GPIO_DIRECTION_INPUT); \
        rc |= air_gpio_setDirection(d, AIR_GPIO_DIRECTION_INPUT); \
        rc |= air_gpio_setDirection(a, AIR_GPIO_DIRECTION_INPUT); \
    }while(0)

/*GLOBAL VARIABLE DECLARATIONS
*/
/*LOCAL SUBPROGRAM DECLARATIONS
*/
static int
_customer_system_init_en8851c_rfb(
    void);

static int
_customer_system_post_init_en8851c_rfb(
    void);

static int
_customer_system_init_en8853c_rfb(
    void);

static int
_customer_system_post_init_en8853c_rfb(
    void);

static int
_customer_system_init_en8851c_8p_2sfp(
    void);

static int
_customer_system_post_init_en8851c_8p_2sfp(
    void);

static int
_customer_system_init_en8853c_24p_en8804_4p_4sfp(
    void);

static int
_customer_system_post_init_en8853c_24p_en8804_4p_4sfp(
    void);

static int _customer_system_init_en8853c_24p_1sfp(
    void);

static int
_customer_system_post_init_en8853c_24p_1sfp(
    void);

static int
_customer_system_init_en8853c_8p_en8804_4sfp(
    void);

static int
_customer_system_post_init_en8853c_8p_en8804_4sfp(
    void);

static int
_customer_system_init_en8853c_8p_en8804_8sfp(
    void);

static int
_customer_system_post_init_en8853c_8p_en8804_8sfp(
    void);

static int
_customer_system_init_en8853c_16p_2sfp(
    void);

static int
_customer_system_post_init_en8853c_16p_2sfp(
    void);

/*STATIC VARIABLE DECLARATIONS
*/
const MODULE_SYS_INIT_MAP_ITEM_T module_sysinit_map[] =
{
    {MW_PRODUCT_ID_EN8851C_8P, _customer_system_init_en8851c_rfb, _customer_system_post_init_en8851c_rfb},
    {MW_PRODUCT_ID_EN8851C_RFB, _customer_system_init_en8851c_rfb, _customer_system_post_init_en8851c_rfb},
    {MW_PRODUCT_ID_EN8853C_RFB, _customer_system_init_en8853c_rfb, _customer_system_post_init_en8853c_rfb},

    {MW_PRODUCT_ID_EN8851C_8P_2SFP, _customer_system_init_en8851c_8p_2sfp, _customer_system_post_init_en8851c_8p_2sfp},
    {MW_PRODUCT_ID_EN8851C_8P_2SFP_L, _customer_system_init_en8851c_8p_2sfp, _customer_system_post_init_en8851c_8p_2sfp},
    {MW_PRODUCT_ID_EN8853C_24P_EN8804_4P_4SFP, _customer_system_init_en8853c_24p_en8804_4p_4sfp, _customer_system_post_init_en8853c_24p_en8804_4p_4sfp},
    {MW_PRODUCT_ID_EN8853C_24P_1SFP, _customer_system_init_en8853c_24p_1sfp, _customer_system_post_init_en8853c_24p_1sfp},
    {MW_PRODUCT_ID_EN8853C_8P_EN8804_4SFP, _customer_system_init_en8853c_8p_en8804_4sfp, _customer_system_post_init_en8853c_8p_en8804_4sfp},
    {MW_PRODUCT_ID_EN8853C_8P_EN8804_8SFP, _customer_system_init_en8853c_8p_en8804_8sfp, _customer_system_post_init_en8853c_8p_en8804_8sfp},
    {MW_PRODUCT_ID_EN8853C_16P_2SFP, _customer_system_init_en8853c_16p_2sfp, _customer_system_post_init_en8853c_16p_2sfp},
};
const int MODULE_SYS_INIT_MAP_ITEM_CNT = sizeof(module_sysinit_map)/sizeof(MODULE_SYS_INIT_MAP_ITEM_T);

/*LOCAL SUBPROGRAM BODIES
*/
/*FUNCTION NAME: customer_system_get_productId
* PURPOSE:
*       the API offered to other module to get the module ID saving in memory
* INPUT:
*       none
* OUTPUT:
*       ptr_length -- The length of module ID.
* RETURN:
*       NULL -- It stard for the module ID having not been gotten, or getting failed
*       NOT A NULL -- return the pointer which point to string of product ID
*/
const C8_T *
customer_system_get_productId(
    UI8_T *ptr_length)
{
    UI32_T  tlv_type_addr = 0;
    UI8_T   tlv_value_len = 0;

    if(MW_E_OK == mw_is_tlv_data_exist(MW_TLV_TYPE_PRODUCT_ID, &tlv_type_addr))
    {
        mw_read_tlv_data(sizeof(UI8_T), (tlv_type_addr + TLV_DATA_TYPE_SIZE), (void *)&tlv_value_len);
    }
    if((0 != tlv_type_addr) && (0 != tlv_value_len) && (TLV_DATA_PRODUCT_ID_VALUE_SIZE >= tlv_value_len))
    {
        /* return product id string addr */
        *ptr_length = tlv_value_len;
        return (C8_T *)(tlv_type_addr + TLV_DATA_HEADER_SIZE);
    }
    return NULL;
}

static int
_customer_system_init_en8851c_rfb(
    void)
{
    int rc = E_OK;

    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO6_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO8_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO12_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO13_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO16_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO9_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO11_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO15_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO10_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO17_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    /* Output pin */
    /* GPIO0 :  SYSTEM LED */
    /* GPIO6 :  SGMII-0 LED */
    /* GPIO8 :  SGMII-1 LED */
    /* GPIO12:  RESET LED */
    /* GPIO13:  TX-DIS-0 */
    /* GPIO16:  TX-DIS-1 */
    rc |= air_gpio_setDirection(GPIO_PIN0, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN6, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN8, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN12, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN13, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN16, AIR_GPIO_DIRECTION_OUTPUT);
    SET_GPIO_OUTPUT(GPIO_PIN0, rc);
    SET_GPIO_OUTPUT(GPIO_PIN6, rc);
    SET_GPIO_OUTPUT(GPIO_PIN8, rc);
    SET_GPIO_OUTPUT(GPIO_PIN12, rc);
    SET_GPIO_OUTPUT(GPIO_PIN13, rc);
    SET_GPIO_OUTPUT(GPIO_PIN16, rc);
    /* Input pin */
    /* GPIO9 :  SW-Reset */
    /* GPIO11:  MOD-ABS-0 */
    /* GPIO15:  MOD-ABS-1 */
    /* GPIO10:  RX-LOS-0 */
    /* GPIO17:  RX-LOS-1 */
    rc |= air_gpio_setDirection(GPIO_PIN9, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(GPIO_PIN11, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(GPIO_PIN15, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(GPIO_PIN10, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(GPIO_PIN17, AIR_GPIO_DIRECTION_INPUT);
    /**/
    rc |= air_gpio_setValue(GPIO_PIN0, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(GPIO_PIN12, GPIO_PIN_LOW);
    return rc;
}

static int
_customer_system_post_init_en8851c_rfb(
    void)
{
    int rc = E_OK;

    rc |= air_gpio_setValue(GPIO_PIN12, GPIO_PIN_HIGH);
    return rc;
}

static int
_customer_system_init_en8853c_rfb(
    void)
{
    int rc = E_OK;

    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO0_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO12_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO1_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO7_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO4_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO9_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO11_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    /* EN8808/8804 reset: GPIO12, 1, 7 */
    /* LED clear control: GPIO4 high to release clear */
    rc |= air_gpio_setDirection(EN8853C_RFB_PIN_SYS_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_RFB_PIN_RESET_EN8808_1, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_RFB_PIN_RESET_EN8808_2, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_RFB_PIN_RESET_EN8804, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_RFB_PIN_RESET_LED, AIR_GPIO_DIRECTION_OUTPUT);
    /**/
    SET_GPIO_OUTPUT(EN8853C_RFB_PIN_SYS_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_RFB_PIN_RESET_EN8808_1, rc);
    SET_GPIO_OUTPUT(EN8853C_RFB_PIN_RESET_EN8808_2, rc);
    SET_GPIO_OUTPUT(EN8853C_RFB_PIN_RESET_EN8804, rc);
    SET_GPIO_OUTPUT(EN8853C_RFB_PIN_RESET_LED, rc);
    /* Input pin */
    /* GPIO8 :  SW-Reset */
    rc |= air_gpio_setDirection(EN8853C_RFB_PIN_RESET_SW, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_RFB_PIN_PCA9535_INT, AIR_GPIO_DIRECTION_INPUT);
    /**/
    rc |= air_gpio_setValue(EN8853C_RFB_PIN_SYS_LED, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_RFB_PIN_RESET_EN8808_1, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_RFB_PIN_RESET_EN8808_2, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_RFB_PIN_RESET_EN8804, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_RFB_PIN_RESET_LED, GPIO_PIN_LOW);
    delay1ms(5);
    rc |= air_gpio_setValue(EN8853C_RFB_PIN_RESET_EN8808_1, GPIO_PIN_HIGH);
    rc |= air_gpio_setValue(EN8853C_RFB_PIN_RESET_EN8808_2, GPIO_PIN_HIGH);
    rc |= air_gpio_setValue(EN8853C_RFB_PIN_RESET_EN8804, GPIO_PIN_HIGH);
    return rc;
}

static int
_customer_system_post_init_en8853c_rfb(
    void)
{
    int rc = E_OK;

    rc |= air_gpio_setValue(EN8853C_RFB_PIN_RESET_LED, GPIO_PIN_HIGH);
    return rc;
}

static int
_customer_system_init_en8851c_8p_2sfp(
    void)
{
    int rc = E_OK;

    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO4_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO5_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO6_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO8_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO12_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO13_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO16_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO9_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO11_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO15_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO10_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO17_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    /* Output pin */
    /* GPIO0 :  SYSTEM LED */
    /* GPIO6 :  SGMII-0 LED */
    /* GPIO8 :  SGMII-1 LED */
    /* GPIO12:  RESET LED */
    /* GPIO13:  TX-DIS-0 */
    /* GPIO16:  TX-DIS-1 */
    rc |= air_gpio_setDirection(GPIO_PIN0, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN4, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN5, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN6, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN8, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN12, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN13, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(GPIO_PIN16, AIR_GPIO_DIRECTION_OUTPUT);
    SET_GPIO_OUTPUT(GPIO_PIN0, rc);
    SET_GPIO_OUTPUT(GPIO_PIN4, rc);
    SET_GPIO_OUTPUT(GPIO_PIN5, rc);
    SET_GPIO_OUTPUT(GPIO_PIN6, rc);
    SET_GPIO_OUTPUT(GPIO_PIN8, rc);
    SET_GPIO_OUTPUT(GPIO_PIN12, rc);
    SET_GPIO_OUTPUT(GPIO_PIN13, rc);
    SET_GPIO_OUTPUT(GPIO_PIN16, rc);
    /* Input pin */
    /* GPIO9 :  SW-Reset */
    /* GPIO11:  MOD-ABS-0 */
    /* GPIO15:  MOD-ABS-1 */
    /* GPIO10:  RX-LOS-0 */
    /* GPIO17:  RX-LOS-1 */
    rc |= air_gpio_setDirection(GPIO_PIN9, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(GPIO_PIN11, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(GPIO_PIN15, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(GPIO_PIN10, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(GPIO_PIN17, AIR_GPIO_DIRECTION_INPUT);
    /**/
    rc |= air_gpio_setValue(GPIO_PIN0, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(GPIO_PIN12, GPIO_PIN_LOW);
    return rc;
}

static int
_customer_system_post_init_en8851c_8p_2sfp(
    void)
{
    int rc = E_OK;

    rc |= air_gpio_setValue(GPIO_PIN12, GPIO_PIN_HIGH);
    return rc;
}

static int
_customer_system_init_en8853c_24p_en8804_4p_4sfp(
    void)
{
    int rc = E_OK;

    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO0_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO12_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO1_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO7_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO4_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO9_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO11_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    /* EN8808/8804 reset: GPIO12, 1, 7 */
    /* LED clear control: GPIO4 high to release clear */
    rc |= air_gpio_setDirection(EN8853C_24P_EN8804_4P_4SFP_PIN_SYS_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_1, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_2, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8804, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_LED, AIR_GPIO_DIRECTION_OUTPUT);
    /**/
    SET_GPIO_OUTPUT(EN8853C_24P_EN8804_4P_4SFP_PIN_SYS_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_1, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_2, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8804, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_LED, rc);
    /* Input pin */
    /* GPIO8 :  SW-Reset */
    rc |= air_gpio_setDirection(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_SW, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_EN8804_4P_4SFP_PIN_PCA9535_INT, AIR_GPIO_DIRECTION_INPUT);
    /**/
    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_SYS_LED, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_1, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_2, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8804, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_LED, GPIO_PIN_LOW);
    delay1ms(5);
    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_1, GPIO_PIN_HIGH);
    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8808_2, GPIO_PIN_HIGH);
    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_EN8804, GPIO_PIN_HIGH);
    return rc;
}

static int
_customer_system_post_init_en8853c_24p_en8804_4p_4sfp(
    void)
{
    int rc = E_OK;

    rc |= air_gpio_setValue(EN8853C_24P_EN8804_4P_4SFP_PIN_RESET_LED, GPIO_PIN_HIGH);
    return rc;
}

static int _customer_system_init_en8853c_24p_1sfp(
    void)
{
    int rc = E_OK;

    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO0_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO12_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO1_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO4_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO5_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO8_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO9_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO13_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO15_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO17_MODE, AIR_CHIPSCU_IOMUX_ENABLE);

    /* EN8808 reset: GPIO12, 1 */
    /* LED clear control: GPIO4 high to release clear */
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_SYS_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_RESET_EN8808_1, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_RESET_EN8808_2, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_RESET_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_SFP_LED_1, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_SFP_LED_2, AIR_GPIO_DIRECTION_OUTPUT);

    SET_GPIO_OUTPUT(EN8853C_24P_1SFP_PIN_SYS_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_1SFP_PIN_RESET_EN8808_1, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_1SFP_PIN_RESET_EN8808_2, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_1SFP_PIN_RESET_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_1SFP_PIN_SFP_LED_1, rc);
    SET_GPIO_OUTPUT(EN8853C_24P_1SFP_PIN_SFP_LED_2, rc);

    /* Input pin */
    /* GPIO9 :  SW-Reset */
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_RESET_SW, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_SW_OPT, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_VLAN, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_24P_1SFP_PIN_SFP_RX_LOSS, AIR_GPIO_DIRECTION_INPUT);


    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_SYS_LED, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_RESET_EN8808_1, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_RESET_EN8808_2, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_RESET_LED, GPIO_PIN_LOW);

    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_SFP_LED_1, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_SFP_LED_2, GPIO_PIN_LOW);
    delay1ms(5);
    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_RESET_EN8808_1, GPIO_PIN_HIGH);
    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_RESET_EN8808_2, GPIO_PIN_HIGH);

    return rc;
}

static int
_customer_system_post_init_en8853c_24p_1sfp(
    void)
{
    int rc = E_OK;

    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_RESET_LED, GPIO_PIN_HIGH);

    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_SFP_LED_1, GPIO_PIN_HIGH);
    rc |= air_gpio_setValue(EN8853C_24P_1SFP_PIN_SFP_LED_2, GPIO_PIN_HIGH);
    return rc;
}

static int
_customer_system_init_en8853c_8p_en8804_4sfp(
    void)
{
    int rc = E_OK;

    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO0_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO1_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO4_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO5_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO8_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO9_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO10_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO13_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO15_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO16_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO17_MODE, AIR_CHIPSCU_IOMUX_ENABLE);

    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_SYS_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_LAN_SFP_4_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_RESET_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_LAN_SFP_5_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_LAN_SFP_6_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_LAN_SFP_7_LED, AIR_GPIO_DIRECTION_OUTPUT);

    SET_GPIO_OUTPUT(EN8853C_8P_4SFP_PIN_SYS_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_4SFP_PIN_LAN_SFP_4_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_4SFP_PIN_RESET_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_4SFP_PIN_LAN_SFP_5_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_4SFP_PIN_LAN_SFP_6_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_4SFP_PIN_LAN_SFP_7_LED, rc);

    /* Input pin */
    /* GPIO9 :  SW-Reset */
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_RESET_SW, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_SFP_RX_LOSS_P4, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_SFP_RX_LOSS_P5, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_SFP_RX_LOSS_P6, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_4SFP_PIN_SFP_RX_LOSS_P7, AIR_GPIO_DIRECTION_INPUT);

    rc |= air_gpio_setValue(EN8853C_8P_4SFP_PIN_RESET_LED, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_8P_4SFP_PIN_SYS_LED, GPIO_PIN_LOW);
    delay1ms(5);

    return rc;
}

static int
_customer_system_post_init_en8853c_8p_en8804_4sfp(
    void)
{
    int rc = E_OK;

    rc |= air_gpio_setValue(EN8853C_8P_4SFP_PIN_RESET_LED, GPIO_PIN_HIGH);
    return rc;
}

static int
_customer_system_init_en8853c_8p_en8804_8sfp(
    void)
{
    int rc = E_OK;

    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO0_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO1_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO4_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO5_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO7_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO8_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO9_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO10_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO13_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO15_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO16_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO17_MODE, AIR_CHIPSCU_IOMUX_ENABLE);

    /* EN8804 reset: GPIO7 */
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_SYS_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_LAN_SFP_4_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_RESET_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_LAN_SFP_5_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_LAN_SFP_6_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_LAN_SFP_7_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_RESET_EN8804, AIR_GPIO_DIRECTION_OUTPUT);

    SET_GPIO_OUTPUT(EN8853C_8P_8SFP_PIN_SYS_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_8SFP_PIN_LAN_SFP_4_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_8SFP_PIN_RESET_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_8SFP_PIN_LAN_SFP_5_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_8SFP_PIN_LAN_SFP_6_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_8SFP_PIN_LAN_SFP_7_LED, rc);
    SET_GPIO_OUTPUT(EN8853C_8P_8SFP_PIN_RESET_EN8804, rc);

    /* Input pin */
    /* GPIO9 :  SW-Reset */
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_RESET_SW, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_SFP_RX_LOSS_P4, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_SFP_RX_LOSS_P5, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_SFP_RX_LOSS_P6, AIR_GPIO_DIRECTION_INPUT);
    rc |= air_gpio_setDirection(EN8853C_8P_8SFP_PIN_SFP_RX_LOSS_P7, AIR_GPIO_DIRECTION_INPUT);

    rc |= air_gpio_setValue(EN8853C_8P_8SFP_PIN_RESET_LED, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_8P_8SFP_PIN_RESET_EN8804, GPIO_PIN_LOW);
    delay1ms(5);
    rc |= air_gpio_setValue(EN8853C_8P_8SFP_PIN_RESET_EN8804, GPIO_PIN_HIGH);
    rc |= air_gpio_setValue(EN8853C_8P_8SFP_PIN_SYS_LED, GPIO_PIN_LOW);

    return rc;
}

static int
_customer_system_post_init_en8853c_8p_en8804_8sfp(
    void)
{
    int rc = E_OK;

    rc |= air_gpio_setValue(EN8853C_8P_8SFP_PIN_RESET_LED, GPIO_PIN_HIGH);
    return rc;
}

static int _customer_system_init_en8853c_16p_2sfp(
    void)
{
    int rc = E_OK;

    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO0_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO12_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO4_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO1_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO5_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO6_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO8_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO9_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO10_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO11_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO13_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO15_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO16_MODE, AIR_CHIPSCU_IOMUX_ENABLE);
    rc |= air_chipscu_setIomuxFuncState(0, AIR_CHIPSCU_IOMUX_FORCE_GPIO17_MODE, AIR_CHIPSCU_IOMUX_ENABLE);

    /* EN8808 reset: GPIO12, 1 */
    /* LED clear control: GPIO4 high to release clear */
    rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SYS_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_RESET_EN8808_1, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_RESET_LED, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP1_LED_1, AIR_GPIO_DIRECTION_OUTPUT);
    //rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP1_LED_2, AIR_GPIO_DIRECTION_OUTPUT);
    rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP2_LED_1, AIR_GPIO_DIRECTION_OUTPUT);
	//rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP2_LED_2, AIR_GPIO_DIRECTION_OUTPUT);
	rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP1_TX_DIS, AIR_GPIO_DIRECTION_OUTPUT);
	rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP2_TX_DIS, AIR_GPIO_DIRECTION_OUTPUT);

    SET_GPIO_OUTPUT(EN8853C_16P_2SFP_PIN_SYS_LED, rc);
    air_gpio_setValue(EN8853C_16P_2SFP_PIN_SYS_LED, GPIO_PIN_LOW);
    SET_GPIO_OUTPUT(EN8853C_16P_2SFP_PIN_RESET_EN8808_1, rc);
    SET_GPIO_OUTPUT(EN8853C_16P_2SFP_PIN_RESET_LED, rc);
    //SET_GPIO_OUTPUT(EN8853C_16P_2SFP_PIN_SFP1_LED_1, rc);
    //SET_GPIO_OUTPUT(EN8853C_16P_2SFP_PIN_SFP1_LED_2, rc);
    //air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP2_LED_1, GPIO_PIN_LOW);
    //SET_GPIO_OUTPUT(EN8853C_16P_2SFP_PIN_SFP2_LED_2, rc);
    SET_GPIO_OUTPUT(EN8853C_16P_2SFP_PIN_SFP1_TX_DIS, rc);
	SET_GPIO_OUTPUT(EN8853C_16P_2SFP_PIN_SFP2_TX_DIS, rc);

    /* Input pin */
    /* GPIO9 :  SW-Reset */
    rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_RESET_SW, AIR_GPIO_DIRECTION_INPUT);
    //rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP1_RX_LOSS, AIR_GPIO_DIRECTION_INPUT);
    //rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP1_TX_DIS, AIR_GPIO_DIRECTION_INPUT);
    //rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP1_ABS, AIR_GPIO_DIRECTION_INPUT);
    //rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP2_RX_LOSS, AIR_GPIO_DIRECTION_INPUT);
    //rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP2_TX_DIS, AIR_GPIO_DIRECTION_INPUT);
    //rc |= air_gpio_setDirection(EN8853C_16P_2SFP_PIN_SFP2_ABS, AIR_GPIO_DIRECTION_INPUT);

    rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_RESET_EN8808_1, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_RESET_LED, GPIO_PIN_LOW);
    //rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP1_LED_1, GPIO_PIN_LOW);
    ////rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP1_LED_2, GPIO_PIN_LOW);
    //rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP2_LED_1, GPIO_PIN_LOW);
    ////rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP2_LED_2, GPIO_PIN_LOW);
    delay1ms(5);
    rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_RESET_EN8808_1, GPIO_PIN_HIGH);

    return rc;
}

static int
_customer_system_post_init_en8853c_16p_2sfp(
    void)
{
    int rc = E_OK;

    #if 0
	UI8_T test_port[] = {2,1,4,3,5,6,7,8,10,9,12,11,13,14,15,16};
	
    rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_RESET_LED, GPIO_PIN_HIGH);

	UI8_T   port = 0;
    UI32_T  total_port_num = sizeof(test_port), unit = 0;

	/*disable all ports*/
	for(port = 0; port < total_port_num; port++)
	{
		air_port_setAdminState(unit, test_port[port], FALSE);
	}

	/*led light by phy*/

	for(port = 0; port < total_port_num; port++)
	{
		/* Configure all the LED state of LED s0 on each port to OFF, and verify the state of LED 0 */
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_OFF);

		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_OFF);

		/* Recovery each LED to be controlled by PHY */
		air_port_setPhyLedCtrlMode(unit, test_port[port], 0, AIR_PORT_PHY_LED_CTRL_MODE_PHY);
		air_port_setPhyLedCtrlMode(unit, test_port[port], 1, AIR_PORT_PHY_LED_CTRL_MODE_PHY);

	}

	/*led test one by one*/
	for(port = 0; port < total_port_num; port++)
	{
		/* Config the LED control mode to force mode */
	    air_port_setPhyLedCtrlMode(unit, test_port[port], 0, AIR_PORT_PHY_LED_CTRL_MODE_FORCE);
	    air_port_setPhyLedCtrlMode(unit, test_port[port], 1, AIR_PORT_PHY_LED_CTRL_MODE_FORCE);
		/* Configure all the LED state of LED 0 on each port to OFF, and verify the state of LED 0 */
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_OFF);
		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_OFF);
		/* Configure all the LED state of LED 0 on each port to OFF, and verify the state of LED 0 */
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_ON);
		delay1ms(1000);
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_OFF);
		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_ON);
		delay1ms(1000);
		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_OFF);
		/* Recovery each LED to be controlled by PHY */
		air_port_setPhyLedCtrlMode(unit, test_port[port], 0, AIR_PORT_PHY_LED_CTRL_MODE_PHY);
		air_port_setPhyLedCtrlMode(unit, test_port[port], 1, AIR_PORT_PHY_LED_CTRL_MODE_PHY);

	}

	/*port 17,18*/
	air_perif_setGpioOutputAutoMode(0, EN8853C_16P_2SFP_PIN_SFP1_LED_1, FALSE);
    air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP1_LED_1, GPIO_PIN_HIGH);
    
    delay1ms(1000);
    air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP1_LED_1, GPIO_PIN_LOW);
    air_perif_setGpioOutputAutoMode(0, EN8853C_16P_2SFP_PIN_SFP1_LED_1, TRUE);

    air_perif_setGpioOutputAutoMode(0, EN8853C_16P_2SFP_PIN_SFP2_LED_1, FALSE);
    air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP2_LED_1, GPIO_PIN_HIGH);
    delay1ms(1000);
    air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP2_LED_1, GPIO_PIN_LOW);
	air_perif_setGpioOutputAutoMode(0, EN8853C_16P_2SFP_PIN_SFP2_LED_1, TRUE);
	
	delay1ms(1000);
	rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_RESET_LED, GPIO_PIN_LOW);
    rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_RESET_LED, GPIO_PIN_HIGH);

   /*enable all ports*/
	for(port = 0; port < total_port_num; port++)
	{
		air_port_setAdminState(unit, test_port[port], TRUE);
	}
	#else
	UI8_T test_port[] = {2,1,4,3,5,6,7,8,10,9,12,11,13,14,15,16};
    rc |= air_gpio_setValue(EN8853C_16P_2SFP_PIN_RESET_LED, GPIO_PIN_HIGH);

	UI8_T   port = 0;
    UI32_T  total_port_num = sizeof(test_port), unit = 0;

	/*disable all ports*/
	for(port = 0; port < total_port_num; port++)
	{
		air_port_setAdminState(unit, test_port[port], FALSE);
	}


	/*led light by phy*/
	for(port = 0; port < total_port_num; port++)
	{
		/* Configure all the LED state of LED s0 on each port to OFF, and verify the state of LED 0 */
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_OFF);

		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_OFF);

		/* Recovery each LED to be controlled by PHY */
		air_port_setPhyLedCtrlMode(unit, test_port[port], 0, AIR_PORT_PHY_LED_CTRL_MODE_PHY);
		air_port_setPhyLedCtrlMode(unit, test_port[port], 1, AIR_PORT_PHY_LED_CTRL_MODE_PHY);
	}

	/*led test	green on */
	for(port = 0; port < total_port_num; port++)
	{
		/* Config the LED control mode to force mode */
	    air_port_setPhyLedCtrlMode(unit, test_port[port], 0, AIR_PORT_PHY_LED_CTRL_MODE_FORCE);
	    air_port_setPhyLedCtrlMode(unit, test_port[port], 1, AIR_PORT_PHY_LED_CTRL_MODE_FORCE);
		/* Configure all the LED state of LED 0 on each port to OFF, and verify the state of LED 0 */
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_OFF);
		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_OFF);

		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_ON);
	}
	air_perif_setGpioOutputAutoMode(0, EN8853C_16P_2SFP_PIN_SFP1_LED_1, FALSE);
	air_perif_setGpioOutputAutoMode(0, EN8853C_16P_2SFP_PIN_SFP2_LED_1, FALSE);
    //air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP1_LED_1, GPIO_PIN_HIGH);
    //air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP2_LED_1, GPIO_PIN_LOW);
    air_perif_setGpioOutputData(0, EN8853C_16P_2SFP_PIN_SFP1_LED_1, AIR_PERIF_GPIO_DATA_HIGH);
    air_perif_setGpioOutputData(0, EN8853C_16P_2SFP_PIN_SFP2_LED_1, AIR_PERIF_GPIO_DATA_LOW);
	delay1ms(2000);
	/*led test yellon on */
	for(port = 0; port < total_port_num; port++)
	{
		/* Config the LED control mode to force mode */
	    air_port_setPhyLedCtrlMode(unit, test_port[port], 0, AIR_PORT_PHY_LED_CTRL_MODE_FORCE);
	    air_port_setPhyLedCtrlMode(unit, test_port[port], 1, AIR_PORT_PHY_LED_CTRL_MODE_FORCE);
		/* Configure all the LED state of LED 0 on each port to OFF, and verify the state of LED 0 */
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_OFF);
		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_OFF);

		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_ON);

	}
	air_perif_setGpioOutputData(0, EN8853C_16P_2SFP_PIN_SFP1_LED_1, AIR_PERIF_GPIO_DATA_LOW);
    air_perif_setGpioOutputData(0, EN8853C_16P_2SFP_PIN_SFP2_LED_1, AIR_PERIF_GPIO_DATA_HIGH);
	//air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP1_LED_1, GPIO_PIN_LOW);
	//air_gpio_setValue(EN8853C_16P_2SFP_PIN_SFP2_LED_1, GPIO_PIN_LOW);

	delay1ms(2000);
	
	/*led light by phy*/
	for(port = 0; port < total_port_num; port++)
	{
		/* Configure all the LED state of LED s0 on each port to OFF, and verify the state of LED 0 */
		air_port_setPhyLedForceState(unit, test_port[port], 0, AIR_PORT_PHY_LED_STATE_OFF);

		/* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
		air_port_setPhyLedForceState(unit, test_port[port], 1, AIR_PORT_PHY_LED_STATE_OFF);

		/* Recovery each LED to be controlled by PHY */
		air_port_setPhyLedCtrlMode(unit, test_port[port], 0, AIR_PORT_PHY_LED_CTRL_MODE_PHY);
		air_port_setPhyLedCtrlMode(unit, test_port[port], 1, AIR_PORT_PHY_LED_CTRL_MODE_PHY);

	}
	
	/*enable all ports*/
	for(port = 0; port < total_port_num; port++)
	{
		air_port_setAdminState(unit, test_port[port], TRUE);
	}
	air_perif_setGpioOutputAutoMode(0, EN8853C_16P_2SFP_PIN_SFP1_LED_1, TRUE);
	air_perif_setGpioOutputAutoMode(0, EN8853C_16P_2SFP_PIN_SFP2_LED_1, TRUE);
	#endif
	
    return rc;
}

/*FUNCTION NAME: _customer_system_get_init_item_by_productId
* PURPOSE:
*       to find the port configuring item by inputing product ID
* INPUT:
*       product_id    -- the product ID of the module
* OUTPUT:
*       none
* RETURN:
*       NULL -- having not found the port configuring item
*       NOT A NULL -- return the pointer which point to the configuring item which of module ID matched the module ID inputed
* NOTES:
*      None
*/
const MODULE_SYS_INIT_MAP_ITEM_T *
_customer_system_get_init_item_by_productId(
    UI8_T   product_id)
{
    int i = 0;

    for(i = 0; i < MODULE_SYS_INIT_MAP_ITEM_CNT; i++)
    {
        if(product_id == module_sysinit_map[i].product_id)
        {
            return &(module_sysinit_map[i]);
        }
    }
    return NULL;
}

/*FUNCTION NAME: customer_system_init
* PURPOSE:
*        the API offered to main.cpp call for initializing system by customer's configuration
* INPUT:
*       none
* OUTPUT:
*       none
* RETURN:
*       E_OK --Initializing successfully
*       OTHERS -- Initializing fail
* NOTES:
*      None
*/
int
customer_system_init(
    void)
{
    int rc = E_ENTRY_NOT_FOUND;
    const MODULE_SYS_INIT_MAP_ITEM_T *ptr_item = NULL;
    UI8_T   product_id = mw_get_productID();

    ptr_item = _customer_system_get_init_item_by_productId(product_id);
    if(NULL != ptr_item)
    {
        ptr_item->ptr_init_func();
        rc = E_OK;
    }
    return rc;
}

/*FUNCTION NAME: customer_system_post_init
* PURPOSE:
*        the API offered to main.cpp call for doing some thing of after initializing system
* INPUT:
*       none
* OUTPUT:
*       none
* RETURN:
*       E_OK -- post-initializing successfully
*       OTHERS -- post-initializing fail
* NOTES:
*      None
*/
int
customer_system_post_init(
    void)
{
    int rc = E_ENTRY_NOT_FOUND;
    UI8_T   port = 0;
    UI32_T  total_port_num = 0, unit = 0;
    AIR_ERROR_NO_T air_rc = AIR_E_OK;
    const MODULE_SYS_INIT_MAP_ITEM_T *ptr_item = NULL;
    AIR_INIT_PORT_MAP_T *ptr_portMapList = NULL, *ptr_cur = NULL;
    UI8_T   product_id = mw_get_productID();

    ptr_item = _customer_system_get_init_item_by_productId(product_id);
    if(NULL != ptr_item)
    {
        osapi_calloc((sizeof(AIR_INIT_PORT_MAP_T) * AIR_PORT_NUM), "portMapList", (void**)&ptr_portMapList);
        if(NULL != ptr_portMapList)
        {
            air_rc = air_init_getSdkPortMap(unit, &total_port_num, ptr_portMapList);
            if(AIR_E_OK == air_rc)
            {
                ptr_cur = ptr_portMapList;
                for(port = 0; port < total_port_num; port++)
                {
                    if(AIR_INIT_PORT_TYPE_CPU != ptr_cur->port_type)
                    {
                        /* Config the LED control mode to force mode */
                        air_port_setPhyLedCtrlMode(unit, port, 0, AIR_PORT_PHY_LED_CTRL_MODE_FORCE);
                        air_port_setPhyLedCtrlMode(unit, port, 1, AIR_PORT_PHY_LED_CTRL_MODE_FORCE);

                        /* Configure all the LED state of LED 0 on each port to ON, and verify the state of LED 0 */
                        air_port_setPhyLedForceState(unit, port, 0, AIR_PORT_PHY_LED_STATE_ON);

                        /* Configure all the LED state of LED 1 on each port to ON, and verify the state of LED 1 */
                        air_port_setPhyLedForceState(unit, port, 1, AIR_PORT_PHY_LED_STATE_ON);

                    }
                    ptr_cur++;
                }

                /* Before pulling the reset pin, force all LEDs on, and after pulling the pin, force all LEDs off */
                ptr_item->ptr_post_init_func();

                ptr_cur = ptr_portMapList;
                for(port = 0; port < total_port_num; port++)
                {
                    if(AIR_INIT_PORT_TYPE_CPU != ptr_cur->port_type)
                    {
                        /* Configure all the LED state of LED 0 on each port to OFF, and verify the state of LED 0 */
                        air_port_setPhyLedForceState(unit, port, 0, AIR_PORT_PHY_LED_STATE_OFF);

                        /* Configure all the LED state of LED 1 on each port to OFF, and verify the state of LED 1 */
                        air_port_setPhyLedForceState(unit, port, 1, AIR_PORT_PHY_LED_STATE_OFF);

                        /* Recovery each LED to be controlled by PHY */
                        air_port_setPhyLedCtrlMode(unit, port, 0, AIR_PORT_PHY_LED_CTRL_MODE_PHY);
                        air_port_setPhyLedCtrlMode(unit, port, 1, AIR_PORT_PHY_LED_CTRL_MODE_PHY);

                    }
                    ptr_cur++;
                }
            }
            else
            {
                osapi_printf("air_init_getSdkPortMap get fail.\n");
            }
            osapi_free(ptr_portMapList);
        }
        rc = E_OK;
    }

    air_perif_setGpioOutputAutoMode(0, GPIO_PIN0, FALSE);
    btn_reset_isr_init(GPIO_PIN9,GPIO_BOTH_EDG_DETECT);
    return rc;
}
