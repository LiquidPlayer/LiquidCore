/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.jscshim;

import org.liquidplayer.javascript.JSContext;

import java.lang.reflect.Method;

/**
 * Provides static initialization for projects that require the JavaScriptCore -> V8 bridge
 */
public class JSCShim {
    /* Ensure the shared libraries get loaded first */
    static {
        try {
            Method init = JSContext.class.getDeclaredMethod("init");
            init.setAccessible(true);
            init.invoke(null);
        } catch (Exception e) {
            e.printStackTrace();
        }
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

    @SuppressWarnings("JniMissingFunction")
    private static native void setJSCShimToken(long token);
}
