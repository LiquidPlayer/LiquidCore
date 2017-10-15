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
    private Long group;

    /**
     * Creates a new context group
     * @since 0.1.0
     */
    public JSContextGroup() {
        group = create();
        hasDedicatedThread = false;
    }
    /**
     * Wraps an existing context group
     * @param groupRef  the JavaScriptCore context group reference
     * @since 0.1.0
     */
    public JSContextGroup(Long groupRef)
    {
        group = groupRef;
        hasDedicatedThread = isManaged(group);
        /* If the entire JSContextGroup is running in a dedicated thread, then let
         * that thread handle lifecycle.  Release here instead of in finalizer, which may
         * only get called after the thread is killed or possibly deadlock.
         */
        if (hasDedicatedThread) {
            release(group);
        }
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        /* If we are not managed by a single thread, we can release this group during
         * finalization.  It is unsafe to do so if running in a managed thread.  We will let
         * the native code handle that.
         */
        if (!hasDedicatedThread) {
            release(group);
        }
    }

    /**
     * Determines if the ContextGroup is being managed by a single thread (i.e. Node)
     * @return true if managed by a single thread, false otherwise
     */
    boolean hasDedicatedThread() {
        return hasDedicatedThread;
    }

    /**
     * Gets the JavaScriptCore context group reference
     * @since 0.1.0
     * @return  the JavaScriptCore context group reference
     */
    public Long groupRef() {
        return group;
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
                !(groupRef() == null || groupRef() == 0) &&
                groupRef().equals(((JSContextGroup)other).groupRef());
    }

    protected native long create();
    protected native static void release(long group);
    protected native boolean isManaged(long group);

    private boolean hasDedicatedThread = false;
}
