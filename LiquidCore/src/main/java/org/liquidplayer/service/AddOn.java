package org.liquidplayer.service;

import org.liquidplayer.javascript.JSValue;

/**
 * Supported addons must expose a public {@code Object} that implements this interface.  The
 * object must expose a public constructor that takes {@code android.content.Context} as
 * its lone parameter.
 * <br><br>
 * The package and naming of this class is extremely important.  The class *must* be in
 * the {@code org.liquidplayer.addon} package, and it must be named the same as the native
 * module that will be requested by JavaScript, with the following exceptions:
 * <br><br>
 * 1. The first character must be capitalized<br>
 * 2. Any non-alphanumeric or underscore ('_') characters must be ommitted and the *next*
 *    character capitalized.
 * <br><br>
 * For example, if node attempts to require "my-native-moDULE.node", LiquidCore will look for
 * the class {@code org.liquidplayer.addon.MyNativeModULE} to resolve the addon.
 */
public interface AddOn {
    /**
     * When {@code require('<MODULE_NAME>.node')} is called from JavaScript, this method will
     * be triggered if {@code <MODULE_NAME>} resolves to this class as described above.  This method
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
