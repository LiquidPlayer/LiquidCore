//
// JSException.java
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

/**
 *  A JSException is thrown for a number of different reasons, mostly by the JavaScriptCore
 *  library.  The description of the exception is given in the message.
 * @since 1.0
 *
 */
public class JSException extends RuntimeException {
    private static final long serialVersionUID = 1L;

    private JSError error;

    /**
     * Creates a Java exception from a thrown JavaScript exception
     * @param error  The JSValue thrown by the JavaScriptCore engine
     * @since 1.0
     */
    public JSException(JSValue error) {
        super(new JSError(error).message());
        this.error = new JSError(error);
    }

    /**
     * Creates a JavaScriptCore exception from a string message
     * @param ctx  The JSContext in which to create the exception
     * @param message  The exception meessage
     * @since 1.0
     */
    public JSException(JSContext ctx, final String message) {
        super((message==null) ? "Error" : message);
        try {
            this.error = new JSError(ctx,(message==null) ? "Error" : message);
        } catch (JSException e) {
            // We are having an Exception Inception. Stop the madness
            this.error = null;
        }
    }
    /**
     * Gets the JSValue of the thrown exception
     * @return  the JSValue of the JavaScriptCore exception
     * @since 1.0
     */
    public JSError getError() {
        return error;
    }

    /**
     * JavaScript error stack trace, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/Stack
     * @return stack trace for error
     * @since 3.0
     */
    public String stack() {
        return (error!=null) ? error.stack() : "undefined";
    }

    /**
     * JavaScript error name, see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/name
     * @return error name
     * @since 3.0
     */
    public String name() {
        return (error!=null) ? error.name() : "JSError";
    }

    @Override
    public String toString() {
        if (error!=null) {
            try {
                return error.toString();
            } catch (JSException e) {
                return "Unknown Error";
            }
        }
        return "Unknown Error";
    }
}
