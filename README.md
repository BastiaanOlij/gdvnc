# Godot GDNative VNC library

This module works as a VNC client for Godot. The desktop of the VNC Server will be represented as a texture.

Note that this is fully based on libvnc, which just like the original VNC implementation from AT&T, is licensed under GPL. That means I have no choice but to also release the source code here under GPL.

I have only tested building this on Windows, the instructions below may need to be altered for other platforms.

You need the following installed to run this:
- A C and CPP compiler (tested this with Microsoft Visual Studio 2017)
- CMake
- Python 2 (not 3)
- Scons
- Godot 3 (tested with RC3)

Most of the 3rd party libraries use cmake, but both cpp_bindings and our project here uses scons

Building
--------
Open up a terminal and CD into the folder into which you've cloned this project

We need to start with making sure our submodules are up to date:
```
git submodule init
git submodule update
```

*note* see section patches below for any issues around patching.

Building zlib:
```
cd zlib
mkdir build
cd build
cmake .. -DCMAKE_GENERATOR_PLATFORM=x64
cmake --build . --config Release
cd ..\..
```

The zlib build process renames the zconf.h file to zconf.h.include and creates a new one in your build folder.
Copy the one in your build folder back into the zlib folder or libpng won't compile.

Building libpng
```
cd libpng
mkdir build
cd build
cmake .. -DCMAKE_GENERATOR_PLATFORM=x64 -DZLIB_INCLUDE_DIR=../../zlib/ -DZLIB_LIBRARY=../../zlib/build/Release/zlibstatic
cmake --build . --config Release
cd ..\..
```

Building libvnc:
```
cd libvncserver
mkdir build
cd build
cmake .. -DCMAKE_GENERATOR_PLATFORM=x64 -DZLIB_INCLUDE_DIR=../../zlib/ -DZLIB_LIBRARY=../../zlib/build/Release/zlibstatic -DPNG_PNG_INCLUDE_DIR=../../libpng/ -DPNG_LIBRARY=../../libpng/build/Release/libpng16_static
cmake --build . --config Release
cd ..\..
```

Building cpp_bindings:
```
cd cpp_bindings
scons platform=windows target=release generate_bindings=yes headers=../godot_headers godotbinpath=<pathtoexe>/godot.exe 
cd ..
```

And finally build our library:
```
scons platform=windows target=release
```

Patches
-------
Note, at the time of writing this there are two issues in libvnc that you need to patch:
- https://github.com/LibVNC/libvncserver/pull/185
- https://github.com/LibVNC/libvncserver/pull/215
