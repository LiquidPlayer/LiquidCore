//
// JSContextGroup.java
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
 Copyright (c) 2014-2016 Eric Lange. All rights reserved.

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

import java.io.File;
import java.io.IOException;

/**
 * A JSContextGroup associates JavaScript contexts with one another. Contexts
 * in the same group may share and exchange JavaScript objects. Sharing and/or
 * exchanging JavaScript objects between contexts in different groups will produce
 * undefined behavior. When objects from the same context group are used in multiple
 * threads, explicit synchronization is required.
 *
 */
@SuppressWarnings("JniMissingFunction")
public class JSContextGroup {
    static {
        JSContext.init();
    }
    private JNIJSContextGroup group;

    public class LoopPreserver {
        private JNILoopPreserver m_preserver;
        LoopPreserver(JNILoopPreserver jniLoopPreserver) {
            m_preserver = jniLoopPreserver;
        }

        public void release() {
            if (m_preserver != null) m_preserver.release();
        }
    }

    /**
     * Creates a new context group
     * @since 0.1.0
     */
    public JSContextGroup() {
        group = JNIJSContextGroup.create();
        hasDedicatedThread = false;
    }

    /**
     * Wraps an existing context group
     * @param groupRef  the JavaScriptCore context group reference
     * @since 0.1.0
     */
    JSContextGroup(Object groupRef)
    {
        group = (JNIJSContextGroup)groupRef;
        hasDedicatedThread = group.isManaged();
    }

    /**
     * Creates a new context group for which all contexts created in it will start from
     * a snapshot.  If snapshot file load files, this constructor will fail silently
     * and use the default snapshot.
     * @param snapshot File generated from previous call to createSnapshot()
     */
    public JSContextGroup(File snapshot)
    {
        group = JNIJSContextGroup.createWithSnapshotFile(snapshot.getAbsolutePath());
        hasDedicatedThread = false;
    }

    /**
     * Creates a snapshot of JavaScript code that can be used by the JSContextGroup(File)
     * constructor to speed up startup of new contexts.
     * @param script Script to create snapshot from
     * @param snapshotToWrite File to write (will be overwritten if exists)
     * @return The written file
     * @throws IOException thrown if taking snapshot or writing to the file fails; check message
     */
    static File createSnapshot(String script, File snapshotToWrite) throws IOException
    {
        int result = JNIJSContextGroup.createSnapshot(script, snapshotToWrite.getAbsolutePath());
        switch(result) {
            case  0: return snapshotToWrite; // success
            case -1: throw new IOException("Failed to create snapshot");
            case -2: throw new IOException("Unable to open for writing");
            case -3: throw new IOException("Could not write file");
            case -4: throw new IOException("Could not close file");
            default: throw new IOException();
        }
    }

    /**
     * Determines if the ContextGroup is being managed by a single thread (i.e. Node)
     * @return true if managed by a single thread, false otherwise
     */
    private boolean hasDedicatedThread() {
        return hasDedicatedThread;
    }

    /**
     * Gets the JavaScriptCore context group reference
     * @since 0.1.0
     * @return  the JavaScriptCore context group reference
     */
    public JNIJSContextGroup groupRef() {
        return group;
    }

    boolean isOnThread() {
        return (!hasDedicatedThread() ||
                android.os.Process.myTid() == mContextGroupThreadTid);
    }

    private int mContextGroupThreadTid = 0;
    @SuppressWarnings("unused") // called from Native code
    private void inContextCallback(Runnable runnable) {
        mContextGroupThreadTid = android.os.Process.myTid();
        runnable.run();
    }

    void schedule(Runnable runnable) {
        group.runInContextGroup(this, runnable);
    }

    public LoopPreserver keepAlive() {
        return new LoopPreserver(JNILoopPreserver.create(groupRef()));
    }

    /**
     * Checks if two JSContextGroups refer to the same JS context group
     * @param other the other object to compare
     * @return true if refer to same context group, false otherwise
     * @since 0.1.0
     */
    @Override
    public boolean equals(Object other) {
        return (other !=null) &&
                (this == other) ||
                (other instanceof JSContextGroup) &&
                groupRef() != null &&
                groupRef().equals(((JSContextGroup)other).groupRef());
    }

    private boolean hasDedicatedThread = false;
}
