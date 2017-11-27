cmake_minimum_required(VERSION 3.6)

PROJECT(aperture CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)

add_subdirectory(cmake)
include(ucm)

IF (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
	"Build type. One of: Debug Release RelWithDebInfo" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_STANDARD 14)

include(conanbuildinfo.cmake)
conan_basic_setup()

ucm_set_runtime(STATIC)

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
#find_package(Boost REQUIRED COMPONENTS chrono date_time program_options system thread)

# generate proto files
set(Aperture_PROTO
    protoinclude/bcl.proto
    protoinclude/StratusAsset.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${Aperture_PROTO})

set(Aperture_SRCS
    aperture/AppLog.cpp
    aperture/Asset.cpp
    aperture/AssetServer.cpp 
    aperture/AuthChallengeMsg.cpp
    aperture/AuthResponseMsg.cpp
    aperture/AuthStatusMsg.cpp
    aperture/ClientRequestMsg.cpp
    aperture/CloudFilesAsset.cpp
    aperture/CloudFilesConnector.cpp
    aperture/CloudFilesGetWorker.cpp
    aperture/connection.cpp
    aperture/connection_manager.cpp
    aperture/HttpRequestHandler.cpp
    aperture/IAsset.cpp
    aperture/IAssetServer.cpp
    aperture/mime_types.cpp
    aperture/RackspaceAuthorizer.cpp
    aperture/reply.cpp 
    aperture/request_handler.cpp
    aperture/request_parser.cpp
    aperture/server.cpp
    aperture/ServerResponseMsg.cpp
    aperture/Settings.cpp
    aperture/SHA1.cpp 
    aperture/stdafx.cpp
    aperture/TokenBucket.cpp
    aperture/Validator.cpp
    aperture/WhipURI.cpp
    aperture/win_main.cpp
    ${PROTO_SRCS}
    )

set(Aperture_HDRS
    aperture/AppLog.h
    aperture/Asset.h
    aperture/AssetServer.h
    aperture/AssetSizeCalculator.h
    aperture/AuthChallengeMsg.h
    aperture/AuthResponseMsg.h
    aperture/AuthStatusMsg.h
    aperture/byte.h
    aperture/ClientRequestMsg.h
    aperture/CloudFilesAsset.h
    aperture/CloudFilesConnector.h
    aperture/CloudFilesGetWorker.h
    aperture/connection.hpp
    aperture/connection_manager.hpp
    aperture/Finally.h
    aperture/header.hpp
    aperture/HttpRequestHandler.h
    aperture/IAsset.h
    aperture/IAssetServer.h
    aperture/lru_cache.h
    aperture/mime_types.hpp
    aperture/RackspaceAuthorizer.h
    aperture/reply.hpp 
    aperture/request.hpp
    aperture/request_handler.hpp
    aperture/request_parser.hpp
    aperture/server.hpp
    aperture/ServerResponseMsg.h
    aperture/Settings.h
    aperture/SHA1.h
    aperture/stdafx.h
    aperture/targetver.h
    aperture/TokenBucket.h
    aperture/Validator.h
    aperture/Version.h
    aperture/WhipURI.h
    ${PROTO_HDRS}
    )

LIST(APPEND Aperture_SRCS ${Aperture_HDRS})
SET_SOURCE_FILES_PROPERTIES(${Aperture_HDRS}
							PROPERTIES HEADER_FILE_ONLY TRUE)

LIST(APPEND Aperture_SRCS ${Aperture_PROTO})
SET_SOURCE_FILES_PROPERTIES(${Aperture_PROTO}
							PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("Prototype Files" FILES ${Aperture_PROTO})

# generated proto files are saved in the build directory
include_directories(${CMAKE_CURRENT_BINARY_DIR}/)

add_executable(aperture 
    ${Aperture_SRCS}
    )
target_link_libraries(aperture 
    ${CMAKE_THREAD_LIBS_INIT}
    ${CONAN_LIBS}
    )

IF (WIN32)
  SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT aperture)
ENDIF(WIN32)
