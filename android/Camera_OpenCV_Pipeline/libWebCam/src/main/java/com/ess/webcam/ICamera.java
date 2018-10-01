package com.ess.webcam;

/**
 * Interface providing camera component control access
 */
public interface ICamera {

    /**
     * Return frame access interface for client usage
     *
     * @param interface_index
     * @return
     */
    public long getFrameAccessIfc(int interface_index);

    /**
     * Start the camera stream
     */
    public void start();

    /**
     * Stop the camera stream
     */
    public void stop();

    public String prepare(final USBMonitor.UsbControlBlock ctrlBlock);

    /**
     * Register json status callback
     *
     * @param callback
     */
    public void setStatusCallback(final IStatusCallback callback);

    /**
     * Queries camera for supported mode
     *
     * @return supported video modes String
     */
    public String getSupportedVideoModes();

//    /**
//     * Select camera capture mode
//     *
//     * @param width
//     * @param height
//     * @param frameFormat
//     */
//    public void setCaptureMode(final int width, final int height, final int frameFormat);

    /**
     * Select camera capture mode
     *
     * @param width
     * @param height
     * @param min_fps
     * @param max_fps
     * @param frameFormat
     */
    public void setCaptureMode(final int width, final int height, final int min_fps, final int max_fps, final int frameFormat);
}