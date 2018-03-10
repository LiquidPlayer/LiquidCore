//
// JSError.java
//
// AndroidJSCore project
// https://github.com/ericwlange/AndroidJSCore/
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2016 Eric Lange. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;

/**
 * A convenience class for managing JavaScript error objects
 * @since 0.1.0
 */
@SuppressWarnings("WeakerAccess,SameParameterValue")
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
    public JSError(JSValue error) {
        super((JNIJSObject)error.toObject().valueRef(), error.getContext());
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
