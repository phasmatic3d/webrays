# Build webrays for the Web

Follow the instructions to install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).

After cloning and installing the latest toolchain, simply execute the scripts to set up the latest environment.

For Linux and Mac OS

```
./emsdk activate latest
source ./emsdk_env.sh
```
or for Windows
```
"path\to\emsdk\emsdk" activate latest
"path\to\emsdk\emsdk_env.bat"
```

To prepare build targets, on Linux and Mac OS, we simply execute

```
emcmake cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

**!!IMPORTANT!!** On Windows the `make` utility is not available by default. We have 2 options. If Visual Studio is installed, we can use `nmake` which is a tool similar to `make` that comes with the standard Visual Studio installation. To be able to use `nmake` we need to setup the Visual Studio environment on our current console. In most recent Visual Studio versions, the batch files to setup the proper environment can be found at `C:\Program Files (x86)\Microsoft Visual Studio\<Version>\<Edition>\VC\Auxiliary\Build`

The console is now set up with both the Emscripten and Visual Studio environments. From here, we can issue the following command

```
emcmake cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
```

If Visual Studio is not installed we can download a Windows version of `make`, e.g. from [here](http://gnuwin32.sourceforge.net/packages/make.htm) (Make sure to download both the `Binary` and the `Dependencies`). 

With `make` installed we execute the following command

```
emcmake cmake -S . -B build -G "Unix Makefiles" -DCMAKE_MAKE_PROGRAM=/path/to/make -DCMAKE_BUILD_TYPE=Release
```

Build the library and examples

```
cmake.exe --build build --config Release
```

Install the library and examples. 

```
cmake.exe --install build --config Release
```

We can set the `CMAKE_INSTALL_PREFIX` variable during the initial `cmake` command to control the installation directory. E.g. `-DCMAKE_INSTALL_PREFIX=/path/to/install`

then, 

```
cd /path/to/install/webrays
```

And start a local server with 

```
python2 -m SimpleHTTPServer
```
or
```
python3 -m http.server
```

Point your browser to 
```
http://localhost:8000/
```
And select one of the examples to start developing