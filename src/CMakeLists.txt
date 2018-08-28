# output directory
# ----------------
include(SetOutputDirectory)

# pre process
# -----------
IF(WIN32)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS 
											${RBK_PROTO_DIR}/message_battery.proto
											${RBK_PROTO_DIR}/message_movetask.proto
											${RBK_PROTO_DIR}/message_odometer.proto
											${RBK_PROTO_DIR}/message_controller.proto
											${RBK_PROTO_DIR}/message_header.proto)
ENDIF()

# define
# ------
add_definitions(-D__RBK_PLUGIN__="${PROJECT_NAME}")

# source
# ------
aux_source_directory(. SRCS)

# include
# -------
IF(UNIX)
	IF(NOT IS_SUBPROJECT)
		include_directories(BEFORE ${RBK_PATH}/include/robokit/protocol)
	ELSE()
		include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src/robokit/protocol)
	ENDIF()
ENDIF()

# link
# ----

# for VS solution file
# --------------------
IF(WIN32)
	file(GLOB HEADERS "*.h")
	source_group("include" FILES ${HEADERS})
	source_group("src" FILES ${SRCS})
ENDIF()

# target
# ------
IF(WIN32)
	# windows
	include(generate_product_version)
	generate_product_version(
        VersionFilesOutputVariable
        NAME "${PROJECT_NAME}"
        ICON ${RBK_PATH}/robokit.ico
        VERSION_MAJOR ${${PROJ_NAME}_VERSION_MAJOR}
        VERSION_MINOR ${${PROJ_NAME}_VERSION_MINOR}
        VERSION_PATCH ${${PROJ_NAME}_VERSION_PATCH}
        COMPANY_NAME ${ROBOTKIT_COMPANY_NAME}
        ORIGINAL_FILENAME "[${ROBOTKIT_NAME}] ${PROJECT_NAME}"
        INTERNAL_NAME "[${ROBOTKIT_NAME}] ${PROJECT_NAME}"
        FILE_DESCRIPTION "[${ROBOTKIT_NAME}] ${PROJECT_NAME}"
	)
	
	add_library(${PROJECT_NAME} SHARED
			${SRCS}
			${HEADERS}
			${PROTO_SRCS}
			${PROTO_HDRS}
			${VersionFilesOutputVariable})
	target_link_libraries(${PROJECT_NAME}
			core
			utils
			foundation
			chasis
			${LUA_LIBRARIES}
			${Protobuf_LIBRARIES}
			${SQLite3_LIBRARY})
ELSEIF(UNIX)
	IF(APPLE)
        # macOS
		add_library(${PROJECT_NAME} SHARED
				${SRCS})
		target_link_libraries(${PROJECT_NAME}
				core
				utils
				protocol
				foundation
				boost_system-mt
				boost_thread-mt)
	ELSE(APPLE)
        # linux
		add_library(${PROJECT_NAME} SHARED
				${SRCS})
		target_link_libraries(${PROJECT_NAME}
				core
				utils
				protocol
				foundation
				boost_system-mt
				boost_thread-mt)
	ENDIF()
ENDIF()

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "rbk_plugin")

# other
# -----
