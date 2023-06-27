package com.example.androidex;

import com.example.androidex.MainActivity.Args;

import android.util.Log;

public class TurnOnOffThread extends Thread {
    private boolean isMusicOn = true;
    private boolean isRunning = true;

    private MyService myService;
    
    public native static int openDevice();
    public native static void writeDevice(int fd);
    public native static void closeDevice(int fd);
    
    private int fd;

    static {
        System.loadLibrary("intr_driver");
    }

    public TurnOnOffThread(MyService service) {
        myService = service;
        myService.startMusic();
        fd = openDevice();
    }

    @Override
    public void run() {
        while (isRunning) {
        	writeDevice(fd);  // It will sleep here!
            toggleMusic();    // After the thread woke up
        }
    }

    public void stopThread() {
        isRunning = false;
        closeDevice(fd);
    }

    private void toggleMusic() {
        isMusicOn = !isMusicOn;
        // Control the music playback
        if (isMusicOn) {
            myService.startMusic();
            Log.d("TurnOnOffThread", "Music turned on");
        } else {
            myService.stopMusic();
            Log.d("TurnOnOffThread", "Music turned off");
        }
    }
}
