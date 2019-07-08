package com.ess.webcam;

import android.text.TextUtils;
import android.util.Log;

import com.ess.util.UsbMonitor;
import com.ess.util.UsbMonitor.UsbConnectionData;

/**
 * camera library provides access to native level libuvc library
 */
public class Camera implements ICamera {

    private static final boolean DEBUG = true;
    private static final String TAG = Camera.class.getSimpleName();

    private static final String DEFAULT_USBFS = "/dev/bus/usb";

    /**
     * unknown frame format
     */
    public static final int CAMERA_FRAME_FORMAT_UNKNOWN = 0;
    /**
     * yuyv uvc frame format
     */
    public static final int CAMERA_FRAME_FORMAT_YUYV = 1;
    /**
     * mjpeg frame format
     */
    public static final int CAMERA_FRAME_FORMAT_MJPEG = 2;
    /**
     * 32-bit RGBA
     */
    public static final int CAMERA_FRAME_FORMAT_RGBX = 3;
    /**
     * 8 bit gray frame format
     */
    public static final int CAMERA_FRAME_FORMAT_GRAY_8 = 4;
    /**
     * 16 bit gray frame format
     */
    public static final int CAMERA_FRAME_FORMAT_GRAY_16 = 5;

    private UsbConnectionData connectionData;

    /**
     * current camera frame format
     */
    protected int currentFrameFormat = CAMERA_FRAME_FORMAT_UNKNOWN;
    /**
     * current camera width
     */
    protected int currentWidth = 0;
    /**
     * current camera height
     */
    protected int currentHeight = 0;

    protected String supportedVideoModes;

    /**
     * Handle to native camera object.
     */
    protected long nativePtr;

    /**
     * camera constructor
     */
    public Camera() {
        // create native render engine
        nativePtr = nativeCreate();
        supportedVideoModes = null;
    }

    protected void finalize() {
        if (DEBUG) Log.e(TAG, "finalize() start");

        stop();

        if (DEBUG) Log.e(TAG, "finalize() end");
    }

    /**
     * Prepare the camera libuvc stack
     * USB permission is necessary before this method is called
     *
     * @param connectionData
     */
    public synchronized String prepare(UsbMonitor.UsbConnectionData connectionData) {

        int result;
        try {
            this.connectionData = connectionData;

            result = nativePrepare(nativePtr,
                    connectionData.getVenderId(), connectionData.getProductId(),
                    connectionData.getFileDescriptor(),
                    connectionData.getBusNum(),
                    connectionData.getDevNum(),
                    getUSBFSName(connectionData));
        } catch (final Exception e) {
            Log.w(TAG, e);
            result = -1;
        }
        if (result != 0) {
            throw new UnsupportedOperationException("open failed:result=" + result);
        }
        if (nativePtr != 0 && TextUtils.isEmpty(supportedVideoModes)) {
            supportedVideoModes = nativeGetSupportedVideoModes(nativePtr);
        }

        return supportedVideoModes;
    }

    public void setStatusCallback(final IStatusCallback callback) {

        if (nativePtr != 0) {
            nativeSetStatusCallback(nativePtr, callback);
        }
    }

    public synchronized String getSupportedVideoModes() {
        return !TextUtils.isEmpty(supportedVideoModes) ? supportedVideoModes : (supportedVideoModes = nativeGetSupportedVideoModes(nativePtr));
    }

    // TODO auto-negotiate frame interval ?
//    public void setCaptureMode(final int width, final int height, final int frameFormat) {
//
//        if (usbConnectionData != null) {
//            setCaptureMode(width, height, DEFAULT_UVC_PREVIEW_MIN_FPS, DEFAULT_UVC_PREVIEW_MAX_FPS, frameFormat);
//        } else {
//            throw new IllegalArgumentException("invalid control block");
//        }
//    }

    public void setCaptureMode(final int width, final int height, final int min_fps, final int max_fps, final int frameFormat) {

        if ((width == 0) || (height == 0))
            throw new IllegalArgumentException("invalid preview size");

        if (nativePtr != 0) {
            final int result = nativeSetCaptureMode(nativePtr, width, height, min_fps, max_fps, frameFormat);
            if (result != 0)
                throw new IllegalArgumentException("Failed to set preview size");
            currentFrameFormat = frameFormat;
            currentWidth = width;
            currentHeight = height;
        }
    }

    public synchronized long getFrameAccessIfc(int interface_index) {
        return nativeGetFrameAccessIfc(nativePtr, interface_index);
    }

    public synchronized void start() {
        if (connectionData != null) {
            nativeStart(nativePtr);
        }
    }

    public synchronized void stop() {

        if (connectionData != null) {
            nativeStop(nativePtr);
        }

        if (connectionData != null) {
            connectionData.close();
            connectionData = null;
        }

        currentFrameFormat = -1;
        supportedVideoModes = null;
    }

    private final String getUSBFSName(final UsbConnectionData connectionData) {

        String result = null;
        final String name = connectionData.getDeviceName();

        final String[] v = !TextUtils.isEmpty(name) ? name.split("/") : null;
        if ((v != null) && (v.length > 2)) {
            final StringBuilder sb = new StringBuilder(v[0]);
            for (int i = 1; i < v.length - 2; i++)
                sb.append("/").append(v[i]);
            result = sb.toString();
        }
        if (TextUtils.isEmpty(result)) {
            Log.w(TAG, "failed to get USBFS path, try to use default path:" + name);
            result = DEFAULT_USBFS;
        }
        return result;
    }

    static {
        System.loadLibrary("opencv_java4");
        System.loadLibrary("native-pipeline-lib");
    }

    private final native long nativeCreate();
    private final native int nativePrepare(long id_camera, int venderId, int productId, int fileDescriptor, int busNum, int devAddr, String usbfs);
    private static final native int nativeSetStatusCallback(final long nativePtr, final IStatusCallback callback);
    private static final native String nativeGetSupportedVideoModes(final long id_camera);
    private static final native int nativeSetCaptureMode(final long id_camera, final int width, final int height, final int min_fps, final int max_fps, final int mode);
    private static final native long nativeGetFrameAccessIfc(final long id_camera, int interface_index);
    private static final native int nativeStart(final long id_camera);
    private static final native int nativeStop(final long id_camera);
}
