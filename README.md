# InWorldz aperture texture and mesh server

Aperture is a simple not quite compliant HTTP/1.1 server that sends texture and
mesh data to viewer software connected to the InWorldz grid

### Build requirements
- [CMake 3.8.2 or later](https://cmake.org/)
- [Conan 0.28.1 or later](https://www.conan.io/)

You will need to add the Bincrafters conan repositores to obtain prebuilt packages:
```
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
```

### Sample cmake build command for Windows 64-bit

```cmd
conan install . -s build_type=Debug -s arch=x86_64 -s compiler.runtime=MTd --build=missing
mkdir build
cd build && cmake -G "Visual Studio 15 2017 Win64" .. -DCMAKE_BUILD_TYPE=Debug
```

## Directions for Ubuntu 16.04 LTS 64-bit

```bash
conan install . -s build_type=Debug -s arch=x86_64 --build=missing
mkdir build
cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug
```

## A bare-bones `aperture.cfg` file

```ini
http_listen_port = <port number>
caps_token = 2960079
whip_url = whip://<password>@<ip>:32700
```
