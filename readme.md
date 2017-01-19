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
If you have any questions email me at `max[at]embeddedprofessional.com`  
Thanks to all the sources I used to figure this out, I looked at eleventy billion different sources to be able to finally build and run even the examples, I wish I could thank each one but that is lost to history  

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
The opencv version assumed here is 2.4.13. This version is built with VS2012 but Dlib requires VS2015 for C++11 support. Therefore the opencv libs and dlls have to be built before we can use it with the Dlib examples. If you get an error like `mismatch detected for '_MSC_VER': value '1900' doesn't match value '1800'` when attempting to build examples the mismatch between compiler versions used for the opencv and Dlib builds will require you to build them yourself. This is a multi-step process, hopefully this description is accurate for your purposes. For this example I am building for 64-bit windows.  
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
  * type `cmake -G "Visual Studio 14 2015 Win64"`, this should create the VS2015 project  
* build the debug and release versions of opencv  
  * open the VS2015 project that was created in the last step, for me 'C:\Users\Maxim\Downloads\opencv\sources\OpenCV.sln'  
  * build Debug and then Release versions, this will take a long time (I think > 1 hour for me)
  * libs and dlls should now be in the following directories (~ is whatever the root of these is for you):  
    * debug libs in '~\opencv\sources\lib\Debug'  
    * release libs in '~\opencv\sources\lib\Release'  
    * debug dlls in '~\opencv\sources\bin\Debug'  
    * release dlls in '~\opencv\sources\bin\Release'  
* you can close the VS2015 project and whatever command windows you have open and open your project
* in your project add links to the lib and dll dependencies, for me this was needed for building 'webcam_face_pose_ex.cpp'  
  * right click on your project and choose the Release configuration for Platform x64  
  * choose Configuration Properties->C/C++->General from the left menu window
  * in Additional Include Directories add paths to your Release dlls, for me 'C:/Users/Maxim/Downloads/opencv/sources/bin/Release'  
  * choose Configuration Properties->VC++ Directories from the left menu window  
    * in Library Directories add the path to your Release libs, for me 'C:\Users\Maxim\Downloads\opencv\sources\lib\Release'  
  * choose Configuration Properties->Linker->Input from the left menu window  
    * in Addtional Dependencies add the .dlls you will need for the example you are building, for me 'opencv_highgui2413.lib;opencv_core2413.lib;opencv_video2413.lib;' were added, not sure if I needed the last one
   * if you need to build in Debug mode follow the above but choose the Debug configuration from the properties pop-up and repeat the steps choosing the debug dlls and libs (debug libs and dlls have 'd' appended to the end of them)
* build your project  
* add the .dlls you will need to the directory in which your executable is created  
  * for my example my project is called 'facial_detection', so in the '~/facial_detection/x64/Release' directory I had to add:
    * jpeg.dll
    * opencv_core2413.dll
    * opencv_highgui2413.dll
* I also had to add the 'shape_predictor_68_face_landmarks.dat' file to the directory the .exe was in, i got it at:
  * https://github.com/AKSHAYUBHAT/TensorFace/blob/master/openface/models/dlib/shape_predictor_68_face_landmarks.dat
* open the directory the .exe is in and run it by double-clicking it, if I run it from VS2015 it closes prematurely  
  
I hope this helps you! Any errors, omissions, questions email me at `max[at]embeddedprofessional.com`    

#If you are building a dll that uses Dlib:
* add dlib/all/source.cpp to sources but disable precompiled headers for this file by clicking on source.cpp, selecting Properties->Precompiled Headers and change the field Precompiled Header to Not Using Precompiled Header
