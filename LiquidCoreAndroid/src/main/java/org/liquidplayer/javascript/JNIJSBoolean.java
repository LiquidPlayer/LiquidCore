/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

class JNIJSBoolean extends JNIJSPrimitive {
    JNIJSBoolean(long ref) { super(ref); }

    @Override boolean isBoolean() { return true; }

    @Override protected boolean primitiveEqual(JNIJSPrimitive b)
    {
        return (b instanceof JNIJSNumber) ? b.primitiveEqual(this) :
                (b instanceof JNIJSBoolean) && ((JNIJSBoolean) b).reference == reference;
    }
    @Override protected boolean primitiveStrictEqual(JNIJSPrimitive b) {
        return (b instanceof JNIJSBoolean) && ((JNIJSBoolean) b).reference == reference;
    }

    @Override JNIJSValue createJSONString() { /* FIXME! */ return this; }
    @Override boolean toBoolean() { return reference==ODDBALL_TRUE; }
    @Override double toNumber() { return (reference==ODDBALL_TRUE) ? 1 : 0; }
    @Override String toStringCopy()
    {
        return (reference==ODDBALL_TRUE) ? "true" : "false";
    }
}
