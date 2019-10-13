/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

class JNIJSException extends Exception {
    JNIJSException(long valueRef) {
        exception = JNIJSValue.fromRef(valueRef);
    }

    final JNIJSValue exception;
}
