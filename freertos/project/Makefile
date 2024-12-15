ifdef FREERTOS_MK
include $(FREERTOS_MK)
else
include $(shell pwd)/config.mk
endif

### for airoha modifications ###################
ifeq ($(filter $(PLAT), EN8851 EN8852),$(PLAT))
AIR_8851_SUPPORT = 1
IS_FPGA_STAGE=0
DUAL_IMAGE = 1
OS_MODULE = freertos
#IRQ_DBG_SUPPORT = 1
endif
################################################

PROG ?= airRTOSSystem
include $(shell pwd)/toolchain.mk


ifeq ($(AIR_8851_SUPPORT),1)
# fixed platform configuration
ADDR = 4GB
USE_CACHEWB = 1
V = 1
USE_HTTPS = 1

# Select platform configuration
# PLAT: EN8851, EN8852
ifneq ($(filter $(PLAT), EN8851 EN8852),$(PLAT))
$(error Unknown PLAT "$(PLAT)" is not supported!)
endif
else
# Select platform configure
# PLAT: AG101P, AE210P, AE100 AE3XX
# ADDR: 4GB, 16MB
PLAT ?= AE210P
ADDR ?= 4GB

ifneq ($(filter $(PLAT), AG101P AE210P AE100 AE3XX),$(PLAT))
$(error Unknown PLAT "$(PLAT)" is not supported!)
endif

ifneq ($(filter $(ADDR), 4GB 16MB),$(ADDR))
$(error Unknown ADDR "$(ADDR)" is not supported!)
endif
endif

PLATFORM = $(shell echo $(PLAT) | tr A-Z a-z)

# Define the directories
FREERTOS_SOURCE_DIR	= $(shell pwd)/../kernel/source
FREERTOS_APP_DIR = $(shell pwd)/../app
BSP_SOURCE_DIR = $(shell pwd)/../bsp
BSP_ARCH_NDS32_DIR = $(BSP_SOURCE_DIR)/arch/NDS32
BSP_DRIVER_SOURCE_DIR = $(BSP_SOURCE_DIR)/drivers

#for en8851 sdk build in freeRTOS
#AIR_SDK_DIR = $(BSP_DRIVER_SOURCE_DIR)/en8851
#AIR_SDK_DIR = $(shell pwd)/../../air_system
#AIR_SDK_OBJECT_DIR = $(BSP_DRIVER_SOURCE_DIR)/en8851/obj
AIR_SDK_LIB_DIR = $(AIR_SDK_DIR)/build/image
#AIR_SDK_LIB_DIR = $(BSP_DRIVER_SOURCE_DIR)/en8851/air_sdk/lib
ifeq ($(AIR_MW_SUPPORT),ENABLED)
AIR_MW_SYSTEM = ../../air_magic_wand/air_mw_system
HTMP_TOOL_DIR = $(AIR_MW_SYSTEM)/html/tools/makefsdata
AIR_MW_HTTP_CGI_DIR = $(AIR_MW_SYSTEM)/html/cgi
MW_OBJ = $(AIR_MW_SYSTEM)/obj
endif

APP_SOURCE_DIR = $(shell pwd)/../app
APP_COMMON_MINIMAL_DIR = $(APP_SOURCE_DIR)/Demo/Common/Minimal
APP_COMMON_RESET_DIR = $(APP_SOURCE_DIR)/common/btn_reset
APP_LWIP_ROOT_DIR = $(APP_SOURCE_DIR)/lwip
APP_LWIP_EN8851_PORT_DIR = $(APP_LWIP_ROOT_DIR)/ports/AIR_8851
APP_LWIP_2_1_2_DIR = $(APP_SOURCE_DIR)/lwip/lwip-2.1.2/src
APP_LWIP_API_DIR = $(APP_LWIP_2_1_2_DIR)/api
APP_LWIP_CORE_DIR = $(APP_LWIP_2_1_2_DIR)/core
APP_LWIP_NETIF_DIR = $(APP_LWIP_2_1_2_DIR)/netif
APP_LWIP_APPS_DIR = $(APP_LWIP_2_1_2_DIR)/apps
APP_LWIP_INCLUDE_DIR = $(APP_LWIP_2_1_2_DIR)/include
APP_LWIP_APPS_HTTP_DIR = $(APP_LWIP_APPS_DIR)/http
APP_LWIP_APPS_SNMP_DIR = $(APP_LWIP_APPS_DIR)/snmp
APP_LWIP_APPS_MBEDTLS_DIR = $(APP_LWIP_2_1_2_DIR)/apps/mbedtls-2.16.9
APP_MBEDTLS_LIB_DIR = $(APP_LWIP_APPS_MBEDTLS_DIR)/library
APP_MBEDTLS_INC_DIR = $(APP_LWIP_APPS_MBEDTLS_DIR)/include
APP_HTTP_CLIENT_DIR = $(APP_LWIP_APPS_DIR)/freertos_http_client
APP_COMMON_DIR = $(APP_LWIP_APPS_DIR)/common_utilities
APP_HTTP_CLIENT_CORE_DIR = $(APP_HTTP_CLIENT_DIR)/coreHTTP/source
APP_HTTP_CLIENT_PARSER_DIR = $(APP_HTTP_CLIENT_CORE_DIR)/dependency/3rdparty/http_parser
APP_TELNETD_DIR = $(APP_LWIP_APPS_DIR)/freertos_telnetd

ifeq ($(USE_HTTPS),1)
APP_MBEDTLS_CFG_DIR = $(APP_LWIP_APPS_MBEDTLS_DIR)/configs
APP_ALTCP_TLS_DIR = $(APP_LWIP_APPS_DIR)/altcp_tls
endif
APP_LWIP_APPS_MQTT_DIR = $(APP_LWIP_APPS_DIR)/mqtt

# The linker script
LDSCRIPT = $(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/$(PLATFORM).ld
ifeq ($(AIR_8851_SUPPORT),1)
ifeq ($(DATA_STACK_IN_GDMPSRAM),1)
SAGSCRIPT = $(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/$(PLATFORM)_gdmpSram.sag
else
SAGSCRIPT = $(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/$(PLATFORM).sag
endif
SAGTOOL = $(BSP_ARCH_NDS32_DIR)/util/nds_ldsag
endif

# Define the source files we have

# BSP source files
STARTUP_SRCS = $(BSP_ARCH_NDS32_DIR)/start.S

ifeq ($(PLATFORM),en8851)
BSP_SRCS = \
	$(BSP_ARCH_NDS32_DIR)/startup-nds32.c \
	$(BSP_ARCH_NDS32_DIR)/cache.c \
	$(BSP_SOURCE_DIR)/main.c \
	$(BSP_SOURCE_DIR)/utils/cmd_interpreter.c \
	$(BSP_SOURCE_DIR)/utils/cmd_shell.c \
	$(BSP_SOURCE_DIR)/utils/cmd_peripheral.c \
	$(BSP_SOURCE_DIR)/utils/i2c_cmd.c \
	$(BSP_SOURCE_DIR)/utils/gpio_cmd.c \
	$(BSP_SOURCE_DIR)/utils/chipscu_cmd.c \
	$(BSP_SOURCE_DIR)/utils/xmodem.c \
	$(BSP_SOURCE_DIR)/utils/tftp_client.c \
	$(BSP_SOURCE_DIR)/customer/customer_system.c \
	$(BSP_ARCH_NDS32_DIR)/util/util.c \
	$(BSP_ARCH_NDS32_DIR)/util/cmd_func.c \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/interrupt.c \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/irq.S \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/uart.c \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/FreeRTOS_tick_config.c \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/timer.c \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/hw_init.c

BSP_SRCS += \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/spinorwrite.c \
    $(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/nor_flash_write/spi_nor_entry.c	\
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/nor_flash_write/spinor/spi_controller.c	\
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/nor_flash_write/util/flash_util.c

else
ifeq ($(PLATFORM),en8852)

else
BSP_SRCS = \
	$(BSP_ARCH_NDS32_DIR)/startup-nds32.c \
	$(BSP_ARCH_NDS32_DIR)/cache.c \
	$(BSP_SOURCE_DIR)/main.c \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/$(PLATFORM).c \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/irq.S \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/uart.c \
	$(BSP_ARCH_NDS32_DIR)/$(PLATFORM)/FreeRTOS_tick_config.c
endif
endif

LIBC_SRCS = \
	$(BSP_ARCH_NDS32_DIR)/libc/stdio/do_printf.c \
	$(BSP_ARCH_NDS32_DIR)/libc/stdio/printf.c

# Kernel source files
FREERTOS_SRCS = \
	$(FREERTOS_SOURCE_DIR)/croutine.c \
	$(FREERTOS_SOURCE_DIR)/list.c \
	$(FREERTOS_SOURCE_DIR)/queue.c \
	$(FREERTOS_SOURCE_DIR)/tasks.c \
	$(FREERTOS_SOURCE_DIR)/timers.c \
	$(FREERTOS_SOURCE_DIR)/event_groups.c \
	$(FREERTOS_SOURCE_DIR)/stream_buffer.c \
	$(BSP_ARCH_NDS32_DIR)/MemMang/heap_4.c

PORT_SRCS = \
	$(BSP_ARCH_NDS32_DIR)/GCC/port.c \
	$(BSP_ARCH_NDS32_DIR)/GCC/portasm.S

RTOS_SRCS = $(FREERTOS_SRCS) $(PORT_SRCS)

# Driver source files
ifeq ($(PLATFORM),en8851)
DRIVER_SRCS = \
	$(BSP_DRIVER_SOURCE_DIR)/en8851/pdma.c \
        $(BSP_DRIVER_SOURCE_DIR)/en8851/air_i2c.c \
        $(BSP_DRIVER_SOURCE_DIR)/en8851/air_util.c \
        $(BSP_DRIVER_SOURCE_DIR)/en8851/hal_sco_i2c.c \
        $(BSP_DRIVER_SOURCE_DIR)/en8851/air_gpio.c \
        $(BSP_DRIVER_SOURCE_DIR)/en8851/hal_sco_gpio.c \
        $(BSP_DRIVER_SOURCE_DIR)/en8851/air_chipscu.c \
        $(BSP_DRIVER_SOURCE_DIR)/en8851/hal_sco_chipscu.c \
        $(BSP_DRIVER_SOURCE_DIR)/en8851/hwcfg_util.c

endif

# Demo source files
ifeq ($(AIR_8851_SUPPORT),1)
APP_MBEDTLS_LIB = \
	$(APP_MBEDTLS_LIB_DIR)/aes.c \
	$(APP_MBEDTLS_LIB_DIR)/aesni.c \
	$(APP_MBEDTLS_LIB_DIR)/arc4.c \
	$(APP_MBEDTLS_LIB_DIR)/aria.c \
	$(APP_MBEDTLS_LIB_DIR)/asn1parse.c \
	$(APP_MBEDTLS_LIB_DIR)/asn1write.c \
	$(APP_MBEDTLS_LIB_DIR)/base64.c \
	$(APP_MBEDTLS_LIB_DIR)/bignum.c \
	$(APP_MBEDTLS_LIB_DIR)/blowfish.c \
	$(APP_MBEDTLS_LIB_DIR)/camellia.c \
	$(APP_MBEDTLS_LIB_DIR)/ccm.c \
	$(APP_MBEDTLS_LIB_DIR)/certs.c \
	$(APP_MBEDTLS_LIB_DIR)/chacha20.c \
	$(APP_MBEDTLS_LIB_DIR)/chachapoly.c \
	$(APP_MBEDTLS_LIB_DIR)/cipher.c \
	$(APP_MBEDTLS_LIB_DIR)/cipher_wrap.c \
	$(APP_MBEDTLS_LIB_DIR)/cmac.c \
	$(APP_MBEDTLS_LIB_DIR)/ctr_drbg.c \
	$(APP_MBEDTLS_LIB_DIR)/debug.c \
	$(APP_MBEDTLS_LIB_DIR)/des.c \
	$(APP_MBEDTLS_LIB_DIR)/dhm.c \
	$(APP_MBEDTLS_LIB_DIR)/ecdh.c \
	$(APP_MBEDTLS_LIB_DIR)/ecdsa.c \
	$(APP_MBEDTLS_LIB_DIR)/ecjpake.c \
	$(APP_MBEDTLS_LIB_DIR)/ecp.c \
	$(APP_MBEDTLS_LIB_DIR)/ecp_curves.c \
	$(APP_MBEDTLS_LIB_DIR)/entropy.c \
	$(APP_MBEDTLS_LIB_DIR)/entropy_poll.c \
	$(APP_MBEDTLS_LIB_DIR)/error.c \
	$(APP_MBEDTLS_LIB_DIR)/gcm.c \
	$(APP_MBEDTLS_LIB_DIR)/havege.c \
	$(APP_MBEDTLS_LIB_DIR)/hkdf.c \
	$(APP_MBEDTLS_LIB_DIR)/hmac_drbg.c \
	$(APP_MBEDTLS_LIB_DIR)/md2.c \
	$(APP_MBEDTLS_LIB_DIR)/md4.c \
	$(APP_MBEDTLS_LIB_DIR)/md5.c \
	$(APP_MBEDTLS_LIB_DIR)/md.c \
	$(APP_MBEDTLS_LIB_DIR)/md_wrap.c \
	$(APP_MBEDTLS_LIB_DIR)/memory_buffer_alloc.c \
	$(APP_MBEDTLS_LIB_DIR)/net_sockets.c \
	$(APP_MBEDTLS_LIB_DIR)/nist_kw.c \
	$(APP_MBEDTLS_LIB_DIR)/oid.c \
	$(APP_MBEDTLS_LIB_DIR)/padlock.c \
	$(APP_MBEDTLS_LIB_DIR)/pem.c \
	$(APP_MBEDTLS_LIB_DIR)/pk.c \
	$(APP_MBEDTLS_LIB_DIR)/pkcs11.c \
	$(APP_MBEDTLS_LIB_DIR)/pkcs12.c \
	$(APP_MBEDTLS_LIB_DIR)/pkcs5.c \
	$(APP_MBEDTLS_LIB_DIR)/pkparse.c \
	$(APP_MBEDTLS_LIB_DIR)/pk_wrap.c \
	$(APP_MBEDTLS_LIB_DIR)/pkwrite.c \
	$(APP_MBEDTLS_LIB_DIR)/platform.c \
	$(APP_MBEDTLS_LIB_DIR)/platform_util.c \
	$(APP_MBEDTLS_LIB_DIR)/poly1305.c \
	$(APP_MBEDTLS_LIB_DIR)/ripemd160.c \
	$(APP_MBEDTLS_LIB_DIR)/rsa.c \
	$(APP_MBEDTLS_LIB_DIR)/rsa_internal.c \
	$(APP_MBEDTLS_LIB_DIR)/sha1.c \
	$(APP_MBEDTLS_LIB_DIR)/sha256.c \
	$(APP_MBEDTLS_LIB_DIR)/sha512.c \
	$(APP_MBEDTLS_LIB_DIR)/ssl_cache.c \
	$(APP_MBEDTLS_LIB_DIR)/ssl_ciphersuites.c \
	$(APP_MBEDTLS_LIB_DIR)/ssl_cli.c \
	$(APP_MBEDTLS_LIB_DIR)/ssl_cookie.c \
	$(APP_MBEDTLS_LIB_DIR)/ssl_srv.c \
	$(APP_MBEDTLS_LIB_DIR)/ssl_ticket.c \
	$(APP_MBEDTLS_LIB_DIR)/ssl_tls.c \
	$(APP_MBEDTLS_LIB_DIR)/threading.c \
	$(APP_MBEDTLS_LIB_DIR)/timing.c \
	$(APP_MBEDTLS_LIB_DIR)/version.c \
	$(APP_MBEDTLS_LIB_DIR)/version_features.c \
	$(APP_MBEDTLS_LIB_DIR)/x509.c \
	$(APP_MBEDTLS_LIB_DIR)/x509_create.c \
	$(APP_MBEDTLS_LIB_DIR)/x509_crl.c \
	$(APP_MBEDTLS_LIB_DIR)/x509_crt.c \
	$(APP_MBEDTLS_LIB_DIR)/x509_csr.c \
	$(APP_MBEDTLS_LIB_DIR)/x509write_crt.c \
	$(APP_MBEDTLS_LIB_DIR)/x509write_csr.c \
	$(APP_MBEDTLS_LIB_DIR)/xtea.c

APP_COMMON_UTILITY = \
	$(APP_COMMON_DIR)/mbedtls_freertos/mbedtls_freertos_port.c

APP_WEBSOCKET_CLIENT_DIR = \
	$(APP_COMMON_DIR)/platform/posix/transport/src/sockets_posix.c \
	$(APP_COMMON_DIR)/platform/posix/transport/src/mbedtls_posix.c \
	$(APP_COMMON_DIR)/mbedtls_freertos/mbedtls_error.c \
	$(APP_HTTP_CLIENT_DIR)/http_demo_websocket/http_demo_websocket.c \
	$(APP_HTTP_CLIENT_DIR)/http_demo_websocket/wsClient.c \
	$(APP_HTTP_CLIENT_DIR)/http_demo_websocket/wsCommon.c

APP_HTTPD = \
	$(APP_LWIP_APPS_HTTP_DIR)/fs.c \
	$(APP_LWIP_APPS_HTTP_DIR)/httpd.c

APP_SNMP = \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_asn1.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_core.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_mib2.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_mib2_interfaces.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_mib2_ifmib_ifxtable.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_mib2_system.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_msg.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_netconn.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_pbuf_stream.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_raw.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_scalar.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_snmpv2_framework.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_snmpv2_usm.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_table.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_threadsync.c \
	$(APP_LWIP_APPS_SNMP_DIR)/snmp_traps.c

APP_TELNETD = \
	$(APP_TELNETD_DIR)/telnetd.c

APP_MQTT += \
	$(APP_LWIP_APPS_MQTT_DIR)/mqtt.c

APP_SRCS = \
	$(APP_LWIP_API_DIR)/api_lib.c \
	$(APP_LWIP_API_DIR)/api_msg.c \
	$(APP_LWIP_API_DIR)/err.c \
	$(APP_LWIP_API_DIR)/if_api.c \
	$(APP_LWIP_API_DIR)/netbuf.c \
	$(APP_LWIP_API_DIR)/netdb.c \
	$(APP_LWIP_API_DIR)/netifapi.c \
	$(APP_LWIP_API_DIR)/sockets.c \
	$(APP_LWIP_API_DIR)/tcpip.c \
	$(APP_LWIP_CORE_DIR)/def.c \
	$(APP_LWIP_CORE_DIR)/inet_chksum.c \
	$(APP_LWIP_CORE_DIR)/init.c \
	$(APP_LWIP_CORE_DIR)/ip.c \
	$(APP_LWIP_CORE_DIR)/ipv4/dhcp.c \
	$(APP_LWIP_CORE_DIR)/ipv4/etharp.c \
	$(APP_LWIP_CORE_DIR)/ipv4/icmp.c \
	$(APP_LWIP_CORE_DIR)/ipv4/ip4_addr.c \
	$(APP_LWIP_CORE_DIR)/ipv4/ip4.c \
	$(APP_LWIP_CORE_DIR)/ipv4/ip4_frag.c \
	$(APP_LWIP_CORE_DIR)/ipv6/dhcp6.c \
	$(APP_LWIP_CORE_DIR)/ipv6/ethip6.c \
	$(APP_LWIP_CORE_DIR)/ipv6/icmp6.c \
	$(APP_LWIP_CORE_DIR)/ipv6/inet6.c \
	$(APP_LWIP_CORE_DIR)/ipv6/ip6.c \
	$(APP_LWIP_CORE_DIR)/ipv6/ip6_addr.c \
	$(APP_LWIP_CORE_DIR)/ipv6/ip6_frag.c \
	$(APP_LWIP_CORE_DIR)/ipv6/mld6.c \
	$(APP_LWIP_CORE_DIR)/ipv6/nd6.c \
	$(APP_LWIP_CORE_DIR)/mem.c \
	$(APP_LWIP_CORE_DIR)/memp.c \
	$(APP_LWIP_CORE_DIR)/netif.c \
	$(APP_LWIP_CORE_DIR)/pbuf.c \
	$(APP_LWIP_CORE_DIR)/raw.c \
	$(APP_LWIP_CORE_DIR)/stats.c \
	$(APP_LWIP_CORE_DIR)/sys.c \
	$(APP_LWIP_CORE_DIR)/tcp.c \
	$(APP_LWIP_CORE_DIR)/tcp_in.c \
	$(APP_LWIP_CORE_DIR)/tcp_out.c \
	$(APP_LWIP_CORE_DIR)/timeouts.c \
	$(APP_LWIP_CORE_DIR)/udp.c \
	$(APP_LWIP_NETIF_DIR)/ethernet.c \
	$(APP_LWIP_EN8851_PORT_DIR)/ethernetif.c \
	$(APP_LWIP_EN8851_PORT_DIR)/sys_arch.c \
	$(APP_LWIP_APPS_DIR)/tftp/tftp.c \
	$(APP_MBEDTLS_LIB) \
	$(APP_COMMON_UTILITY)

ifeq ($(AIR_MW_SUPPORT),ENABLED)
APP_SRCS += $(APP_HTTPD)
APP_SRCS += $(APP_SNMP)
APP_SRCS +=	$(APP_LWIP_APPS_DIR)/ping/ping.c
APP_SRCS += $(APP_MQTT)
endif

ifeq ($(AIR_WEBSOCKET_DEMO),ENABLED)
APP_SRCS += $(APP_WEBSOCKET_CLIENT_DIR)
endif

ifneq (, $(filter ENABLED, $(AIR_MW_SUPPORT) $(AIR_WEBSOCKET_DEMO)))
APP_SRCS +=	$(APP_LWIP_CORE_DIR)/dns.c
endif

ifeq ($(AIR_APP_SFP),ENABLED)
APP_SRCS += $(APP_SOURCE_DIR)/sfp/air_app_sfp.c
endif

APP_SRCS += $(APP_COMMON_RESET_DIR)/btn_reset.c

ifeq ($(USE_HTTPS),1)
APP_SRCS += \
	$(APP_LWIP_CORE_DIR)/altcp.c \
	$(APP_LWIP_CORE_DIR)/altcp_alloc.c \
	$(APP_LWIP_CORE_DIR)/altcp_tcp.c \
	$(APP_ALTCP_TLS_DIR)/altcp_tls_mbedtls.c \
	$(APP_ALTCP_TLS_DIR)/altcp_tls_mbedtls_mem.c
endif

APP_LWIP_HEADER_INCLUDE_DIR = \
	-I$(APP_LWIP_ROOT_DIR) \
	-I$(APP_LWIP_EN8851_PORT_DIR)/include \
	-I$(APP_LWIP_INCLUDE_DIR) \
	-I$(APP_LWIP_INCLUDE_DIR)/netif \
	-I$(APP_MBEDTLS_INC_DIR) \
	-I$(APP_COMMON_DIR)/include/logging \
	-I$(APP_COMMON_DIR)/platform/posix/transport/include \
	-I$(APP_COMMON_DIR)/mbedtls_freertos \
	-I$(APP_HTTP_CLIENT_CORE_DIR)/include \
	-I$(APP_HTTP_CLIENT_CORE_DIR)/interface \
	-I$(APP_HTTP_CLIENT_PARSER_DIR) \
	-I$(APP_HTTP_CLIENT_DIR)/common/include \
	-I$(APP_HTTP_CLIENT_DIR)/http_demo_mbed_tls \
	-I$(APP_LWIP_APPS_SNMP_DIR)
else
APP_SRCS = \
	$(APP_SOURCE_DIR)/Demo/main_blinky.c \
	$(APP_SOURCE_DIR)/Demo/main_full.c \
	$(APP_SOURCE_DIR)/Demo/reg_test.S \
	$(APP_COMMON_MINIMAL_DIR)/AbortDelay.c \
	$(APP_COMMON_MINIMAL_DIR)/BlockQ.c \
	$(APP_COMMON_MINIMAL_DIR)/blocktim.c \
	$(APP_COMMON_MINIMAL_DIR)/countsem.c \
	$(APP_COMMON_MINIMAL_DIR)/death.c \
	$(APP_COMMON_MINIMAL_DIR)/dynamic.c \
	$(APP_COMMON_MINIMAL_DIR)/EventGroupsDemo.c \
	$(APP_COMMON_MINIMAL_DIR)/flop.c \
	$(APP_COMMON_MINIMAL_DIR)/GenQTest.c \
	$(APP_COMMON_MINIMAL_DIR)/IntSemTest.c \
	$(APP_COMMON_MINIMAL_DIR)/MessageBufferAMP.c \
	$(APP_COMMON_MINIMAL_DIR)/QueueOverwrite.c \
	$(APP_COMMON_MINIMAL_DIR)/recmutex.c \
	$(APP_COMMON_MINIMAL_DIR)/semtest.c \
	$(APP_COMMON_MINIMAL_DIR)/StaticAllocation.c \
	$(APP_COMMON_MINIMAL_DIR)/MessageBufferDemo.c \
	$(APP_COMMON_MINIMAL_DIR)/StreamBufferInterrupt.c \
	$(APP_COMMON_MINIMAL_DIR)/TaskNotify.c \
	$(APP_COMMON_MINIMAL_DIR)/TimerDemo.c
endif


# Define all object files.
OUT_DIR = $(shell pwd)/build
FREERTOS_DIR = $(shell pwd)/..
SRCS = $(APP_SRCS)
OBJS = $(addprefix $(OUT_DIR)/,$(subst $(FREERTOS_DIR),,$(patsubst %.S,%.o,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,${SRCS})))))

#=============================================================================

platform_srcs = $(STARTUP_SRCS) $(BSP_SRCS) $(LIBC_SRCS) $(PORT_SRCS) $(DRIVER_SRCS)
platform_lib = platform.a
platform_lib_dep = $(addprefix $(OUT_DIR)/,$(subst $(FREERTOS_DIR),,$(patsubst %.S,%.o,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,${platform_srcs})))))
lib_list += $(platform_lib)

freertos_srcs = $(FREERTOS_SRCS)
freertos_lib = freertos.a
freertos_lib_dep = $(addprefix $(OUT_DIR)/,$(subst $(FREERTOS_DIR),,$(patsubst %.S,%.o,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,${freertos_srcs})))))
lib_list += $(freertos_lib)

#=============================================================================

### Verbosity control. Use 'make V=1' to get verbose builds.

ifeq ($(V),1)
TRACE_CC  =
TRACE_C++ =
TRACE_LD  =
TRACE_AR  =
TRACE_AS  =
Q=
else
TRACE_CC  = @echo "  CC       " $<
TRACE_C++ = @echo "  C++      " $<
TRACE_LD  = @echo "  LD       " $@
TRACE_AR  = @echo "  AR       " $@
TRACE_AS  = @echo "  AS       " $<
Q=@
endif

# Compilation options
CMODEL = -mcmodel=large

INCLUDES = \
	-I$(APP_COMMON_MINIMAL_DIR)/../include \
	-I$(BSP_SOURCE_DIR) \
	-I$(BSP_SOURCE_DIR)/include \
	-I$(BSP_SOURCE_DIR)/customer \
	-I$(AIR_SDK_DIR)/air_sdk/include \
	-I$(AIR_SDK_DIR)/air_sdk/src/inc \
	-I$(AIR_SDK_DIR)/air_sdk/src/inc/osal \
	-I$(AIR_SDK_DIR)/air_sdk/src/inc/cmlib \
	-I$(AIR_SDK_DIR)/app/diag_shell/inc/parser \
	-I$(AIR_SDK_DIR)/app/sdk_ref \
	$(APP_LWIP_HEADER_INCLUDE_DIR) \
	-I$(BSP_ARCH_NDS32_DIR) \
	-I$(BSP_ARCH_NDS32_DIR)/$(PLATFORM) \
	-I$(BSP_ARCH_NDS32_DIR)/util \
	-I$(FREERTOS_SOURCE_DIR)/../include \
	-I$(BSP_ARCH_NDS32_DIR)/GCC \
	-I$(APP_COMMON_RESET_DIR)/inc \

ifneq ("$(wildcard $(AIR_MW_SYSTEM))", "")
INCLUDES += -I$(AIR_MW_SYSTEM)/cmd/inc/parser
INCLUDES += -I$(AIR_MW_SYSTEM)/util/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/init/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/httpd/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/snmp/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/db/freeRTOS/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/os_api/freeRTOS/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/html/cgi
INCLUDES += -I$(AIR_MW_SYSTEM)/switching/icmp_client/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/mqttd/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/driver/sfp/inc
INCLUDES += -I$(AIR_MW_SYSTEM)/ref
endif

ifeq ($(AIR_WEBSOCKET_DEMO),ENABLED)
INCLUDES += -I$(APP_HTTP_CLIENT_DIR)/http_demo_websocket
endif

ifeq ($(AIR_8851_SUPPORT),1)
INCLUDES += -I$(CROSS_COMPILE_PATH)/../lib/gcc/nds32le-elf/4.9.4/include $(CMODEL)
endif

CFLAGS += $(INCLUDES) $(DEFINES) -include $(PLATFORM).h

ASFLAGS = -D__ASSEMBLY__ $(CFLAGS) -c

LDFLAGS = -T$(LDSCRIPT) $(OPTIM) -static -nostartfiles -Wl,--gc-sections,--undefine=__rtos_signature_freertos_v10_1_1 \
	$(CMODEL)

GCCVER 	= $(shell $(GCC) --version | grep gcc | cut -d" " -f9)

# Add `-fno-delete-null-pointer-checks` flag if the compiler supports it.
# GCC assumes that programs cannot safely dereference null pointers,
# and that no code or data element resides there.
# However, 0x0 is the vector table memory location, so the test must not be removed.
ifeq ($(shell $(CC) -fno-delete-null-pointer-checks -E - 2>/dev/null >/dev/null </dev/null ; echo $$?),0)
	CFLAGS += -fno-delete-null-pointer-checks
	LDFLAGS += -fno-delete-null-pointer-checks
endif

### Make variables

DEFINES = -DCFG_$(ADDR)

ifeq ($(AIR_8851_SUPPORT),1)
DEFINES += -DAIR_8851_SUPPORT -DDUAL_IMAGE=$(DUAL_IMAGE)
ifneq ("$(wildcard $(AIR_MW_SYSTEM))", "")
DEFINES += -DAIR_MW_SUPPORT
HTTPSCHUNKED = -DHTTP_TRANSFER_ENCODING_CHUNKED
DEFINES += $(HTTPSCHUNKED)
endif

ifeq ($(AIR_WEBSOCKET_DEMO),ENABLED)
DEFINES += -DAIR_WEBSOCKET_DEMO
endif

ifeq ($(IS_FPGA_STAGE),1)
DEFINES += -DIS_FPGA_STAGE
endif

ifeq ($(DATA_STACK_IN_GDMPSRAM),1)
DEFINES += -DDATA_STACK_IN_GDMPSRAM
endif

ifeq ($(IRQ_DBG_SUPPORT),1)
DEFINES += -DIRQ_DBG_SUPPORT
endif

ifeq ($(USE_HTTPS),1)
DEFINES += -DENABLE_HTTPS -I$(APP_MBEDTLS_CFG_DIR) -DMBEDTLS_CONFIG_FILE='"$(APP_MBEDTLS_CFG_DIR)/config-freertos-en8851.h"'
endif

ifeq ($(AIR_APP_SFP),ENABLED)
DEFINES += -DAIR_EN_APP_SFP
endif

endif ###AIR_8851_SUPPORT###

ifneq ($(shell echo | $(CROSS_COMPILE_PATH)/$(CC) -E -dM - | grep __NDS32_ISA_V3M__ > /dev/null && echo V3M),V3M)
	ifeq ($(USE_CACHEWB),1)
		DEFINES += -DCFG_CACHE_ENABLE -DCFG_CACHE_WRITEBACK
	endif

	ifeq ($(USE_CACHEWT),1)
		DEFINES += -DCFG_CACHE_ENABLE -DCFG_CACHE_WRITETHROUGH
	endif
endif

# Compilation rules
.SUFFIXES : %.o %.c %.cpp %.S

LOGFILE = $(shell pwd)/log.Makefile

export CC CFLAGS LOGFILE AIR_SDK_DIR
ifneq ("$(wildcard $(AIR_MW_SYSTEM))", "")
ifdef CMW_CONFIG
include $(CMW_CONFIG)
else
include $(AIR_MW_SYSTEM)/config.mk
endif
include $(AIR_MW_SYSTEM)/common.mk
CFLAGS += $(MW_CFLAGS)
export AIR_MW_SYSTEM OS_MODULE HTTPSCHUNKED
endif

ifeq ($(AIR_APP_SFP),ENABLED)
ifeq ($(AIR_SUPPORT_SFP),ENABLED)
$(error AIR_APP_SFP and AIR_SUPPORT_SFP cannot be enabled at the same time.)
endif
endif

lib_list += $(AIR_SDK_LIB_DIR)/air_sdk.a

AIR_OBJ =
ifeq ($(AIR_MW_SUPPORT),ENABLED)
AIR_OBJ += $(wildcard $(MW_OBJ)/*.o)
endif

$(OUT_DIR)/%.o: ../%.c
	$(TEST_PATH) $(dir $@) || $(MKDIR) $(dir $@)
	$(Q)$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $< >> $(LOGFILE) 2>&1
	$(Q)$(CC) -MM -MT $@ $(CFLAGS) $(EXTRA_CFLAGS)  $< > $(patsubst %.o,%.d, $@)

$(OUT_DIR)%.o: ../%.S
	$(TEST_PATH) $(dir $@) || $(MKDIR) $(dir $@)
	$(Q)$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $< >> $(LOGFILE) 2>&1
	$(Q)$(CC) -MM -MT $@ $(CFLAGS) $(EXTRA_CFLAGS) $< > $(patsubst %.o,%.d, $@)


BUILD_DEPENDENCY =
CLEAN_DEPENDENCY =

BUILD_DEPENDENCY += MW_SYSTEM AIR_SDK WEBPAGE_C $(PROG).elf

all: $(BUILD_DEPENDENCY)

MW_SYSTEM:
ifeq ("$(wildcard $(AIR_MW_SYSTEM))", "")
	echo "Not find air_mw_system"
else
	echo "Find air_mw_system"
	@echo "" > $(LOGFILE)
	make -C $(AIR_MW_SYSTEM) clean all
endif

WEBPAGE_C:
ifeq ("$(wildcard $(AIR_MW_SYSTEM))", "")
	echo "Not find air_mw_system"
else
	echo "Find webpage_c"
	echo "Gen makefsdata.bin"
	make -C $(HTMP_TOOL_DIR) all
	echo "Gen fsdata.c"
	$(HTMP_TOOL_DIR)/makefsdata.bin -11
	echo "Copy fsdata.c"
	mv fsdata.c $(APP_LWIP_APPS_HTTP_DIR)
	make -C $(AIR_MW_HTTP_CGI_DIR)
endif


AIR_SDK:
	@echo "" > $(LOGFILE)
	make -C $(AIR_SDK_DIR)

ifeq ($(AIR_EN_WARNING_AS_ERROR),ENABLED)
#$(platform_lib): EXTRA_CFLAGS=-Werror
endif
$(platform_lib): $(platform_lib_dep)
	@echo "..........Building library" $@
	$(AR) -rc $@	$?


ifeq ($(AIR_EN_WARNING_AS_ERROR),ENABLED)
$(freertos_lib): EXTRA_CFLAGS=-Werror
endif
$(freertos_lib): $(freertos_lib_dep)
	@echo "..........Building library" $@
	$(AR) -rc $@	$?

$(PROG).elf : $(OBJS) Makefile $(lib_list)
ifeq ($(AIR_8851_SUPPORT),1)
	rm -f $(LDSCRIPT)
	$(SAGTOOL) $(SAGSCRIPT) -o $(LDSCRIPT)
	gcc -o fwheader ../tool/fwheader.c
endif
	$(TRACE_LD) >> $(LOGFILE) 2>&1
	$(Q)$(CC) -o $@ $(LDFLAGS) $(OBJS) -Wl,-start-group $(lib_list) -Wl,-end-group $(AIR_OBJ) >> $(LOGFILE) 2>&1
	$(OBJCOPY) -O binary -S $(PROG).elf $(PROG).bin
ifeq ($(AIR_8851_SUPPORT),1)
	./fwheader  $(PROG).bin  $(PROG).img
	$(OBJDUMP) -t -x -d -C -h -S $@ > $(PROG).elf.txt
else
	$(OBJDUMP) -S $(PROG).elf > $(PROG).asm
endif
	@echo Completed

clean : $(CLEAN_DEPENDENCY)
	@rm -f $(OBJS) $(OBJS:.o=.d)
	@rm -f $(PROG).elf
	@rm -f $(PROG).bin
	make clean -C $(AIR_SDK_DIR)
	@rm -f $(LOGFILE)
	@rm -rf $(OUT_DIR)
ifeq ($(AIR_8851_SUPPORT),1)
	@rm -f fwheader
	@rm -f $(PROG).img
	@rm -f $(PROG).elf.txt
	@rm -f $(APP_LWIP_APPS_HTTP_DIR)/fsdata.c
	@rm -f $(platform_lib_dep) $(platform_lib_dep:.o=.d)
	@rm -f $(freertos_lib_dep) $(freertos_lib_dep:.o=.d)
	@rm -f $(lib_list)
else
	@rm -f $(PROG).asm
endif

# Automatic dependency generation
ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif

