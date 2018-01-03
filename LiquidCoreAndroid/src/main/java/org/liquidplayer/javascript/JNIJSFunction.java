package org.liquidplayer.javascript;

@SuppressWarnings("JniMissingFunction")
class JNIJSFunction extends JNIJSObject  {
    JNIJSFunction(long ref) {
        super(ref);
    }

    static native JNIJSObject makeFunctionWithCallback(JSFunction thiz, JNIJSContext ctx, String name);
}
