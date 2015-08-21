# InWorldz aperture texture and mesh server

Aperture is a simple not quite compliant HTTP/1.1 server that sends texture and
mesh data to viewer software connected to the InWorldz grid

### Sample cmake build command for windows

cmake .. -DCMAKE_BUILD_TYPE=Debug -DCURL_LIBRARY=C:\lib\curl-7.44.0\builds\libcurl-vc-x64-debug-dll-ipv6-sspi-winssl\lib\libcurl_debug.lib -DCURL_INCLUDE_DIR=C:\lib\curl-7.44.0\include -DPROTOBUF_LIBRARY=C:\lib\protobuf-2.6.1\vsprojects\x64\Debug\libprotobuf.lib -DPROTOBUF_INCLUDE_DIR=C:\lib\protobuf-2.6.1\src -DPROTOBUF_PROTOC_EXECUTABLE=C:\lib\protobuf-2.6.1\vsprojects\x64\Debug\protoc.exe
-G "Visual Studio 14 2015 Win64"
