//
// JSC.java
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
package org.liquidplayer.test;

import org.liquidplayer.javascript.JNIJSContextGroup;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;

import java.util.Scanner;

@SuppressWarnings("JniMissingFunction")
public class JSC {

    static {
        JSContext.dummy();
    }

    private JSContextGroup group = null;

    JSC(JSContextGroup group) {
        this.group = group;
    }

    int testAPI() throws Exception {
        String script = new Scanner(getClass().getClassLoader()
                .getResourceAsStream("testapi.js"), "UTF-8").useDelimiter("\\A").next();
        return main(script, group==null?null:group.groupRef());
    }

    int testMinidom() throws Exception {
        String script = new Scanner(getClass().getClassLoader()
                .getResourceAsStream("minidom.js"), "UTF-8").useDelimiter("\\A").next();
        return minidom(script, group==null?null:group.groupRef());
    }

    private native int main(String script, JNIJSContextGroup contextGroup);
    private native int minidom(String script, JNIJSContextGroup contextGroup);
}
