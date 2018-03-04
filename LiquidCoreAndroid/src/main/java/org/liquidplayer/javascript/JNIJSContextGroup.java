//
// JNIJSContextGroup.java
//
// AndroidJSCore project
// https://github.com/ericwlange/AndroidJSCore/
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

    /* Natives */
    static native int createSnapshot(String script, String snapshotFile);

    private static native long create();
    private static native long createWithSnapshotFile(String snapshotFile);
    private static native boolean isManaged(long groupRef);
    private static native void runInContextGroup(long groupRef, Object thisObject, Runnable runnable);
    private static native void Finalize(long groupRef);
}
