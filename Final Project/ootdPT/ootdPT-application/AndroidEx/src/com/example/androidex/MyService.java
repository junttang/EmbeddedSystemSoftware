package com.example.androidex;

import android.app.Service;
import android.content.Intent;
import android.media.MediaPlayer;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class MyService extends Service {

    private final IBinder binder = (IBinder) new MyBinder();
    private MediaPlayer mp;
    private static final float VOLUME_LEVEL = 0.2f;

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d("test", "Service onCreate");
        mp = MediaPlayer.create(this, R.raw.morning);
        mp.setLooping(false);
        mp.setVolume(VOLUME_LEVEL, VOLUME_LEVEL); // Set the volume level
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d("test", "Service onStartCommand");
        mp.start();
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mp.stop();
        Log.d("test", "Service onDestroy");
    }

    public void startMusic() {
        mp.start();
    }

    public void stopMusic() {
        mp.pause();
    }

    public class MyBinder extends Binder {
        public MyService getService() {
            return MyService.this;
        }
    }
}
