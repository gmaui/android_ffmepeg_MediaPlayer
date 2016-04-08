@echo on

echo "autojavah..."
setlocal EnableDelayedExpansion

set ndk=C:\Users\hc\AppData\Local\Android\android-ndk-r10e\ndk-build
set cur=C:\Users\hc\AndroidStudioProjects\MediaPlayer
set verbose=V=1
set platform=APP_PLATFORM=android-15
set path=NDK_PROJECT_PATH=null
set abi=APP_ABI=armeabi
set mk=APP_BUILD_SCRIPT=%cur%\app\build\intermediates\ndk\debug\Android.mk
set obj_out=%cur%\app\build\intermediates\ndk\debug\obj
::set libs_out=app\build\intermediates\jniLibs\debug\armeabi
set libs_out=%cur%\app\src\main\jniLibs\armeabi

cd %~dp0
%ndk% %verbose% %platform% %path% %abi% %mk% %obj_out% %libs_out%

echo "done..."
pause 