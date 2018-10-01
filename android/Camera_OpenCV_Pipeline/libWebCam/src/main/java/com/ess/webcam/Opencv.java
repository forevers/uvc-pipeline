package com.ess.webcam;

public class Opencv implements IOpencv {

    protected long frameAccessHandle;
    protected long opencvHandle;

    public Opencv(long frameAccessHandle, int height, int width, int shift) {
        this.frameAccessHandle = frameAccessHandle;
        opencvHandle = nativeCreate(frameAccessHandle, height, width, shift);
    }

    //    private static final String TAG = Camera.class.getSimpleName();
    private static final String TAG = "test";

    public synchronized long getFrameAccessIfc(int interface_index) {
        return nativeGetFrameAccessIfc(opencvHandle, interface_index);
    }

    public synchronized void start() {
        nativeStart(opencvHandle);
    }

    public synchronized void stop() {
        nativeStop(opencvHandle);
    }

    public synchronized void cycleProcessingMode() {
        nativeCycleProcessingMode(opencvHandle);
    }

    public synchronized int scaleChannelIfGray16(boolean downscale) {
        return nativeScaleIfGray16(opencvHandle, downscale);
    }

    static {
        System.loadLibrary("opencv_java3");
        System.loadLibrary("native-pipeline-lib");
    }

    private final native long nativeCreate(long rame_access_handle, int height, int width, int shift);
    private static final native long nativeGetFrameAccessIfc(final long id_open_cv, int interface_index);
    private static final native int nativeStart(final long id_opencv);
    private static final native int nativeStop(final long id_opencv);
    private static final native int nativeCycleProcessingMode(final long id_opencv);
    private static native int nativeScaleIfGray16(final long id_opencv, boolean downscale);
}
