package com.ess.webcam;

import android.view.Surface;

/**
 * Interface providing surface rendering component control access
 */
public interface IRenderer {

    /**
     * Start rendering
     */
    public void start();

    /**
     * Stop rendering
     */
    public void stop();

    /**
     * Set rendering surface
     * @param surface
     */
    public void setSurface(final Surface surface);
}
