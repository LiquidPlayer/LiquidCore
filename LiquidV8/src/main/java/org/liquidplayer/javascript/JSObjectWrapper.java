/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;


abstract class JSObjectWrapper extends JSObject {
    JSObjectWrapper(JSObject obj) {
        mJSObject = obj;
        context = obj.getContext();
        valueRef = obj.valueRef();
    }
    private final JSObject mJSObject;

    /**
     * Gets underlying JSObject
     * @return JSObject representing the wrapped object
     * @since 0.1.0
     */
    public JSObject getJSObject() {
        return mJSObject;
    }
}
