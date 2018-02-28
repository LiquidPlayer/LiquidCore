package org.liquidplayer.javascript;

/**
 * Used in communicating with native code.
 * Clients do not need to use this.
 */
class JNIReturnObject {
    /**
     * The boolean return value
     */
    boolean bool;
    /**
     * The numeric return value
     */
    double number;
    /**
     * The reference return value
     */
    long reference;
    /**
     * The exception reference if one was thrown, otherwise 0L
     */
    long exception;

    public String string;
}
