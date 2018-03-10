//
// JNIJSNumber.java
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

    @Override JNIJSValue createJSONString() throws JNIJSException { /* FIXME! */ return this; }
    @Override boolean toBoolean() { return value!=0; }
    @Override double toNumber() throws JNIJSException { return value; }
    @Override String toStringCopy() throws JNIJSException
    {
        return df.format(value);
    }

    private final static DecimalFormat df = new DecimalFormat("0",
            DecimalFormatSymbols.getInstance(Locale.ENGLISH));
    static {
        df.setMaximumFractionDigits(340); //340 = DecimalFormat.DOUBLE_FRACTION_DIGITS
    }

}
