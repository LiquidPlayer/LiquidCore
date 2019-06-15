/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;

class JNIJSContextGroup extends JNIObject {
    private JNIJSContextGroup(long ref) {
        super(ref);
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        Finalize(reference);
    }

    @NonNull static JNIJSContextGroup createGroup()
    {
        return new JNIJSContextGroup(create());
    }

    @NonNull static JNIJSContextGroup createGroupWithSnapshot(String snapshotFile)
    {
        return new JNIJSContextGroup(createWithSnapshotFile(snapshotFile));
    }

    static JNIJSContextGroup fromRef(long groupRef)
    {
        return new JNIJSContextGroup(groupRef);
    }

    boolean isManaged()
    {
        return isManaged(reference);
    }

    void runInContextGroup(Object thisObject, Runnable runnable)
    {
        runInContextGroup(reference, thisObject, runnable);
    }

    JNILoopPreserver newLoopPreserver()
    {
        return new JNILoopPreserver(JNILoopPreserver.create(reference));
    }

    void terminateExecution() {
        TerminateExecution(reference);
    }

    /* Natives */
    static native int createSnapshot(String script, String snapshotFile);

    private static native long create();
    private static native long createWithSnapshotFile(String snapshotFile);
    private static native boolean isManaged(long groupRef);
    private static native void runInContextGroup(long groupRef, Object thisObject, Runnable runnable);
    private static native void Finalize(long groupRef);
    private static native void TerminateExecution(long groupRef);
}
