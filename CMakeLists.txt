PROJECT(aperture)
cmake_minimum_required(VERSION 3.2)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(conanbuildinfo.cmake)
conan_basic_setup()

find_package(Threads REQUIRED)

set(Aperture_srcs
    aperture/AppLog.cpp aperture/Asset.cpp
    aperture/AssetServer.cpp aperture/AuthChallengeMsg.cpp
    aperture/AuthResponseMsg.cpp aperture/AuthStatusMsg.cpp
    aperture/ClientRequestMsg.cpp aperture/CloudFilesAsset.cpp
    aperture/CloudFilesConnector.cpp aperture/CloudFilesGetWorker.cpp
    aperture/connection.cpp aperture/connection_manager.cpp
    aperture/HttpRequestHandler.cpp aperture/IAsset.cpp
    aperture/IAssetServer.cpp aperture/mime_types.cpp
    aperture/RackspaceAuthorizer.cpp
    aperture/reply.cpp aperture/request_handler.cpp
    aperture/request_parser.cpp aperture/server.cpp
    aperture/ServerResponseMsg.cpp aperture/Settings.cpp
    aperture/SHA1.cpp aperture/stdafx.cpp aperture/TokenBucket.cpp
    aperture/Validator.cpp aperture/WhipURI.cpp aperture/win_main.cpp
    ${PROTO_SRCS}
    )

set(INCLUDES ${INCLUDES} deps/json_spirit_v4.06/json_spirit )
add_subdirectory(deps/json_spirit_v4.06)

add_definitions(${DEFS})
include_directories(${INCLUDES})
add_executable( aperture ${Aperture_srcs})
target_link_libraries(aperture ${CMAKE_THREAD_LIBS_INIT} ${CONAN_LIBS})
