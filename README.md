# InWorldz aperture texture and mesh server

Aperture is a simple not quite compliant HTTP/1.1 server that sends texture and
mesh data to viewer software connected to the InWorldz grid

### Build requirements
- [CMake 3.8.2 or later](https://cmake.org/)
- [Conan 0.28.1 or later](https://www.conan.io/)

You will need to add the SlideWave LLC conan repository to obtain prebuilt packages:
```
conan remote add slidewave https://api.bintray.com/conan/slidewavellc/conan-libs
```

### Sample cmake build command for Windows 64-bit

```
conan install . -s build_type=Debug -s arch=x86_64 -s compiler.runtime=MTd --build=missing
mkdir build && cd build
cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_BUILD_TYPE=Debug
```
