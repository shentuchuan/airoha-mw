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

/* FILE NAME:   default_config.c
 * PURPOSE:
 *      1.Definition default value of each group.
 *      2.Definition name of each Json member.
 *
 * NOTES:
 */

 /* INCLUDE FILE DECLARATIONS
 */
#include "default_config.h"

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */
/*****************************************
 *  SYS_INFO                             *
 *****************************************/
const DB_SYS_INFO_T _sys_info_dft =
{
    .sys_name = "HR210S-16T2GS",
    .sw_version = "1.0",
    .dhcp_enable = 0,
    .autodns_enable = 0,
    .static_ip = 0x0102A8C0,
    //.static_ip = 0xC800A8C0,
    .static_mask = 0x00FFFFFF,
    .static_gw = 0x0102A8C0,
    //.static_gw = 0x0100A8C0,
    .static_dns = 0x08080808
};

/*****************************************
 *  ACCOUNT_INFO                         *
 *****************************************/
#ifdef AIR_8851_SUPPORT
/* User Account use SHA-256 */
const DB_ACCOUNT_INFO_T _account_info_dft =
{
    .username = "admin",
    .passwd = {0x8c,0x69,0x76,0xe5,0xb5,0x41,0x04,0x15,0xbd,0xe9,0x08,0xbd,0x4d,0xee,0x15,0xdf,
               0xb1,0x67,0xa9,0xc8,0x73,0xfc,0x4b,0xb8,0xa8,0x1f,0x6f,0x2a,0xb4,0x48,0xa9,0x18} /*admin */
};
#else
/* User Account use plain text */
const DB_ACCOUNT_INFO_T _account_info_dft =
{
    .username = "admin",
    .passwd = "admin"
};
#endif

#ifdef AIR_SUPPORT_DEFAULT_CONFIG
/*****************************************
 *  PORT_CFG_INFO                        *
 *****************************************/
const DB_PORT_CFG_INFO_T _port_cfg_info_dft =
{
    .type = 0,
    .admin_status = 1,
    .admin_speed = 0,
    .admin_duplex = 2,
    .admin_flow_ctrl = 0,
    //.admin_flow_ctrl = 1,
    .admin_spd_ability = 31,
    .isolation = 0x1FFFFFFE,
    .ingress_rate = 0,
    .egress_rate = 0,
    .green_ethernet = 0,
    .eee_enable = 0,
    .pvid = 1,
    .trunk_id = 0,
    .mirror_id = 0,
    .vlan_list = 1,
    .mac_limit = 0,
    .storm_bc_rate = 0,
    .storm_mc_rate = 0,
    .storm_uc_rate = 0,
    .storm_bc_mode = 0,
    .storm_mc_mode = 0,
    .storm_uc_mode = 0,
    .storm_bc_cfg = 0,
    .storm_mc_cfg = 0,
    .storm_uc_cfg = 0,
    .igmp_router_port = 0,
    .vlan_ig_filter = 0
};

/*****************************************
 *  PORT_QOS                             *
 *****************************************/
const DB_PORT_QOS_T _port_qos_dft =
{
    .priority = 0,
    .q0_weight_wrr = 1,
    .q1_weight_wrr = 2,
    .q2_weight_wrr = 3,
    .q3_weight_wrr = 4,
    .q4_weight_wrr = 5,
    .q5_weight_wrr = 6,
    .q6_weight_wrr = 7,
    .q7_weight_wrr = 8,
    .q0_weight_wfq = 1,
    .q1_weight_wfq = 2,
    .q2_weight_wfq = 3,
    .q3_weight_wfq = 4,
    .q4_weight_wfq = 5,
    .q5_weight_wfq = 6,
    .q6_weight_wfq = 7,
    .q7_weight_wfq = 8
};

/*****************************************
 *  TRUNK_PORT                           *
 *****************************************/
const DB_TRUNK_PORT_T _trunk_port_dft =
{
     .members = {0,0,0,0}
};

/*****************************************
 *  LOOP_PREVEN_INFO                     *
 *****************************************/
const DB_LOOP_PREVEN_INFO_T _loop_preven_info_dft =
{
    .enable = 0,
};

/*****************************************
 *  IGMP_SNP_INFO                        *
 *****************************************/
const DB_IGMP_SNP_INFO_T _igmp_snp_info_dft =
{
    .enable = 0,
    .rpt_suppress = 0,
    .fast_leave = 0
};

#ifdef AIR_SUPPORT_IGMP_SNP_QUERIER
/*****************************************
 *  IGMP_SNP_QUERIER_INFO                *
 *****************************************/
const DB_IGMP_SNP_QUERIER_INFO_T _igmp_snp_querier_info_dft =
{
    .vlan_id =   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    .state =     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    .election =  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    .version =   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    .address =   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
#endif /* AIR_SUPPORT_IGMP_SNP_QUERIER */

/*****************************************
 *  PORT_MIRROR_INFO                     *
 *****************************************/
const DB_PORT_MIRROR_INFO_T _port_mirror_info_dft =
{
    .enable = {0,0,0,0},
    .dest_port = {0,0,0,0},
    .src_in_port = {0,0,0,0},
    .src_eg_port = {0,0,0,0}
};

/*****************************************
 *  JUMBO_FRAME_INFO                     *
 *****************************************/
const DB_JUMBO_FRAME_INFO_T _jumbo_frame_info_dft =
{
    .cfg = 0
};

/*****************************************
 *  VLAN_CFG_INFO                        *
 *****************************************/
const DB_VLAN_CFG_INFO_T _vlan_cfg_info_dft =
{
    .enable_port_b = 1,
    .enable_8021q_b = 0,
    .enable_mtu = 0
};

/*****************************************
 *  VLAN_ENTRY                           *
 *****************************************/
const DB_VLAN_ENTRY_T _vlan_entry_dft =
{
    .vlan_id = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,0,0,0,0},
    .descr = {"Default","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","",""},
    .port_member = {0x1FFFFFFE,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    .tagged_member = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    .untagged_member = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

/*****************************************
 *  VOICE_VLAN_INFO                      *
 *****************************************/
const DB_VOICE_VLAN_INFO_T _voice_vlan_info_dft =
{
    .vlan_state = 0,
    .vlan_id = 0,
    .vlan_priority = 0
#ifdef AIR_SUPPORT_VOICE_VLAN
    ,.port_mode = 0xFFFFFFF
#ifdef AIR_SUPPORT_VOVLAN_SECURITY
    ,.security_mode = 0
#endif
#endif
};

/*****************************************
 *  SURVEI_VLAN_INFO                     *
 *****************************************/
const DB_SURVEI_VLAN_INFO_T _survei_vlan_info_dft =
{
    .vlan_state = 0,
    .vlan_id = 0,
    .vlan_priority = 0
};

/*****************************************
 *  OUI_ENTRY                            *
 *****************************************/
const DB_OUI_ENTRY_T _oui_entry_dft =
{
    .mac = {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},
            {0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
    .type = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    .descr = {"","","","","","","","","","","","","","","",""}
};

/*****************************************
 *  QOS_INFO                             *
 *****************************************/
const DB_QOS_INFO_T _qos_info_dft =
{
    .qos_schedule = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    .qos_mode = 0,
    .pri_to_queue = {1,0,2,3,4,5,6,7},
    .dscp_to_pri = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

/*****************************************
 *  STATIC_MAC_ENTRY                     *
 *****************************************/
const DB_STATIC_MAC_ENTRY_T _static_mac_entry_dft =
{
    .mac_addr = {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},
                 {0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},
                 {0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},
                 {0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},
                 {0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},
                 {0,0,0,0,0,0},{0,0,0,0,0,0}},
    .vid = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    .port = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

/*****************************************
 *  DHCP_SNP_INFO                        *
 *****************************************/
#ifdef AIR_SUPPORT_DHCP_SNOOP
const DB_DHCP_SNP_INFO_T _dhcp_snp_info_dft =
{
    .enable = 0,
};

const DB_DHCP_SNP_PORT_INFO_T _dhcp_snp_port_dft =
{
    .trust_port = 0,
    .opt82_enable = 0,
    .opt82_mode = 0,
    .circuit_id_type = 0,
    .circuit_id = {0},
    .remote_id_type = 0,
    .remote_id = {0}
};
#endif
#ifdef AIR_SUPPORT_SNMP
const DB_SNMP_INFO_T _snmp_info_dft =
{
    .version = 0x0,
    .trap_hostname = "\0",
    .trap_enable = 0x0,
    .trap_type = 0x0,
    .trap_dst_ip = 0x0102A8C0,
    //.trap_dst_ip = 0x0100A8C0,
    .read_community = "public",
    .set_community = "private",
    .trap_community = "public"
};
#endif
#ifdef AIR_SUPPORT_LLDPD
const DB_LLDP_INFO_T _lldp_info_dft =
{
    .enable = 0,
    .tx_hold_multipler = 4,
    .tx_interval = 30,
    .reinit_delay = 2,
    .tx_delay = 2
};
#endif
/*****************************************
 *  RSTP                                 *
 *****************************************/
#ifdef AIR_SUPPORT_RSTP
const DB_RSTP_INFO_T _rstp_info_dft =
{
    .enable                  = 0,
    .force_version           = 2,
    .forward_delay           = 15,
    .max_age                 = 20,
    .transmit_hold_count     = 6,
    .priority                = 32768,
};

const DB_RSTP_PORT_INFO_T _rstp_port_info_dft =
{
    .priority    = 128,
    .cost        = 2000,
};
#endif /* AIR_SUPPORT_RSTP */

/*****************************************
 *  MQTTD                                *
 *****************************************/
#ifdef AIR_SUPPORT_MQTTD
const DB_MQTTD_CFG_INFO_T _mqttd_cfg_info_dft =
{
#ifdef AIR_SUPPORT_MQTTD_STARTONBOOT
    .enable = 1
#else
    .enable = 0
#endif
};
#endif /* AIR_SUPPORT_MQTTD */

#ifdef AIR_SUPPORT_POE
const DB_POE_CFG_T _poe_cfg_dft =
{
    .total_available_power  = 0,
};
const DB_POE_PORT_CFG_T _poe_port_cfg_dft =
{
    .port_power_control     = 1,
    .port_available_power   = 30,
    .port_priority          = 0,
    .port_delay_time        = 0,
    .port_af_at_mode        = 1,
};
#endif

#endif
/* GLOBAL VARIABLE DECLARATIONS
 */

/* STATIC VARIABLE DECLARATIONS
 */

/* LOCAL SUBPROMGRAM BODIES
*/

/* EXPORTED SUBPROGRAM BODIES
 */

