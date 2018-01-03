package org.liquidplayer.javascript;

abstract class JNIObject {
    JNIObject(long reference) {
        this.reference = reference;
    }

    protected long reference;
}
