package com.ess.utils;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

public class ThreadedHandler extends Handler {

	private static final String TAG = ThreadedHandler.class.getSimpleName();

	/* default handler */
	public static final ThreadedHandler createHandler() {
		return createHandler(TAG);
	}

    /* handler callback associated with posted messages and runnables */
    public static final ThreadedHandler createHandler(final Callback callback) {
        return createHandler(TAG, callback);
    }

	private static final ThreadedHandler createHandler(final String name) {
        Looper looper = createLooper(name);
		return new ThreadedHandler(looper);
	}

	private static final ThreadedHandler createHandler(final String name, final Callback callback) {
        Looper looper = createLooper(name);
		return new ThreadedHandler(looper, callback);
	}

    private static final Looper createLooper(final String name) {
        final HandlerThread handlerThread = new HandlerThread(name);
        handlerThread.start();
        return handlerThread.getLooper();
    }

	private ThreadedHandler(final Looper looper) {
		super(looper);
	}

	private ThreadedHandler(final Looper looper, final Callback callback) {
		super(looper, callback);
	}

}
