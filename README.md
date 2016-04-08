# android_ffmepeg_MediaPlayer

1.third_part library:ffmpeg

a. download the latest version 3.0.1
b. edit ffmpeg-3.0.1/configure
   from:
   SLIBNAME_WITH_MAJOR='$(SLIBNAME).$(LIBMAJOR)'
   LIB_INSTALL_EXTRA_CMD='$$(RANLIB) "$(LIBDIR)/$(LIBNAME)"'
   SLIB_INSTALL_NAME='$(SLIBNAME_WITH_VERSION)'
   SLIB_INSTALL_LINKS='$(SLIBNAME_WITH_MAJOR) $(SLIBNAME)'
   
   to:
   SLIBNAME_WITH_MAJOR='$(SLIBPREF)$(FULLNAME)-$(LIBMAJOR)$(SLIBSUF)'
   LIB_INSTALL_EXTRA_CMD='$$(RANLIB) "$(LIBDIR)/$(LIBNAME)"'
   SLIB_INSTALL_NAME='$(SLIBNAME_WITH_MAJOR)'
   SLIB_INSTALL_LINKS='$(SLIBNAME)'
c. touch ffmpeg-3.0.1/android_build.sh as below:
   #!/bin/bash
   NDK=/usr/local/android-ndk-r10d
   SYSROOT=$NDK/platforms/android-15/arch-arm/
   TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64
   function build_one
   {
      ./configure \
          --prefix=$PREFIX \
          --enable-shared \
          --disable-static \
          --disable-doc \
          --disable-ffmpeg \
          --disable-ffplay \
          --disable-ffprobe \
          --disable-ffserver \
          --disable-avdevice \
          --disable-doc \
          --disable-symver \
          --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
          --target-os=linux \
          --arch=arm \
          --enable-cross-compile \
          --sysroot=$SYSROOT \
          --extra-cflags="-Os -fpic $ADDI_CFLAGS" \
          --extra-ldflags="$ADDI_LDFLAGS" \
          $ADDITIONAL_CONFIGURE_FLAG
      make clean
      make
      make install
   }
   CPU=arm
   PREFIX=$(pwd)/android/$CPU 
   ADDI_CFLAGS="-marm"
   build_one
d.sudo chmod +x build_android.sh
   ./build_android.sh
e.ls -R android

bing_cao@DS-44:~/project/hisi310/external/ffmpeg-3.0.1$ ls -R android/
android/:
arm

android/arm:
Android.mk  include  lib

android/arm/include:
libavcodec  libavfilter  libavformat  libavutil  libswresample  libswscale

android/arm/include/libavcodec:
avcodec.h  avfft.h    dirac.h       dxva2.h  vaapi.h  vdpau.h    videotoolbox.h   xvmc.h
avdct.h    d3d11va.h  dv_profile.h  qsv.h    vda.h    version.h  vorbis_parser.h

android/arm/include/libavfilter:
avfilter.h  avfiltergraph.h  buffersink.h  buffersrc.h  version.h

android/arm/include/libavformat:
avformat.h  avio.h  version.h

android/arm/include/libavutil:
adler32.h     bswap.h           error.h         log.h                         pixelutils.h   threadmessage.h
aes.h         buffer.h          eval.h          lzo.h                         pixfmt.h       time.h
aes_ctr.h     camellia.h        ffversion.h     macros.h                      random_seed.h  timecode.h
attributes.h  cast5.h           fifo.h          mastering_display_metadata.h  rational.h     timestamp.h
audio_fifo.h  channel_layout.h  file.h          mathematics.h                 rc4.h          tree.h
avassert.h    common.h          frame.h         md5.h                         replaygain.h   twofish.h
avconfig.h    cpu.h             hash.h          mem.h                         ripemd.h       version.h
avstring.h    crc.h             hmac.h          motion_vector.h               samplefmt.h    xtea.h
avutil.h      des.h             imgutils.h      murmur3.h                     sha.h
base64.h      dict.h            intfloat.h      opt.h                         sha512.h
blowfish.h    display.h         intreadwrite.h  parseutils.h                  stereo3d.h
bprint.h      downmix_info.h    lfg.h           pixdesc.h                     tea.h

android/arm/include/libswresample:
swresample.h  version.h

android/arm/include/libswscale:
swscale.h  version.h

android/arm/lib:
libavcodec-57.so  libavfilter-6.so  libavformat-57.so  libavutil-55.so  libswresample-2.so  libswscale-4.so

f.edit the  Android.mk
LOCAL_PATH:= $(call my-dir)
 
 include $(CLEAR_VARS)
LOCAL_MODULE:= libavcodec
LOCAL_SRC_FILES:= lib/libavcodec-57.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)
 
 include $(CLEAR_VARS)
LOCAL_MODULE:= libavformat
LOCAL_SRC_FILES:= lib/libavformat-57.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)
 
 include $(CLEAR_VARS)
LOCAL_MODULE:= libswscale
LOCAL_SRC_FILES:= lib/libswscale-4.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)
 
 include $(CLEAR_VARS)
LOCAL_MODULE:= libavutil
LOCAL_SRC_FILES:= lib/libavutil-55.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)
 
 include $(CLEAR_VARS)
LOCAL_MODULE:= libavfilter
LOCAL_SRC_FILES:= lib/libavfilter-6.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)
 
 include $(CLEAR_VARS)
LOCAL_MODULE:= libswresample
LOCAL_SRC_FILES:= lib/libswresample-2.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)

h. copy the android/arm/* to the android_ffmepg_MediaPlayer;
   
i. local.properties config the ndk.dir
   sdk.dir=C\:\\Users\\hc\\AppData\\Local\\Android\\sdk
   ndk.dir=C\:\\Users\\hc\\AppData\\Local\\Android\\android-ndk-r10e

j. config the app/src/build.gradle  disable the gradle autobuild ndk.
   apply plugin: 'com.android.application'

android {
    compileSdkVersion 23
    buildToolsVersion "23.0.3"
    sourceSets.main.jni.srcDirs = []

    defaultConfig {
        applicationId "cn.whaley.sh.mediaplayer"
        minSdkVersion 15
        targetSdkVersion 23
        versionCode 1
        versionName "1.0"
        ndk {
            abiFilter "armeabi"
            moduleName "FFMediaPlayer-jni"
            ldLibs "log", "z", "m", "jnigraphics", "android"
        }
    }

    buildTypes {
        release {
            minifyEnabled false
            proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
        }
    }
}

dependencies {
    compile fileTree(dir: 'libs', include: ['*.jar'])
    testCompile 'junit:junit:4.12'
    compile 'com.android.support:appcompat-v7:23.2.1'
}

k.javah 
   > edit the java 
   > javah use autojavah.bat  app/src/main/autojavah.bat
   > edit the c
   > ndk-build use autobuild.bat  app/autobuild.bat
   
l. compile the android_ffmpeg_MediaPlayer project.
