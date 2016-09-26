//
// JSRegExp.java
// AndroidJSCore project
//
// https://github.com/ericwlange/AndroidJSCore/
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
package org.liquidplayer.v8;

/**
 *  A convenience class for managing JavaScript regular expressions.  See
 *  <a href="https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Regular_Expressions">
 *  https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Regular_Expressions</a>
 *  for details on JavaScript regexp.
 * @since 1.0
 */
public class JSRegExp extends JSObject {
    /**
     * Creates a new JavaScript regular expression
     * @param ctx  The context in which to create the regular expression
     * @param pattern  The REGEXP pattern
     * @param flags  The REGEXP flags
     * @since 1.0
     */
    public JSRegExp(JSContext ctx, final String pattern, final String flags) {
        context = ctx;
        context.sync(new Runnable() {
            @Override
            public void run() {
                valueRef = makeRegExp(context.ctxRef(), pattern, flags).reference;
                addJSExports();
            }
        });
    }
    /**
     * Creates a new JavaScript regular expression
     * @param ctx  The context in which to create the regular expression
     * @param pattern  The REGEXP pattern
     * @since 1.0
     */
    public JSRegExp(JSContext ctx, String pattern) {
        this(ctx,pattern,"");
    }

    /**
     * A special JSArray returned by the result of JSRegExp.exec()
     * See: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/RegExp/exec
     * @since 3.0
     */
    public class ExecResult extends JSArray<String> {
        protected ExecResult(JSObject result) {
            super(result.valueRef(),result.getContext(),String.class);
            protect(context.ctxRef(), valueRef);
        }

        /**
         * Returns index of match
         * @return The 0-based index of the match in the string
         * @since 3.0
         */
        public Integer index() {
            return property("index").toNumber().intValue();
        }

        /**
         * Returns the original string to be matched
         * @return The original string
         * @since 3.0
         */
        public String input() {
            return property("input").toString();
        }
    }

    /**
     * JavaScript RegExp.prototype.exec(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/RegExp/exec
     * @param str  The string to match
     * @return an ExecResult JSArray, containing match information
     * @since 3.0
     */
    public ExecResult exec(String str) {
        JSValue result = property("exec").toFunction().call(this,str);
        return (result.isNull()) ? null : new ExecResult(result.toObject());
    }

    /**
     * JavaScript RegExp.prototype.lastIndex
     * The index at which to start the next match
     * @return The index at which to start the next match
     * @since 3.0
     */
    public Integer lastIndex() {
        return property("lastIndex").toNumber().intValue();
    }

    /**
     * Indicates if the "i" flag was used to ignore case
     * @return true if "i" flag was used, false otherwise
     * @since 3.0
     */
    public Boolean ignoreCase() {
        return property("ignoreCase").toBoolean();
    }

    /**
     * Indicates if the "g" flag was used for a global match
     * @return true if "g" flag was used, false otherwise
     * @since 3.0
     */
    public Boolean global() {
        return property("global").toBoolean();
    }

    /**
     * Indicates if the "m" flag was used to search in strings across multiple lines
     * @return true if "m" flag was used, false otherwise
     * @since 3.0
     */
    public Boolean multiline() {
        return property("multiline").toBoolean();
    }

    /**
     * Return the text of the pattern
     * @return the text of the pattern
     * @since 3.0
     */
    public String source() {
        return property("source").toString();
    }

    /**
     * JavaScript, RegExp.prototype.test(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/RegExp/test
     * @param str The string against which to match the regular expression
     * @return true if test succeeded, false otherwise
     * @since 3.0
     */
    public Boolean test(String str) {
        return property("test").toFunction().call(this,str).toBoolean();
    }
}
