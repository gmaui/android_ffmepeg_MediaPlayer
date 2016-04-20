package cn.whaley.sh.mediaplayer;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.IOException;

import cn.whaley.sh.mediaplayer.FFMediaPlayer.FFMediaPlayer;
import cn.whaley.sh.mediaplayer.ui.MessageBox;

public class MediaPlayerActivity extends Activity implements SurfaceHolder.Callback{

    private final String TAG = "MediaPlayerActivity";

    private FFMediaPlayer FFPlayer = new FFMediaPlayer();
    private SurfaceView surfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_media_player);

        surfaceView = (SurfaceView) findViewById(R.id.surface_view);
        surfaceView.getHolder().addCallback(this);
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
        try {
            FFPlayer.prepare();
            FFPlayer.start();
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Can't set video: " + e.getMessage());
            MessageBox.show(this, e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "Can't set video: " + e.getMessage());
            MessageBox.show(this, e);
        } catch (IOException e) {
            Log.e(TAG, "Can't set video: " + e.getMessage());
            e.printStackTrace();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        try {
            if(FFPlayer != null)
                FFPlayer.stop();
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "stop: " + e.getMessage());
            MessageBox.show(this, e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "stop: " + e.getMessage());
            MessageBox.show(this, e);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        try {
            if(FFPlayer != null)
                FFPlayer.setDisplay(surfaceView.getHolder());
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "setSurface: " + e.getMessage());
            MessageBox.show(this, e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "setSurface: " + e.getMessage());
            MessageBox.show(this, e);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
