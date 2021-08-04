# Build webrays on Linux

To build webrays and the examples you will at least need something like the following command, depending on your distro's package manager.

```
apt-get install build-essential cmake libsdl2-dev 
```

Next, we will require a build of ANGLE. Follow the instructions from ANGLE's repository to build ANGLE from source. Make sure to build a shared version with `is_component_build = false` in order to link dependencies into the build targets.

With SDL2 and ANGLE installed we simply point our console to the cloned webrays folder and execute 

```
cmake -S . -B build -DBUILD_EXAMPLES=1 -DEGL_LIBRARY=path/to/ANGLE/libEGL.so -DGLES_LIBRARY=path/to/ANGLE/libGLESv2.so -DCMAKE_BUILD_TYPE=Release
```

Alternatively, you can use a precompiled version of ANGLE. For example if you happen to have Visual Studio Code installed, it already comes with a compiled version of ANGLE.

```
cmake -S . -B build -DBUILD_EXAMPLES=1 -DEGL_LIBRARY=/usr/share/code/libEGL.so -DGLES_LIBRARY=/usr/share/code/libGLESv2.so -DCMAKE_BUILD_TYPE=Release
```

Build with

```
cmake --build build --config Release
```

Finally, install with

```
cmake --install build --config Release
```

If you prefer a non-default installation path, you can pass `-DCMAKE_INSTALL_PREFIX=/custom/install/path` to the first `cmake` command.

## Using webrays in your own application

After building or installing, it is very easy to use webrays in your own application. An important aspect that you need to consider is that your application and webrays need to use the same libGLESv2 library from ANGLE in order to properly work on the same context. This is easily taken care of by accordingly setting the `rpath` during compilation.

For example, the `hello_triangle` example can be manually built using the following command

```
clang hello_triangle.c -Wl,-rpath,\$ORIGIN -I /usr/include/SDL2 -I/custom/install/path/webrays/include -o hello_triangle -L/custom/install/path/webrays/lib -lSDL2 -lEGL -lGLESv2 -lwebrays
```