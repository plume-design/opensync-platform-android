package com.opensync.app;

import android.util.Log;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.app.ActivityManager;
import android.content.Intent;
import android.content.ComponentName;
import android.content.Context;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.media.MediaMetadata;
import android.text.TextUtils;

import java.util.Timer;
import java.util.TimerTask;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Date;

import org.greenrobot.eventbus.EventBus;

public class OpenSyncStreamingEvent extends MediaController.Callback {
    private final MediaSessionManager mMediaSessionManager;
    private static Context mContext;
    private List<MediaController> sessions = new ArrayList<>();
    private Timer timer;
    private HandlerThread handlerThread;
    private Handler handler;

    private PlaybackState playbackState;

    private long mesureStartupTs = 0;
    public static long mStartupTime = 0;
    private boolean doesMeasureStartup = false;
    private long playbackPosition = 0;

    private String TAG = "OpenSyncStreamingEvent";

    OpenSyncStreamingEvent(Context context) {
        mContext = context;

        mMediaSessionManager = (MediaSessionManager) mContext.getSystemService(
                Context.MEDIA_SESSION_SERVICE);

        Log.i(TAG, "Init");
        initTimer();
    }

    private void initTimer() {
        handlerThread = new HandlerThread("OpenSyncStreamingEventThread");
        handlerThread.start();
        handler = new Handler(handlerThread.getLooper());

        timer = new Timer();
        TimerTask task = new TimerTask() {
            @Override
            public void run() {
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        updateMediaControllerCallback();
                        if (doesMeasureStartup) {
                            if (getPlaybackPosition() > playbackPosition) {
                                doesMeasureStartup = false;
                                mStartupTime = new Date().getTime() - mesureStartupTs;
                                Log.i(TAG, "Streaming startup time measured: " + mStartupTime);
                                EventBus.getDefault().post(playbackState);
                            }
                        }
                    }
                });
            }
        };

        timer.schedule(task, 0, 1000);
    }

    public void stopTimer() {
        if (timer != null) {
            timer.cancel();
            timer = null;
        }

        if (handlerThread != null) {
            handlerThread.quit();
            handlerThread = null;
        }
    }

    private int getPlaybackPosition() {
        for (MediaController aController : sessions) {
            PlaybackState  playbackState = aController.getPlaybackState();
            return (int) playbackState.getPosition();
        }
        return 0;
    }

    public void updateMediaControllerCallback() {
        if (mMediaSessionManager != null) {
            List<MediaController> newSessions =
                mMediaSessionManager.getActiveSessions
                (new ComponentName(mContext, OpenSyncStreamingEvent.class));

            Iterator<MediaController> iterator = sessions.iterator();
            while (iterator.hasNext()) {
                MediaController existingController = iterator.next();
                if (!containsController(newSessions, existingController)) {
                    existingController.unregisterCallback(mMediaListener);
                    Log.d(TAG, "Media Session Stop: " + existingController.getPackageName());
                    iterator.remove();
                }
            }

            for (MediaController aController : newSessions) {
                if (!containsController(sessions, aController)) {
                    Log.d(TAG, "New Media Session: " + aController.getPackageName());
                    aController.registerCallback(mMediaListener);
                    sessions.add(aController);
                }
            }
        }
    }

    private boolean containsController(List<MediaController> controllers, MediaController controller) {
        for (MediaController c : controllers) {
            if (TextUtils.equals(c.getPackageName(), controller.getPackageName())) {
                return true;
            }
        }
        return false;
    }

    private void unregisterCallbacks() {
        for (MediaController aController : sessions) {
            aController.unregisterCallback(mMediaListener);
        }
        sessions.clear();
    }

    private final MediaController.Callback mMediaListener = new MediaController.Callback() {
        private int lastState = PlaybackState.STATE_NONE;
        private int curState = PlaybackState.STATE_NONE;

        @Override
        public void onPlaybackStateChanged(PlaybackState state) {
            playbackState = state;
            curState = playbackState.getState();

            if (curState != lastState) {
                Log.i(TAG, "Playback StateChanged: "
                        + getStateName(lastState)
                        + " -> "
                        +getStateName(curState));
                EventBus.getDefault().post(state);

                /* Start */
                if ((lastState == PlaybackState.STATE_NONE   || lastState == PlaybackState.STATE_STOPPED) &&
                    (curState == PlaybackState.STATE_PLAYING || curState == PlaybackState.STATE_BUFFERING)) {
                    doesMeasureStartup = true;
                    mesureStartupTs = new Date().getTime();
                    playbackPosition = getPlaybackPosition();
                    mStartupTime = 0;
                    Log.i(TAG, "Streaming start measure startup time");
                }
                /* Stop */
                if (lastState == PlaybackState.STATE_PLAYING &&
                    (curState == PlaybackState.STATE_NONE || curState == PlaybackState.STATE_STOPPED)) {
                    doesMeasureStartup = false;
                }
            }

            lastState = curState;
        }

        private String getStateName(int state) {
            switch (state) {
                case PlaybackState.STATE_NONE:
                    return "STATE_NONE";
                case PlaybackState.STATE_STOPPED:
                    return "STATE_STOPPED";
                case PlaybackState.STATE_PAUSED:
                    return "STATE_PAUSED";
                case PlaybackState.STATE_PLAYING:
                    return "STATE_PLAYING";
                case PlaybackState.STATE_FAST_FORWARDING:
                    return "STATE_FAST_FORWARDING";
                case PlaybackState.STATE_REWINDING:
                    return "STATE_REWINDING";
                case PlaybackState.STATE_BUFFERING:
                    return "STATE_BUFFERING";
                case PlaybackState.STATE_ERROR:
                    return "STATE_ERROR";
                default:
                    return "UNKNOWN_STATE(" + state + ")";
            }
        }
    };
}
