package org.liquidplayer.jscshim;

import org.liquidplayer.javascript.JSContext;

/**
 * Provides static initialization for projects that require the JavaScriptCore -> V8 bridge
 */
@SuppressWarnings("JniMissingFunction")
public class JSCShim {
    static {
        JSContext.dummy();
    }
    private static boolean initialized = false;

    /**
     * To use the JavaScriptCore header files, you must compile JSCShim.cpp into your project
     * and call this function to connect to the LiquidCore JSC -> v8 bridge.
     */
    public static void staticInit() {
        if (!initialized) {
            setJSCShimToken(getJSCShimToken());
            initialized = true;
        }
    }

    private static native long getJSCShimToken();
    private static native void setJSCShimToken(long token);
}
