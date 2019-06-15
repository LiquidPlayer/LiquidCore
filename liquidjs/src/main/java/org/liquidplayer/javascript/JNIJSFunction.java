/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

class JNIJSFunction {
    static JNIJSObject fromRef(long valueRef)
    {
        return (JNIJSObject) JNIJSValue.fromRef(valueRef);
    }

    static native long makeFunctionWithCallback(JSFunction thiz, long ctxRef, String name);
    static native void setException(long funcRef, long valueRef);
}
