package cn.whaley.sh.mediaplayer.FFMediaPlayer;

import android.os.Parcel;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.IOException;
import java.lang.ref.WeakReference;

/**
 * Created by hc on 2016/4/10.
 */
public class FFMediaPlayer {
    private final static String TAG = "FFMediaPlayer";
    private final static String DEBUG_IN = "IN ";
    private final static String DEBUG_OUT = "OUT ";
    private static final int MEDIA_NOP = 0; // interface test message
    private static final int MEDIA_PREPARED = 1;
    private static final int MEDIA_PLAYBACK_COMPLETE = 2;
    private static final int MEDIA_BUFFERING_UPDATE = 3;
    private static final int MEDIA_SEEK_COMPLETE = 4;
    private static final int MEDIA_SET_VIDEO_SIZE = 5;
    private static final int MEDIA_ERROR = 100;
    private static final int MEDIA_INFO = 200;

    /** Unspecified media player error.
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_UNKNOWN = 1;

    /** Media server died. In this case, the application must release the
     * MediaPlayer object and instantiate a new one.
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_SERVER_DIED = 100;

    /** The video is streamed and its container is not valid for progressive
     * playback i.e the video's index (e.g moov atom) is not at the start of the
     * file.
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200;


    /** Unspecified media player info.
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_UNKNOWN = 1;

    /** The video is too complex for the decoder: it can't decode frames fast
     *  enough. Possibly only the audio plays fine at this stage.
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_VIDEO_TRACK_LAGGING = 700;

    /** Bad interleaving means that a media has been improperly interleaved or
     * not interleaved at all, e.g has all the video samples first then all the
     * audio ones. Video is playing but a lot of disk seeks may be happening.
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_BAD_INTERLEAVING = 800;

    /** The media cannot be seeked (e.g live stream)
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_NOT_SEEKABLE = 801;

    /** A new set of metadata is available.
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_METADATA_UPDATE = 802;

    public static final int MEDIA_INFO_FRAMERATE_VIDEO = 900;
    public static final int MEDIA_INFO_FRAMERATE_AUDIO = 901;

    private int                         mNativeContext;

    public FFMediaPlayer(){
        Log.d(TAG, DEBUG_IN + "FFMediaPlayer");
        native_setup(new WeakReference<FFMediaPlayer>(this));
        Log.d(TAG, DEBUG_OUT + "FFMediaPlayer");
    }

    public void setDataSource(String path) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        native_setDataSource(path);
    }

    public void setDisplay(SurfaceHolder sh) {
        Surface surface;
        if (sh != null) {
            surface = sh.getSurface();
        } else {
            surface = null;
        }
        native_setVideoSurface(surface);
    }

    public void prepare() throws IOException, IllegalStateException {
        native_prepare();
    }

    public void prepareAsync() throws IOException, IllegalStateException {
        native_prepareAsync();
    }

    public void start() throws IllegalStateException {
        native_start();
    }

    public void stop() throws IllegalStateException {
        native_stop();
    }

    public void pause() throws IllegalStateException {
        native_pause();
    }

    public void seekTo(int msec) throws IllegalStateException {
        native_seekTo(msec);
    }

    public void release() {
        native_release();
    }

    public void reset() {
        native_reset();
    }

    public void invoke(Parcel request, Parcel reply) {
        int retcode = native_invoke(request, reply);
        reply.setDataPosition(0);
        if (retcode != 0) {
            throw new RuntimeException("failure code: " + retcode);
        }
    }

    public void isPlaying() {
        native_isPlaying();
    }

    public int getVideoWidth() {
        return native_getVideoWidth();
    }

    public int getVideoHeight() {
        return native_getVideoHeight();
    }

    public boolean setParameter(int key, String value) {
        Parcel p = Parcel.obtain();
        p.writeString(value);
        boolean ret = native_setParameter(key, p);
        p.recycle();
        return ret;
    }

    public boolean setParameter(int key, int value) {
        Parcel p = Parcel.obtain();
        p.writeInt(value);
        boolean ret = native_setParameter(key, p);
        p.recycle();
        return ret;
    }

    protected void finalize() {
        native_finalize();
    }

    /**
     * Called from native code when an interesting event happens.  This method
     * just uses the EventHandler system to post the event back to the main app thread.
     * We use a weak reference to the original MediaPlayer object so that the native
     * code is safe from the object disappearing from underneath it.  (This is
     * the cookie passed to native_setup().)
     */
    private static void postEventFromNative(Object mediaplayer_ref,
                                            int what, int arg1, int arg2, Object obj)
    {
        switch(what) {
            case MEDIA_INFO_FRAMERATE_VIDEO:
                Log.d(TAG, "Video fps:" + arg1);
                break;
            case MEDIA_INFO_FRAMERATE_AUDIO:
                Log.d(TAG, "Audio fps:" + arg1);
                break;
        }
    }

    public native String native_getVersion();

    private native void native_setDataSource(String path) throws IOException, IllegalArgumentException, IllegalStateException;

    private native void native_prepare() throws IOException, IllegalStateException;

    private native void native_prepareAsync() throws IllegalStateException;

    private native void native_start() throws IllegalStateException;

    private native void native_stop() throws IllegalStateException;

    private native void native_pause() throws IllegalStateException;

    public native void native_seekTo(int msec) throws IllegalStateException;

    private native void native_release();

    private native void native_reset();

    private native boolean native_isPlaying();

    private native int native_getVideoWidth();

    private native int native_getVideoHeight();

    private native final int native_invoke(Parcel request, Parcel reply);

    private native boolean native_setParameter(int key, Parcel value);

    private native void native_setVideoSurface(Surface surface);

    static {
        System.loadLibrary("avutil-55");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("swscale-4");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("swresample-2");
        System.loadLibrary("FFMediaPlayer-jni");
        native_init();
    }

    private static native final void native_init();

    private native final void native_setup(Object self);

    private native final void native_finalize();
}
