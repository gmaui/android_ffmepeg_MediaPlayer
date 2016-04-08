@echo on

@echo "autojavah..."

@set Out_Path=jni
@set Source_Path=java
@set Target_Class=cn.whaley.sh.mediaplayer.FFMediaPlayer.FFMediaPlayerJNI

@cd %~dp0
javah -d %Out_Path% -classpath %Source_Path% -jni %Target_Class%

@echo "done..."

pause 