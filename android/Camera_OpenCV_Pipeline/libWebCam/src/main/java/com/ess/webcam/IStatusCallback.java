package com.ess.webcam;

/**
 * Interface providing JSON status callback from native components
 */
public interface IStatusCallback {
    /**
     * JSON status callback
     *
     * @param statusSchema the json schema associated with jsonString
     * @param jsonString json status string
     */
    void onStatus(int statusSchema, String jsonString);
}
