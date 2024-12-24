################################################################################
AIR_SYSTEM      := $(shell pwd)

################################################################################
include                 $(AIR_SYSTEM)/toolchain.mk
################################################################################
AIR_SYSTEM_BUILD        := $(AIR_SYSTEM)/build

################################################################################
TARGET_NAME             :=

################################################################################
ARCH                    := andes
AIR_HOST_ENDIAN         := AIR_EN_HOST_32_BIT_LITTLE_ENDIAN

OS_TYPE                 := freertos
OS_PATH                 := $(AIR_SYSTEM)/../freertos

#WORK_SPACE              := USER_SPACE
WORK_SPACE              := KERNEL_SPACE

BUILD_TYPE              := STATIC_LIB
#BUILD_TYPE              := SHARED_LIB

################################################################################
ENABLED_SWITCH_CHIPS    := en8853c
ENABLED_PHY_CHIPS       := en8808

################################################################################
# Enable the following feature by set the value to "ENABLED"
# Disable the following feature by set the value to "DISABLED"
################################################################################
AIR_EN_DEBUG            := ENABLED

################################################################################
AIR_EN_I2C_PHY          := ENABLED

################################################################################
AIR_EN_COMPILER_SUPPORT_FUNCTION   := ENABLED

################################################################################
AIR_EN_COMPILER_SUPPORT_LONG_LONG  := ENABLED

################################################################################
AIR_EN_64BIT_ADDR       := DISABLED

################################################################################
# Enable the following applications by set the value to "ENABLED"
# Disable the following applications by set the value to "DISABLED"
################################################################################
AIR_EN_SFP_LED          := ENABLED

################################################################################
AIR_EN_I2C_BITBANG      := DISABLED

################################################################################
AIR_EN_LED_CLK_SYNC     := ENABLED

################################################################################
AIR_EN_DMA_RESERVED     := DISABLED
AIR_OS_MEMORY_SZ        := 4080
AIR_DMA_RESERVED_SZ     := 16

################################################################################
export AIR_SYSTEM_BUILD
export TARGET_NAME
export ARCH
export AIR_HOST_ENDIAN
export OS_TYPE
export OS_PATH
export WORK_SPACE
export BUILD_TYPE
export ENABLED_SWITCH_CHIPS
export ENABLED_PHY_CHIPS
export AIR_EN_DEBUG
export AIR_EN_I2C_PHY
export AIR_EN_I2C_TO_I2C_ACCESS
export AIR_EN_COMPILER_SUPPORT_FUNCTION
export AIR_EN_COMPILER_SUPPORT_LONG_LONG
export AIR_EN_64BIT_ADDR
export AIR_EN_LED_CLK_SYNC
export AIR_EN_I2C_BITBANG
export AIR_EN_DMA_RESERVED AIR_OS_MEMORY_SZ AIR_DMA_RESERVED_SZ
