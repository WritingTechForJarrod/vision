###Vision Module
#Detecting facial features and movement
Current dependencies:
* python 2.7.6
 * opencv
 * numpy
* C++11
 * DLib
  * other packages may be required by DLib  
Complete list of python modules in use at runtime in modules.txt

#For building in Dlib in Windows:
You'll need C++11 support, so VS2015 or later  
If you want to use g++ or another compiler try some flavour of MinGW-64, be sure to install pthread libs  
Building examples in Visual Studio 2015:
* start a new project
* in the solution explorer select Properties(wrench)->Configuration Properties->C/C++ and in Additional Include Directories add the root dlib folder, for me that's something like c:/dlib-19.2, NOT c:/dlib-19.2/dlib/
 * if the above does not work, check the configuration, the include paths might need to be added to the Win32 config if you are using x64 config
* add the dlib/all/source.cpp to your project source files 
* add the dlib/examples/* file you want to use
* make sure the debug symbols used are available, select Tools->Options->Debugging->Symbols and allow the Microsoft symbol servers access
* if you need to open and save jpeg images you'll need to add `#define DLIB_JPEG_SUPPORT' and link to libjpeg
 * right click on your project, select Properties->Configuration Properties->C/C++->Preprocessor and add DLIB_JPEG_SUPPORT to Preprocessor Definitions
 * to install libjpeg open Tools->NuGet Package Manager->Manage NuGet Packages for Solution... and search 'libjpeg' in the Browse tab
 * install the package you find
 
#For building Dlib with opencv dependencies in Windows:  
Follow the above instructions, then prepare yourself for a journey of great discovery.  
The opencv version assumed here is 2.4.13. This version is built with VS2012 but Dlib requires VS2015 for C++11 support. Therefore the opencv libs and dlls have to be built before we can use it with the Dlib examples. This is a multi-step process, hopefully this description is accurate for your purposes. For this example I am building for 64-bit windows.  
The steps:  
* download opencv 2.4.13 and install somewhere
* download CMake for windows and add to the command line path 
 * Start->Edit System Environment Variables->Environment Variables...->Edit the path, add dir CMake executable is in to path
* add VS2015 command line tool to VS2015 menu  
 * in VS2015 go to Tools->External Tools  
 * click Add
 * call tool Title 'Command Prompt' or something
 * in Command put `%comspec%`  
 * in Arguments put the path to 'VsDevCmd.bat' for VS2015, for me it is `C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat`  
 * click 'OK' or something  
* create VS2015 project in preparation for building opencv for VS2015  
 * start VS2015 command prompt by selecting Tools->External Tools->Command Prompt (or whatever you named it)  
 * navigate to the sources directory of opencv, for me it is 'C:\Users\Maxim\Downloads\opencv\sources' (I am lazy)  
 * type `cmake -G "Visual Studio 14 2015 Win64"`, this should be create the VS2015 project  
* build the debug and release versions of opencv  
 * open the VS2015 project that was created in the last step, for me 'C:\Users\Maxim\Downloads\opencv\sources\OpenCV.sln'  
 * build Debug and then Release versions, this will take a long time (I think > 1 hour for me)
 * libs and dlls should now be in the following directories:  
  * debug libs in '~\opencv\sources\lib\Debug'  
  * release libs in '~\opencv\sources\lib\Release'  
  * debug dlls in '~\opencv\sources\bin\Debug'  
  * release dlls in '~\opencv\sources\bin\Release'  

#If you are building a dll that uses Dlib:
* add dlib/all/source.cpp to sources but disable precompiled headers for this file by clicking on source.cpp, selecting Properties->Precompiled Headers and change the field Precompiled Header to Not Using Precompiled Header
