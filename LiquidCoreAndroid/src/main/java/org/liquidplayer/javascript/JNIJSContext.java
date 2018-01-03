package org.liquidplayer.javascript;

@SuppressWarnings("JniMissingFunction")
public class JNIJSContext extends JNIObject {
    JNIJSContext(long ref) {
        super(ref);
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        Finalize(reference);
    }

    static native JNIJSContext create();
    static native JNIJSContext createInGroup(JNIJSContextGroup group);
    native JNIJSContextGroup getGroup();
    native JNIObject getGlobalObject();
    native JNIReturnObject evaluateScript(String script, String sourceURL, int startingLineNumber);
    native void Finalize(long reference);
}
