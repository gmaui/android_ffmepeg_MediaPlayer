@echo on

@echo "autojavah..."

@set Out_Path=jni/libffplayer/
@set Source_Path=java
@set Target_Class=cn.whaley.sh.mediaplayer.FFMediaPlayer.FFMediaPlayer
@set Android_jar=C:\Users\hc\AppData\Local\Android\sdk\platforms\android-23\android.jar

@cd %~dp0
javah -d %Out_Path% -classpath %Android_jar%;%Source_Path% -jni %Target_Class%

@echo "done..."

pause 