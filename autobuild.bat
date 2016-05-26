@echo on

echo "autobuild..."

set my_ndk=C:\Users\hc\AppData\Local\Android\android-ndk-r10e\ndk-build
::set cur=%~dp0
set my_verbose=V=1
set my_platform=APP_PLATFORM=android-15
set my_path=NDK_PROJECT_PATH=null
set my_abi=APP_ABI=armeabi
set my_debug=app\build\intermediates\ndk\debug
set my_mk=APP_BUILD_SCRIPT=app\src\main\jni\Android.mk
set my_pre_mk=APP_BUILD_SCRIPT=app\src\main\jni\ffmpeg-3.0.1\android\arm\Android.mk
set my_obj_out=NDK_OUT=app\build\intermediates\ndk\debug\obj
set my_libs_out=NDK_LIBS_OUT=app\build\intermediates\jniLibs\debug\
set my_out=NDK_LIBS_OUT=app\src\main\jniLibs\

::echo "processing pre..."
cd %~dp0
::start %my_ndk% %my_verbose% %my_path% %my_pre_mk% %my_platform% %my_obj_out% %my_libs_out% %my_abi%

::@sleep 5000

rd/s/q %my_debug%
@echo "processing cur..."
start %my_ndk% %my_verbose% %my_path% %my_mk% %my_platform% %my_obj_out% %my_libs_out% %my_abi%

@timeout /t 8 /nobreak > nul
copy app\build\intermediates\jniLibs\debug\armeabi\*.so app\src\main\jniLibs\armeabi\

@echo "done..."
pause 