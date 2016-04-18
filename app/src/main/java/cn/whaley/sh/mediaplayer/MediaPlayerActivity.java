package cn.whaley.sh.mediaplayer;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import java.io.IOException;

import cn.whaley.sh.mediaplayer.FFMediaPlayer.FFMediaPlayer;
import cn.whaley.sh.mediaplayer.ui.MessageBox;

public class MediaPlayerActivity extends Activity {

    private final String TAG = "MediaPlayerActivity";

    FFMediaPlayer FFPlayer = new FFMediaPlayer();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_media_player);

        Intent i = getIntent();
        String filePath = i.getStringExtra(getResources().getString(R.string.input_file));
        if(filePath == null) {
            Log.d(TAG, "Not specified video file");
            finish();
        } else {
            try {
                Log.d(TAG,"Input File:"+filePath);

                try {
                    FFPlayer.native_getVersion();
                    FFPlayer.setDataSource(filePath);
                    FFPlayer.prepare();
                    FFPlayer.start();
                } catch (IllegalArgumentException e) {
                    Log.e(TAG, "Can't set video: " + e.getMessage());
                    MessageBox.show(this, e);
                } catch (IllegalStateException e) {
                    Log.e(TAG, "Can't set video: " + e.getMessage());
                    MessageBox.show(this, e);
                }
                //setContentView(mMovieView);
            } catch (Exception e) {
                Log.d(TAG, "Error when inicializing ffmpeg: " + e.getMessage());
                MessageBox.show(this, e);
                finish();
            }
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
//        try {
//            FFPlayer.pause();
//        } catch (IllegalArgumentException e) {
//            Log.e(TAG, "Can't set video: " + e.getMessage());
//            MessageBox.show(this, e);
//        } catch (IllegalStateException e) {
//            Log.e(TAG, "Can't set video: " + e.getMessage());
//            MessageBox.show(this, e);
//        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onStop() {
        super.onStop();
        try {
            FFPlayer.stop();
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Can't set video: " + e.getMessage());
            MessageBox.show(this, e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "Can't set video: " + e.getMessage());
            MessageBox.show(this, e);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}
