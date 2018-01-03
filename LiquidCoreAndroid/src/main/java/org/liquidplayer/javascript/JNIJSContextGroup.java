package org.liquidplayer.javascript;

@SuppressWarnings("JniMissingFunction")
public class JNIJSContextGroup extends JNIObject {
    JNIJSContextGroup(long ref) {
        super(ref);
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        Finalize(reference);
    }

    static native JNIJSContextGroup create();
    native boolean isManaged();
    native void runInContextGroup(Object thisObject, Runnable runnable);
    native void Finalize(long reference);
}
