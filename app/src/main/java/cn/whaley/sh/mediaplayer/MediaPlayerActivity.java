package cn.whaley.sh.mediaplayer;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;

import cn.whaley.sh.mediaplayer.FFMediaPlayer.FFMediaPlayerJNI;

public class MediaPlayerActivity extends AppCompatActivity {

    private final String TAG = "MediaPlayerActivity";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_media_player);
        FFMediaPlayerJNI FFPlayer = new FFMediaPlayerJNI();
        Log.d(TAG, FFPlayer.getStringFromNative());
    }
}
