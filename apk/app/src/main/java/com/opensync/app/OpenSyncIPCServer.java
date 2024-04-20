package com.opensync.app;

import android.content.Context;

import org.zeromq.ZMQ;
import org.zeromq.ZContext;
import org.zeromq.ZMQException;

import android.os.Handler;
import android.os.Message;
import android.os.Bundle;
import android.util.Log;

public class OpenSyncIPCServer implements Runnable {
    private static final String TAG = "OpenSync IPCServer";
    private final Context context;
    OpenSyncAPI openSyncAPI;

    public OpenSyncIPCServer(Context context) {
        this.context = context;
        openSyncAPI = new OpenSyncAPI(context);
    }

    @Override
    public void run() {
        ZMQ.Context zmqContext = ZMQ.context(1);
        ZMQ.Socket socket = zmqContext.socket(ZMQ.ROUTER);
        socket.bind("tcp://127.0.0.1:10086");

        try {
            Log.i(TAG, "Start...");
            while (!Thread.currentThread().isInterrupted()) {
                String identity = socket.recvStr();
                socket.sendMore(identity);
                socket.recvStr();  // envelope delimiter
                String msg = socket.recvStr();
                socket.sendMore("");

                Log.i(TAG, identity + " " + msg);
                socket.send(openSyncAPI.Dispatch(msg), 0);
            }
        } catch (ZMQException e) {
            Log.e(TAG, "ZMQ Exception occurred with error code: " + e.getErrorCode(), e);
        } finally {
            socket.close();
            zmqContext.term();
        }
    }
}
