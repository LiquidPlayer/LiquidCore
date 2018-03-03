//
// JSON.java
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
 * A convenience class for creating JavaScript values from JSON
 * @since 0.1.0
 */
public class JSON extends JSValue {
    private JSON(JSContext ctx, final String str) {
        context = ctx;
        valueRef = context.ctxRef().makeFromJSONString(str);
    }

    /**
     * Gets a JSON string representation of any object
     * @param value The JSValue to convert to JSON
     * @return a JSON string representing 'object'
     * @since 0.1.0
     */
    public static String stringify(JSValue value) {
        return value.getContext().property("JSON").toObject()
                .property("stringify").toFunction().call(null,value)
                .toString();
    }

    /**
     * Gets a JSON string representation of any object
     * @param ctx  A js context
     * @param object The object to convert to JSON
     * @return a JSON string representing 'object'
     * @since 0.1.0
     */
    public static String stringify(JSContext ctx, Object object) {
        return ctx.property("JSON").toObject()
                .property("stringify").toFunction().call(null,object)
                .toString();
    }

    /**
     * Creates a new JavaScript value from a JSString JSON string
     * @param ctx  The context in which to create the value
     * @param json  The string containing the JSON
     * @return a JSValue containing the parsed value, or JSValue.isNull() if malformed
     * @since 0.1.0
     */
    public static JSValue parse(JSContext ctx, String json) {
        return new JSON(ctx,json);
    }
}
