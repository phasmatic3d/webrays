# Build webrays on Linux

To build webrays and the examples you will need to install XCode from the App Store in order to get a full development environment setup.

Next, we will require a build of ANGLE. Follow the instructions from ANGLE's repository to build ANGLE from source. Make sure to build a shared version with `is_component_build = false` in order to link dependencies into the build targets.

**!IMPORTANT!** The official development packages for Mac OS from the SDL website come as a self-contained framework. Frameworks package all dependencies, including libraries, headers and other resources in a single package. To use the SDL framework for building the examples we will need to set the `SDL2_PATH` variable during `cmake` configuration to point directly to the SDL2.framework instead of the folder.

```
/Applications/CMake.app/Contents/bin/cmake -S . -B build -DEGL_LIBRARY=/path/to/angle/libEGL.dylib -DGLES_LIBRARY=/path/to/angle/libGLESv2.dylib -DBUILD_EXAMPLES=1 -DSDL2_PATH=/path/to/SDL2.framework
```

Alternatively, we can use a precompiled version of ANGLE. For example if you happen to have Visual Studio Code installed, it already comes with a compiled version of ANGLE.

```
/Applications/CMake.app/Contents/bin/cmake -S . -B build -DEGL_LIBRARY=/Applications/Visual\ Studio\ Code.app/Contents/Frameworks/Electron\ Framework.framework/Versions/A/Libraries/libEGL.dylib -DGLES_LIBRARY=/Applications/Visual\ Studio\ Code.app/Contents/Frameworks/Electron\ Framework.framework/Versions/A/Libraries/libGLESv2.dylib -DBUILD_EXAMPLES=1 -DSDL2_PATH=/path/to/SDL2.framework
```

If all goes well, build with

```
/Applications/CMake.app/Contents/bin/cmake --build build --config Release
```

Finally, install with

```
/Applications/CMake.app/Contents/bin/cmake --install build --config Release
```

If you prefer a non-default installation path, you can pass `-DCMAKE_INSTALL_PREFIX=/custom/install/path` to the first `cmake` command.

## Using webrays in your own application

After building or installing, it is very easy to use webrays in your own application. An important apsect that you need to consider is that your application and webrays need to use the same libGLESv2 library from ANGLE in order to properly work on the same context. This is easily taken care by accordingly setting the `rpath` during compilation.

For example, the `hello_triangle` example can be manually built using the following command

```
clang hello_triangle.c -Wl,-rpath,@executable_path -I/custom/install/path/webrays/include -I/path/to/SDL2.framework/Headers -o hello_triangle -L/custom/install/path/webrays/lib -F/path/to/sdl -framework SDL2 -lEGL -lGLESv2 -lwebrays
```