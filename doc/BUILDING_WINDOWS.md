# Build webrays on Windows

ANGLE has seen great adaption on Windows. It is the default graphics backend on the 3 major browsers, namely Chrome, Firefox and Edge. The `cmake` script first attempts to find the ANGLE libraries in the installation of one of those browsers. Alternatively, you can compile ANGLE yourself using the instructions on their github [repo](https://github.com/google/angle)

You will also need to download the SDL2 development libraries from the [official site](https://www.libsdl.org/download-2.0.php) or some other source.

**!!IMPORTANT!!** For maximum compatibility it is advices to execute the following commands from a console shell that is setup with the Visual Studio environment of your choice. In most recent Visual Studio versions, the batch files to setup the proper environment can be found at `C:\Program Files (x86)\Microsoft Visual Studio\<Version>\<Edition>\VC\Auxiliary\Build`

Simply run the `vcvars64.bat` batch file in your current console.

```
cmake -S . -B build -DBUILD_EXAMPLES=1 -DSDL2_PATH=/path/to/SDL2
```
or for older Visual Studio versions you should also specify the platform (x64)
```
cmake -G "Visual Studio 15 2017" -A x64 -S . -B build -DBUILD_EXAMPLES=1 -DSDL2_PATH=/path/to/SDL2
```

2.
"D:\Tools\cmake-3.18.1-win64-x64\bin\cmake.exe" --build build_web --config Release

3.
"D:\Tools\cmake-3.18.1-win64-x64\bin\cmake.exe" --install build_web --config Release

4.
cd bin

5.
python2 -m SimpleHTTPServer
or
python3 -m http.server

-- Build webrays for Native development

-- Win32

To build with examples issue 

"D:\Tools\cmake-3.18.1-win64-x64\bin\cmake" -S . -B build -DCMAKE_INSTALL_PREFIX=. -DBUILD_EXAMPLES=1 -DSDL2_PATH="D:\C++\Projects\Phasmatic\webrays\Dependencies\SDL2"


