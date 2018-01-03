package org.liquidplayer.javascript;

@SuppressWarnings("JniMissingFunction")
class JNIJSObject extends JNIJSValue {
    JNIJSObject(long ref) {
        super(ref);
    }

    /* Native Methods */

    static native JNIJSObject make(JNIJSContext ctx);
    static native JNIReturnObject makeArray(JNIJSContext ctx, JNIJSValue [] args);
    static native JNIJSObject makeDate(JNIJSContext ctx, long[] args);
    static native JNIJSObject makeError(JNIJSContext ctx, String message);
    static native JNIReturnObject makeRegExp(JNIJSContext ctx, String pattern, String flags);
    static native JNIReturnObject makeFunction(JNIJSContext ctx, String name, String func, String sourceURL,
                                               int startingLineNumber);

    native JNIJSValue getPrototype();
    native void setPrototype(JNIJSValue value);
    native boolean hasProperty(String propertyName);
    native JNIReturnObject getProperty(String propertyName);
    native JNIReturnObject setProperty(String propertyName, JNIJSValue value, int attributes);
    native JNIReturnObject deleteProperty(String propertyName);
    native JNIReturnObject getPropertyAtIndex(int propertyIndex);
    native JNIReturnObject setPropertyAtIndex(int propertyIndex, JNIJSValue value);
    native boolean isFunction();
    native JNIReturnObject callAsFunction(JNIJSObject thisObject, JNIJSValue [] args);
    native boolean isConstructor();
    native JNIReturnObject callAsConstructor(JNIJSValue [] args);
    native String[] copyPropertyNames();
}
