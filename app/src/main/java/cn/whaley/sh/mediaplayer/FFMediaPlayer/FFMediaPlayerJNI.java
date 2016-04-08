package cn.whaley.sh.mediaplayer.FFMediaPlayer;

/**
 * Created by hc on 2016/4/7.
 */
public class FFMediaPlayerJNI {

    private final String TAG = "FFMediaPlayerJNI";

    public native String getStringFromNative();

    static {
        System.loadLibrary("avutil-55");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("swscale-4");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("swresample-2");
        System.loadLibrary("FFMediaPlayer-jni");
    }
}
