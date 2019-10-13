/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import java.util.Iterator;

/**
 * A JavaScript iterator interface shadow object
 * @since 0.1.0
 * @param <T> Parameterized type of iterator elements
 */
/*package*/ class JSIterator<T> extends JSObjectWrapper implements Iterator<T> {
    /**
     * Represents the object returned by 'next'
     * @since 0.1.0
     */
    public class Next extends JSObjectWrapper {
        Next(JSObject next) {
            super(next);
        }

        /**
         * Tests if there are any more elements in the array
         * @return true if more elements to iterate, false otherwise
         */
        boolean done() {
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

    JSIterator(JSObject iterator) {
        super(iterator);
        next = _jsnext();
    }
    private Next next;

    private Next _jsnext() {
        return new Next(getJSObject().property("next").toFunction().call(getJSObject()).toObject());
    }

    /**
     * The 'next' JavaScript iterator object
     * @since 0.1.0
     * @return the next JSObject in the JSIterator
     */
    Next jsnext() {
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
