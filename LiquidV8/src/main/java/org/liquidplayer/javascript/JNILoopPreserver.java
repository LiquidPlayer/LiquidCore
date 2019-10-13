/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

class JNILoopPreserver extends JNIObject {
    JNILoopPreserver(long ref) {
        super(ref);
    }

    @Override
    protected void finalize() {
        Finalize(reference);
    }

    void release()
    {
        release(reference);
    }

    static native long create(long groupRef);
    private static native void release(long loopRef);
    private native void Finalize(long loopRef);
}
