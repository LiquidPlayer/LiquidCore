package org.liquidplayer.service;

import org.liquidplayer.javascript.JSValue;

/**
 * Supported addons must expose a public {@code Object} that implements this interface.  The
 * object must have a default constructor.
 */
public interface AddOn {
    /**
     * When {@code require('<MODULE_NAME>.node')} is called from JavaScript, this method will
     * be triggered if {@code <MODULE_NAME>} is returned in {@code module()}.  This method
     * must then (1) load any required native libraries and (2) execute the
     * {@code __register_<MODULE_NAME>()} static function created from {@code NODE_MODULE_*} macros
     * in {@code node.h}.
     *
     * As a best practice, do not call {@code System.loadLibrary()} statically, but instead call
     * it lazily when this method is triggered.
     *
     * @param module The name of the module to register.
     */
    void register(String module);

    /**
     * Each time {@code require()} is called for this module from JS, this method will be triggered
     * before returning the bound object to the caller.  This is an opportunity to attach anything
     * else to the object that may be required from Java before returning it to the caller.
     *
     * @param binding The native binding object.
     * @param service The {@code #org.liquidplayer.service.MicroService} of this process
     */
    void require(JSValue binding, MicroService service);
}
