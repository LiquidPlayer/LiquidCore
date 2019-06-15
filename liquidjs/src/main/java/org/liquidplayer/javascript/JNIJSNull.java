/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

class JNIJSNull extends JNIJSPrimitive {
    JNIJSNull(long ref) { super(ref); }

    @Override boolean isNull() { return true; }

    @Override protected boolean primitiveEqual(JNIJSPrimitive b) {
        return (b instanceof JNIJSNull || b instanceof JNIJSUndefined);
    }
    @Override protected boolean primitiveStrictEqual(JNIJSPrimitive b)
    {
        return (b instanceof JNIJSNull);
    }

    @Override JNIJSValue createJSONString() { /* FIXME! */ return this; }
    @Override boolean toBoolean() { return false; }
    @Override double toNumber() { return 0; }
    @Override String toStringCopy() { return "null"; }
}
