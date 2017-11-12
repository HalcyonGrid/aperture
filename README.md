# InWorldz aperture texture and mesh server

Aperture is a simple not quite compliant HTTP/1.1 server that sends texture and
mesh data to viewer software connected to the InWorldz grid

### Build requirements
- [CMake 3.8.2 or later](https://cmake.org/)
- [Conan 0.28.1 or later](https://www.conan.io/)

### Sample cmake build command for Windows 64-bit

```
conan install . -s build_type=Debug -arch=x86_64 --build=missing
mkdir build && cd build
cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_BUILD_TYPE=Debug
```
