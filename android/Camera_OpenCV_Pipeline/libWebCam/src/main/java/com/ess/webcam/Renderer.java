package com.ess.webcam;

import android.view.Surface;

public class Renderer implements IRenderer {

    protected long frameAccessHandle;
    protected long rendererHandle;

    public Renderer(long frameAccessHandle, int height, int width) {

        this.frameAccessHandle = frameAccessHandle;
        rendererHandle = nativeCreate(frameAccessHandle, height, width);
    }

    public void setSurface(final Surface surface) {
        nativeSetSurface(rendererHandle, surface);
    }

    public synchronized void start() {
        nativeStart(rendererHandle);
    }

    public synchronized void stop() {
        nativeStop(rendererHandle);
    }

    static {
        System.loadLibrary("opencv_java3");
        System.loadLibrary("native-pipeline-lib");
    }

    private final native long nativeCreate(long frame_access_handle, int height, int width);
    private static final native int nativeStart(final long id_renderer);
    private static final native int nativeStop(final long id_renderer);
    private static final native int nativeSetSurface(final long id_renderer_demux, final Surface surface);
}
