/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import static java.lang.Double.NaN;

class JNIJSUndefined extends JNIJSPrimitive {
    JNIJSUndefined(long ref) { super(ref); }

    @Override boolean isUndefined() { return true; }

    @Override protected boolean primitiveEqual(JNIJSPrimitive b) {
        return (b instanceof JNIJSNull || b instanceof JNIJSUndefined);
    }
    @Override protected boolean primitiveStrictEqual(JNIJSPrimitive b)
    {
        return (b instanceof JNIJSUndefined);
    }

    @Override JNIJSValue createJSONString() { return this; }
    @Override boolean toBoolean() { return false; }
    @Override double toNumber() { return NaN; }
    @Override String toStringCopy() { return "undefined"; }
}
