package org.liquidplayer.javascript;

class JNIJSException extends Exception {
    JNIJSException(long valueRef) {
        exception = JNIJSValue.fromRef(valueRef);
    }

    final JNIJSValue exception;
}
