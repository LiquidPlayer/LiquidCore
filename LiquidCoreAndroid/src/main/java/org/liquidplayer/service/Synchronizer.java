package org.liquidplayer.service;

/**
 * The Synchronizer is used by a MicroService.ServiceStartListener to synchronize
 * asynchronous init.
 */
public interface Synchronizer {
    /**
     * Called when spawning a new thread that must complete before initialization can
     * continue.
     */
    void enter();

    /**
     * Called after the asynchronous process has completed.
     */
    void exit();
}
