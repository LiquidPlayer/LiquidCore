/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;

/**
 * A convenience class for managing JavaScript error objects
 * @since 0.1.0
 */
public class JSError extends JSObject {
    /**
     * Generates a JavaScript throwable exception object
     * @param ctx  The context in which to create the error
     * @param message  The description of the error
     * @since 0.1.0
     */
    public JSError(JSContext ctx, @NonNull final String message) {
        context = ctx;
        valueRef = context.ctxRef().makeError(message);
        addJSExports();
        context.persistObject(this);
    }
    /**
     * Generates a JavaScript throwable exception object
     * @param ctx  The context in which to create the error
     * @since 0.1.0
     */
    public JSError(JSContext ctx) {
        this(ctx, "Error");
    }

    /**
     * Constructs a JSError from a JSValue.  Assumes JSValue is a properly constructed JS Error
     * object.
     * @since 0.1.0
     * @param error the JavaScript Error object
     */
    /*package*/ JSError(JSValue error) {
        super(
            error.isObject() ? (JNIJSObject)error.toObject().valueRef() :
                    (JNIJSObject)new JSError(error.getContext()).valueRef(),
            error.getContext()
        );
    }

    /**
     * JavaScript error stack trace, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/Stack
     * @return stack trace for error
     * @since 0.1.0
     */
    public String stack() {
        return property("stack").toString();
    }

    /**
     * JavaScript error message, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/message
     * @return error message
     * @since 0.1.0
     */
    public String message() {
        return property("message").toString();
    }

    /**
     * JavaScript error name, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/name
     * @return error name
     * @since 0.1.0
     */
    public String name() {
        return property("name").toString();
    }
}
