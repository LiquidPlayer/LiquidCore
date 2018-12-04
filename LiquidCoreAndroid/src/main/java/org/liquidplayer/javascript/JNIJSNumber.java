/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;

class JNIJSNumber extends JNIJSPrimitive {
    JNIJSNumber(long ref)
    {
        super(ref);
        value = Double.longBitsToDouble(ref);
    }
    final private double value;

    @Override boolean isNumber() { return true; }

    @Override protected boolean primitiveEqual(JNIJSPrimitive b)
    {
        return ((b instanceof JNIJSNumber) &&
            ((JNIJSNumber) b).value == value) ||
            ((b instanceof JNIJSBoolean) &&
                    (((JNIJSBoolean) b).reference == ODDBALL_TRUE && toBoolean() ||
                    ((JNIJSBoolean) b).reference == ODDBALL_FALSE && !toBoolean()));
    }
    @Override protected boolean primitiveStrictEqual(JNIJSPrimitive b)
    {
        return (b instanceof JNIJSNumber) && ((JNIJSNumber) b).value == value;
    }

    @Override JNIJSValue createJSONString() { /* FIXME! */ return this; }
    @Override boolean toBoolean() { return value!=0; }
    @Override double toNumber() { return value; }
    @Override String toStringCopy()
    {
        return df.format(value);
    }

    private final static DecimalFormat df = new DecimalFormat("0",
            DecimalFormatSymbols.getInstance(Locale.ENGLISH));
    static {
        df.setMaximumFractionDigits(340); //340 = DecimalFormat.DOUBLE_FRACTION_DIGITS
    }

}
