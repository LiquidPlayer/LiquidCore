//
// JSIterator.java
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
 Copyright (c) 2014 Eric Lange. All rights reserved.

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

import java.util.Iterator;

/**
 * A JavaScript iterator interface shadow object
 * @since 0.1.0
 * @param <T> Parameterized type of iterator elements
 */
public class JSIterator<T> extends JSObjectWrapper implements Iterator<T> {
    /**
     * Represents the object returned by 'next'
     * @since 0.1.0
     */
    public class Next extends JSObjectWrapper {
        protected Next(JSObject next) {
            super(next);
        }

        /**
         * Tests if there are any more elements in the array
         * @return true if more elements to iterate, false otherwise
         */
        public boolean done() {
            return getJSObject().property("done").toBoolean();
        }

        /**
         * Returns the JSValue of the iterated element
         * @return the value returned from next()
         */
        public JSValue value() {
            return getJSObject().property("value");
        }
    }

    /**
     * Wraps a JavaScript iterator in a Java iterator
     * @since 0.1.0
     * @param iterator the JavaScript iterator object.  Assumes the object is a properly formed JS
     *                 iterator
     */
    public JSIterator(JSObject iterator) {
        super(iterator);
        next = _jsnext();
    }
    private Next next = null;

    private Next _jsnext() {
        return new Next(getJSObject().property("next").toFunction().call(getJSObject()).toObject());
    }

    /**
     * The 'next' JavaScript iterator object
     * @since 0.1.0
     * @return the next JSObject in the JSIterator
     */
    public Next jsnext() {
        Next ret = next;
        next = _jsnext();
        return ret;
    }

    /**
     * @see Iterator#next()
     * @since 0.1.0
     * @return next value in the iterator
     */
    @Override
    @SuppressWarnings("unchecked")
    public T next() {
        return (T) jsnext().value();
    }

    /**
     * @see Iterator#hasNext()
     * @since 0.1.0
     * @return true if next() will return a value, false if no values left
     */
    @Override
    public boolean hasNext() {
        return !next.done();
    }

    /**
     * @see Iterator#remove()
     * @since 0.1.0
     * @throws UnsupportedOperationException always
     */
    @Override
    public void remove() {
        throw new UnsupportedOperationException();
    }
}
