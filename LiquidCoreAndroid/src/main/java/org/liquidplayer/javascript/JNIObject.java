package org.liquidplayer.javascript;

abstract class JNIObject extends Throwable {
    JNIObject(long reference) {
        this.reference = reference;
    }

    protected long reference;
}
