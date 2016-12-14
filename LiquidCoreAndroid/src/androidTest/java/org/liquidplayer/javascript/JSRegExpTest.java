//
// JSRegExpTest.java
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

import org.junit.Test;

import static org.junit.Assert.*;

public class JSRegExpTest {
    @Test
    public void testJSRegExp() throws Exception {
        JSContext context = new JSContext();
        testJSRegExp(context);
    }

    public void testJSRegExp(JSContext context) throws Exception {
        JSRegExp regExp = new JSRegExp(context,"quick\\s(brown).+?(jumps)","ig");
        JSRegExp.ExecResult result = regExp.exec("The Quick Brown Fox Jumps Over The Lazy Dog");
        assertEquals(result.get(0),"Quick Brown Fox Jumps");
        assertEquals(result.get(1),"Brown");
        assertEquals(result.get(2),"Jumps");
        assertEquals(result.index().intValue(),4);
        assertEquals(result.input(),"The Quick Brown Fox Jumps Over The Lazy Dog");

        assertEquals(regExp.lastIndex().intValue(),25);
        assertEquals(regExp.ignoreCase(),true);
        assertEquals(regExp.global(),true);
        assertEquals(regExp.multiline(),false);
        assertEquals(regExp.source(),"quick\\s(brown).+?(jumps)");

        JSRegExp regExp2 = new JSRegExp(context,"quick\\s(brown).+?(jumps)");
        assertEquals(regExp2.ignoreCase(),false);
        assertEquals(regExp2.global(),false);
        assertEquals(regExp2.multiline(),false);

        String str = "hello world!";
        assertTrue(new JSRegExp(context,"^hello").test(str));
    }
}