PROJECT( aperture )
cmake_minimum_required(VERSION 3.2)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package( Threads REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_STATIC_RUNTIME OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(Boost_USE_DEBUG_RUNTIME 1)
endif()


set(DEFS ${DEFS} -DBOOST_ALL_DYN_LINK -DBOOST_ALL_NO_LIB)

find_package( Boost COMPONENTS thread system program_options date_time
    chrono REQUIRED)
set(INCLUDES ${INCLUDES} ${Boost_INCLUDE_DIR})
set(LIBS ${LIBS} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_DATE_TIME_LIBRARY}
    ${Boost_CHRONO_LIBRARY})


find_package( Curl REQUIRED)
set(INCLUDES ${INCLUDES} ${CURL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${CURL_LIBRARIES})


find_package( Protobuf REQUIRED)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    protoinclude/bcl.proto protoinclude/StratusAsset.proto)
set(INCLUDES ${INCLUDES} ${PROTOBUF_INCLUDE_DIRS} ${CMAKE_BINARY_DIR})
set(LIBS ${LIBS} ${PROTOBUF_LIBRARIES})
message(${PROTOBUF_LIBRARIES})

set ( Aperture_srcs
        aperture/AppLog.cpp aperture/Asset.cpp
        aperture/AssetServer.cpp aperture/AuthChallengeMsg.cpp
        aperture/AuthResponseMsg.cpp aperture/AuthStatusMsg.cpp
        aperture/ClientRequestMsg.cpp aperture/CloudFilesAsset.cpp
        aperture/CloudFilesConnector.cpp aperture/CloudFilesGetWorker.cpp
        aperture/connection.cpp aperture/connection_manager.cpp
        aperture/HttpRequestHandler.cpp aperture/IAsset.cpp
        aperture/IAssetServer.cpp aperture/mime_types.cpp
        aperture/posix_main.cpp aperture/RackspaceAuthorizer.cpp
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
target_link_libraries(aperture ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${LIBS} )
