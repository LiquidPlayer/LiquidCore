package org.liquidplayer.javascript;

/**
 * Used in communicating with JavaScriptCore JNI.
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
    JNIObject reference;
    /**
     * The exception reference if one was thrown, otherwise 0L
     */
    JNIObject exception;

    public String string;
}
