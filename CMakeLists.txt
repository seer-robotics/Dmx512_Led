cmake_minimum_required(VERSION 3.5.2)

# WIN32 means windows, including Win64
# APPLE means macOS
# UNIX means Unix like system

project(Dmx512_Led)

set(RBK_PLUGIN "RBK_PLUGIN")

set(RBK_PATH "" CACHE PATH "RoboKit SDK path")
IF(RBK_PATH STREQUAL "")
	message(FATAL_ERROR "RBK_PATH is not set")
ENDIF()

list(APPEND CMAKE_MODULE_PATH ${RBK_PATH}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(GetSubprojectStatus)
# is sub project
get_subproject_status(IS_SUBPROJECT)

# configs when build as independent project
# -----------------------------------------
IF(NOT IS_SUBPROJECT)

# config file
# -----------
include(rbkConfig)

# global settings
# ---------------
include(GlobalSettings)

# Include our cmake macros
# ------------------------
include(CMakeHelpers)

# logging
# -------
include(CMakeLogging)

# build type (affects debugging symbols and optimization)
# -------------------------------------------------------
include(BuildType)

# compiler flags
# --------------
include(CompilerFlags)

# find libs
# ---------
message("Finding libraries:")
message("-----------------------------------------------------")
include(GetThirdPartyLibs)

# default includes
# ----------------
include(DefaultIncludes)

# other settings
# --------------
include(RBKOtherSettings)

ENDIF()

# custom config file
# ------------------
include(rbkPluginConfig)

message("")

# add subfolders
# --------------
add_subdirectory(src)