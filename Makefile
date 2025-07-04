################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Top-level application make file.
#
################################################################################
# \copyright
# Copyright 2021-2025, Cypress Semiconductor Corporation (an Infineon company)
# SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################


################################################################################
# Basic Configuration
################################################################################

# Type of ModusToolbox Makefile Options include:
#
# COMBINED    -- Top Level Makefile usually for single standalone application
# APPLICATION -- Top Level Makefile usually for multi project application
# PROJECT     -- Project Makefile under Application
#
MTB_TYPE=COMBINED

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make modlibs' from command line), which will also update Eclipse IDE launch
# configurations. If TARGET is manually edited, ensure TARGET_<BSP>.mtb with a
# valid URL exists in the application, run 'make getlibs' to fetch BSP contents
# and update or regenerate launch configurations for your IDE.
TARGET=EVAL_PMG1_B1_DRP

# Name of application (used to derive name of final linked file).
#
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=mtb-example-pmg1b1-usbpd-drp

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC provided with ModusToolbox IDE
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
#
# If CONFIG is manually edited, ensure to update or regenerate launch configurations
# for your IDE.
CONFIG=Debug

# If set to "true" or "1", display full command-lines when building.
VERBOSE=


################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS=PMG1_PD3_DRP

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES=

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES=

# Add additional defines to the build process (without a leading -D).
# Enabled PD revision 3.0 support, VBus OV Fault Protection and Deep Sleep mode in idle states.
DEFINES=CY_PD_SINK_ONLY=0 CY_PD_SOURCE_ONLY=0 CY_PD_REV3_ENABLE=1 CY_PD_CBL_DISC_DISABLE=0 \
                VBUS_OVP_ENABLE=1 VBUS_UVP_ENABLE=1 VBUS_OCP_ENABLE=0 SYS_DEEPSLEEP_ENABLE=1 \
                CY_PD_USB4_SUPPORT_ENABLE=1 CY_PD_HW_DRP_TOGGLE_ENABLE=1 CY_PDUTILS_TIMER_TYPE=3

        DEFINES+=VCONN_OCP_ENABLE=1 VCONN_SCP_ENABLE=1

PDSTACK_DEFINES=CY_PD_VCONN_DISABLE=0 CY_USE_CONFIG_TABLE=0 CY_PD_BIST_STM_ENABLE=1\
        CY_PD_BIST_MODE_DISABLE=0 CY_PD_EPR_ENABLE=0 CY_PD_EPR_AVS_ENABLE=0

USBPD_DEFINES=CCG_CF_HW_DET_ENABLE=1 PMG1B1_USB_CHARGER=1

FAULT_DEFINES=DISABLE_CC_OVP_OVER_SLEEP=0 BB_ILIM_DET_ENABLE=1\
        VREG_BROWN_OUT_DET_ENABLE=1 VREG_INRUSH_DET_ENABLE=1 PDL_VOUTBB_RCP_ENABLE=0

CHARGING_DEFINES=BATTERY_CHARGING_ENABLE=0

SOLUTION_DEFINES=VBUS_CTRL_TRIM_ADJUST_ENABLE=1 VBUS_CTRL_TYPE_P1=4 CY_PD_VBUS_CF_EN=1\
        VBUS_IN_DISCHARGE_EN=1 VBUS_C_DISCHG_DS=1 VBTR_ENABLE=1 \
        IBTR_ENABLE=1 CCG_PD_BLOCK_ALWAYS_ON=1 BB_PWM_ASYNC_MODE_ENABLE=1\
        CY_USBPD_CGND_SHIFT_ENABLE=0

DEFINES +=$(PDSTACK_DEFINES)
DEFINES +=$(USBPD_DEFINES)
DEFINES +=$(FAULT_DEFINES)
DEFINES +=$(CHARGING_DEFINES)
DEFINES +=$(SOLUTION_DEFINES)


# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS=

# Additional / custom linker flags.
LDFLAGS=

# Additional / custom libraries to link in to the application.
LDLIBS=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=

# Variable set to "PMG1" to enable support for the PMG1 parts and BSPs.
export CY_SUPPORTED_KITS=PMG1

################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION> field
# begins with $$ASSET_REPO$$, then the repo is deposited in the path specified by
# the CY_GETLIBS_SHARED_PATH variable. The default location is one directory level
# above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

# Absolute path to the compiler's "bin" directory.
#
# The default depends on the selected TOOLCHAIN (GCC_ARM uses the ModusToolbox
# IDE provided compiler by default).
CY_COMPILER_PATH=


# Locate ModusToolbox IDE helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    $(HOME)/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)

# If you install ModusToolbox IDE in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder). Make sure you use forward slashes.
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS). On Windows, use forward slashes.)
endif

$(info Tools Directory: $(CY_TOOLS_DIR))

include $(CY_TOOLS_DIR)/make/start.mk
