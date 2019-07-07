package com.ess.webcam;

/**
 * Interface providing opencv processing component control access
 */
public interface IOpencv {

    /**
     * Return frame access interface for client usage
     *
     * @param interface_index
     * @return
     */
    public long getFrameAccessIfc(int interface_index);

    /**
     * Start the opencv component
     */
    public void start();

    /**
     * Stop the opencv component
     */
    public void stop();

    /**
     * Cycles through opencv processes
     */
    public void cycleProcessingMode();

}
