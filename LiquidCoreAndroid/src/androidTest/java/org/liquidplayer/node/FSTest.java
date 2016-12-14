//
// FSTest.java
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
package org.liquidplayer.node;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

import org.junit.Test;
import org.liquidplayer.javascript.JSBaseArray;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSObject;

import java.io.File;
import java.io.InputStream;
import java.util.Scanner;
import java.util.concurrent.Semaphore;

import static org.junit.Assert.*;

public class FSTest {

    private interface OnDone {
        void onDone(JSContext ctx);
    }
    private class Script implements Process.EventListener {

        final private String script;
        final Semaphore processCompleted = new Semaphore(0);
        final private OnDone onDone;
        private JSContext context;

        Script(String script, OnDone onDone) {
            this.script = script;
            this.onDone = onDone;
            new Process(InstrumentationRegistry.getContext(),"_",
                    Process.kMediaAccessPermissionsRW,this);
        }

        @Override
        public void onProcessStart(final Process proc, final JSContext ctx) {
            context = ctx;
            context.evaluateScript(script);
        }

        @Override
        public void onProcessAboutToExit(Process process, int exitCode) {
            onDone.onDone(context);
        }

        @Override
        public void onProcessExit(Process process, int exitCode) {
            processCompleted.release();
        }

        @Override
        public void onProcessFailed(Process process, Exception error) {

        }
    }

    private class Foo extends JSObject {
        Foo(JSContext ctx) { super(ctx); }

        @jsexport(type = Integer.class)
        Property<Integer> x;

        @jsexport(type = String.class)
        Property<String>  y;

        @jsexport(attributes = JSPropertyAttributeReadOnly)
        Property<String> read_only;

        @SuppressWarnings("unused")
        @jsexport(attributes = JSPropertyAttributeReadOnly | JSPropertyAttributeDontDelete)
        int incr(int x) {
            return x+1;
        }
    }

    @Test
    public void testFileSystem1() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        String dirx = context.getFilesDir() + "/__org.liquidplayer.node__/__/persistent";

        final String script = "" +
                "var fs = require('fs');" +
                "process.chdir('persistent');" +
                "fs.writeFile('test.txt', 'Hello, World!', function(err) {" +
                "   if(err) {" +
                "       return console.log(err);" +
                "   }" +
                "   console.log('The file was saved!');" +
                "   fs.readdir('.', function(err,files) {" +
                "       global.files = files;" +
                "   });" +
                "});" +
                "";
        new Script(script, new OnDone() {
            @Override
            public void onDone(JSContext ctx) {
                JSBaseArray files = ctx.property("files").toJSArray();
                assertTrue(files.contains("test.txt"));

                Foo foo = new Foo(ctx);
                ctx.property("foo", foo);
                ctx.evaluateScript("foo.x = 5; foo.y = 'test';");
                assertEquals((Integer)5, foo.x.get());
                assertEquals("test", foo.y.get());
                foo.x.set(6);
                foo.y.set("test2");
                assertEquals(6, foo.property("x").toNumber().intValue());
                assertEquals("test2", foo.property("y").toString());
                assertEquals(6, ctx.evaluateScript("foo.x").toNumber().intValue());
                assertEquals("test2", ctx.evaluateScript("foo.y").toString());
                ctx.evaluateScript("foo.x = 11");
                assertEquals((Integer)11, foo.x.get());
                assertEquals(21, ctx.evaluateScript("foo.incr(20)").toNumber().intValue());

                foo.read_only.set("Ok!");
                assertEquals("Ok!", foo.read_only.get());
                foo.read_only.set("Not Ok!");
                assertEquals("Ok!", foo.read_only.get());
                ctx.evaluateScript("foo.read_only = 'boo';");
                assertEquals("Ok!", foo.read_only.get());

            }
        }).processCompleted.acquire();

        String content = new Scanner(new File(dirx + "/test.txt")).useDelimiter("\\Z").next();
        assertEquals("Hello, World!", content);
        assertTrue(new File(dirx + "/test.txt").delete());
    }

    @Test
    public void testLetsBeNaughty() throws Exception {
        InputStream in = getClass().getClassLoader().getResourceAsStream("fsTest.js");
        Scanner s = new Scanner(in).useDelimiter("\\A");
        String script = s.hasNext() ? s.next() : "";

        new Script(script, new OnDone() {
            @Override
            public void onDone(JSContext ctx) {
                android.util.Log.d("testLetsBeNaughty", ctx.property("a").toString());
                assertTrue(ctx.property("a").toString().contains("EACCES"));
                android.util.Log.d("testLetsBeNaughty", ctx.property("b").toString());
                assertTrue(ctx.property("b").toString().contains("EACCES") ||
                        ctx.property("b").toString().contains("ENOENT"));
                android.util.Log.d("testLetsBeNaughty", ctx.property("c").toString());
                assertTrue(ctx.property("c").toString().contains("EACCES"));
                android.util.Log.d("testLetsBeNaughty", ctx.property("d").toString());
                assertTrue(ctx.property("d").toString().contains("EACCES"));
                android.util.Log.d("testLetsBeNaughty", ctx.property("e").toString());
                assertTrue(ctx.property("e").toString().contains("EACCES"));
            }
        }).processCompleted.acquire();

    }

}