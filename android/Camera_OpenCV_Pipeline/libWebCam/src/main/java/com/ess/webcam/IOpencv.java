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
     * primary view touched
     * @param percent_width touch y location %
     * @param percent_height touch x location %
     */
    public void touchView(float percent_width, float percent_height);

    /**
     * Cycles through opencv processes
     */
    public void cycleProcessingMode();

}
