/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
@SuppressWarnings("WeakerAccess,SameParameterValue")
public class JSContextGroup {
    static {
        JSContext.init();
    }
    private JNIJSContextGroup group;

    public class LoopPreserver {
        private JNILoopPreserver m_reference;
        LoopPreserver(JNILoopPreserver reference) {
            m_reference = reference;
        }

        public void release() {
            if (m_reference != null) m_reference.release();
        }
    }

    /**
     * Creates a new context group
     * @since 0.1.0
     */
    public JSContextGroup() {
        group = JNIJSContextGroup.createGroup();
        hasDedicatedThread = false;
    }

    /**
     * Wraps an existing context group
     * @param groupRef  the JavaScriptCore context group reference
     * @since 0.1.0
     */
    JSContextGroup(JNIJSContextGroup groupRef)
    {
        group = groupRef;
        hasDedicatedThread = group.isManaged();
    }

    @SuppressWarnings("unused") // This is used through Reflection outside this package
    JSContextGroup(long groupRef)
    {
        this(JNIJSContextGroup.fromRef(groupRef));
    }

    /**
     * Creates a new context group for which all contexts created in it will start from
     * a snapshot.  If snapshot file load files, this constructor will fail silently
     * and use the default snapshot.
     * @param snapshot File generated from previous call to createSnapshot()
     */
    public JSContextGroup(File snapshot)
    {
        group = JNIJSContextGroup.createGroupWithSnapshot(snapshot.getAbsolutePath());
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
    public static File createSnapshot(String script, File snapshotToWrite) throws IOException
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
     * Terminates execution of any running script.  The script will exit with an exception.
     */
    public void terminateExecution() {
        group.terminateExecution();
    }

    /**
     * Determines if the ContextGroup is being managed by a single thread (i.e. Node)
     * @return true if managed by a single thread, false otherwise
     */
    private boolean hasDedicatedThread() {
        return hasDedicatedThread;
    }

    /**
     * Gets the JNI context group reference
     * @since 0.1.0
     * @return  the JNI context group reference
     */
    public JNIJSContextGroup groupRef() {
        return group;
    }
    public long groupHash() { return group.reference; }

    public boolean isOnThread() {
        return (!hasDedicatedThread() ||
                android.os.Process.myTid() == mContextGroupThreadTid);
    }

    private int mContextGroupThreadTid = 0;
    @SuppressWarnings("unused") // called from Native code
    private void inContextCallback(Runnable runnable) {
        mContextGroupThreadTid = android.os.Process.myTid();
        runnable.run();
    }

    /**
     * Schedules an asynchronous callback to run in the JavaScript thread.
     * @param runnable A callback Runnable.
     */
    public void schedule(Runnable runnable) {
        group.runInContextGroup(this, runnable);
    }

    public LoopPreserver keepAlive() {
        return new LoopPreserver(group.newLoopPreserver());
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

    private boolean hasDedicatedThread;
}
