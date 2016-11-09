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
* add the dlib/all/source.cpp to your project source files 
* add the dlib/examples/* file you want to use
* make sure the debug symbols used are available, select Tools->Options->Debugging->Symbols and allow the Microsoft symbol servers access
* if you need to open and save jpeg images you'll need to add `#define DLIB_JPEG_SUPPORT' and link to libjpeg
 * right click on your project, select Properties->Configuration Properties->C/C++->Preprocessor and add DLIB_JPEG_SUPPORT to Preprocessor Definitions
 * to install libjpeg open Tools->NuGet Package Manager->Manage NuGet Packages for Solution... and search 'libjpeg' in the Browse tab
 * install the package you find
 
#If you are building a dll that uses Dlib:
* add dlib/all/source.cpp to sources but disable precompiled headers for this file by clicking on source.cpp, selecting Properties->Precompiled Headers and change the field Precompiled Header to Not Using Precompiled Header
