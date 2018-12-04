/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.node;

import android.Manifest;
import android.content.Context;
import android.os.Environment;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.GrantPermissionRule;

import org.junit.Rule;
import org.junit.Test;
import org.liquidplayer.javascript.JSBaseArray;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;
import org.liquidplayer.javascript.JSException;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;

import java.io.File;
import java.io.InputStream;
import java.util.Scanner;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class FSTest {

    @Rule
    public GrantPermissionRule mRuntimePermissionRule
            = GrantPermissionRule.grant(Manifest.permission.WRITE_EXTERNAL_STORAGE);

    private interface OnDone {
        void onDone(JSContext ctx);
    }
    private class Script implements Process.EventListener {

        final private String script;
        final CountDownLatch processCompleted = new CountDownLatch(1);
        final private OnDone onDone;
        private JSContext context;
        Throwable exception;

        Script(String script, OnDone onDone) {
            this.script = script;
            this.onDone = onDone;
            new Process(InstrumentationRegistry.getContext(),"_",
                    Process.kMediaAccessPermissionsRW,this);
        }

        @Override
        public void onProcessStart(final Process proc, final JSContext ctx) {
            context = ctx;
            context.setExceptionHandler(new JSContext.IJSExceptionHandler() {
                @Override
                public void handle(JSException exception) {
                    android.util.Log.e("ProcessException", exception.stack());
                }
            });
            context.evaluateScript(script);
        }

        @Override
        public void onProcessAboutToExit(Process process, int exitCode) {
            try {
                onDone.onDone(context);
            } catch (Throwable e) {
                exception = e;
            }
        }

        @Override
        public void onProcessExit(Process process, int exitCode) {
            processCompleted.countDown();
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
    public void testFileSystem1() throws Throwable {
        Context context = InstrumentationRegistry.getContext();
        String dirx = context.getFilesDir() + "/__org.liquidplayer.node__/__/local";

        final String script = "" +
                "var fs = require('fs');" +
                "process.chdir('./local');" +
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
        Script s = new Script(script, new OnDone() {
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
        });
        assertTrue(s.processCompleted.await(10L, TimeUnit.SECONDS));
        if (s.exception != null) throw s.exception;

        String content = new Scanner(new File(dirx + "/test.txt")).useDelimiter("\\Z").next();
        assertEquals("Hello, World!", content);

        assertTrue(new File(dirx + "/test.txt").exists());
        assertTrue(new File(dirx).exists());
        assertTrue(new File(context.getFilesDir() + "/__org.liquidplayer.node__/__").exists());
        assertTrue(new File(context.getCacheDir() + "/__org.liquidplayer.node__/__").exists());
        File external = context.getExternalFilesDir(null);
        if (external != null) {
            assertTrue(new File(external.getAbsolutePath() + "/LiquidPlayer/_").exists());
        }

        Process.uninstall(context, "_", Process.UninstallScope.Local);

        assertFalse(new File(dirx + "/test.txt").exists());
        assertFalse(new File(dirx).exists());
        assertFalse(new File(context.getFilesDir() + "/__org.liquidplayer.node__/__").exists());
        assertFalse(new File(context.getCacheDir() + "/__org.liquidplayer.node__/__").exists());
        if (external != null) {
            assertTrue(new File(external.getAbsolutePath() + "/LiquidPlayer/_").exists());
        }
    }

    @Test
    public void testLetsBeNaughty() throws Throwable {
        InputStream in = getClass().getClassLoader().getResourceAsStream("fsTest.js");
        Scanner s = new Scanner(in).useDelimiter("\\A");
        String script = s.hasNext() ? s.next() : "";

        Script scr = new Script(script, new OnDone() {
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
        });
        assertTrue(scr.processCompleted.await(10L, TimeUnit.SECONDS));
        if (scr.exception != null) throw scr.exception;

        Process.uninstall(InstrumentationRegistry.getContext(), "_", Process.UninstallScope.Local);
    }

    /**
     * https://github.com/LiquidPlayer/LiquidCore/issues/9
     */
    @Test
    public void testChdirCwdMultipleProcesses() throws Exception {
        final String dir1 = "/home/local";
        final String dir2 = "/home/cache";

        final String proc1 = "process.chdir('" + dir1 + "');";
        final String proc2 = "process.chdir('" + dir2 + "');";

        final CountDownLatch cdl = new CountDownLatch(2);

        class Contexts {
            JSContext c1;
            JSContext c2;
            JSContextGroup.LoopPreserver p1;
            JSContextGroup.LoopPreserver p2;
        }

        final Contexts contexts = new Contexts();

        new Process(InstrumentationRegistry.getContext(), "_",
                Process.kMediaAccessPermissionsRW, new Process.EventListener() {
            @Override
            public void onProcessStart(Process process, JSContext context) {
                contexts.c1 = context;
                contexts.p1 = process.keepAlive();
                cdl.countDown();
            }

            @Override public void onProcessAboutToExit(Process process, int exitCode) {}
            @Override public void onProcessExit(Process process, int exitCode) {}
            @Override public void onProcessFailed(Process process, Exception error) {}
        });

        new Process(InstrumentationRegistry.getContext(), "_",
                Process.kMediaAccessPermissionsRW, new Process.EventListener() {
            @Override
            public void onProcessStart(Process process, JSContext context) {
                contexts.c2 = context;
                contexts.p2 = process.keepAlive();
                cdl.countDown();
            }

            @Override public void onProcessAboutToExit(Process process, int exitCode) {}
            @Override public void onProcessExit(Process process, int exitCode) {}
            @Override public void onProcessFailed(Process process, Exception error) {}
        });

        assertTrue(cdl.await(10L, TimeUnit.SECONDS));

        contexts.c1.evaluateScript(proc1);
        JSValue v = contexts.c1.evaluateScript("process.cwd()");
        assertEquals(dir1, v.toString());

        contexts.c2.evaluateScript(proc2);
        v = contexts.c2.evaluateScript("process.cwd()");
        assertEquals(dir2, v.toString());

        v = contexts.c1.evaluateScript("process.cwd()");
        assertEquals(dir1, v.toString());

        contexts.p1.release();
        contexts.p2.release();

        Process.uninstall(InstrumentationRegistry.getContext(), "_", Process.UninstallScope.Local);
    }

    @Test
    public void testGlobalUninstall() throws Throwable {

        final String script = "" +
                "var fs = require('fs');" +
                "var external = true;" +
                "try {" +
                "   process.chdir('public');" +
                "} catch (e) {" +
                "   console.log('no external storage');" +
                "   external = false;" +
                "}" +
                "if (external) {" +
                "   fs.writeFile('data/test.txt', 'Hello, World!', function(err) {" +
                "      if(err) {" +
                "          return console.log(err);" +
                "      }" +
                "      console.log('/home/public/data/test.txt was saved!');" +
                "   });" +
                "   fs.writeFile('media/Downloads/test2.txt', 'Hello, World!', function(err) {" +
                "      if(err) {" +
                "         return console.log(err);" +
                "      }" +
                "      console.log('/home/public/media/Downloads/test2.txt was saved!');" +
                "   });" +
                "}" +
                "";
        Script s = new Script(script, new OnDone() {
            @Override
            public void onDone(JSContext ctx) {
            }
        });

        assertTrue(s.processCompleted.await(10L, TimeUnit.SECONDS));
        if (s.exception != null) throw s.exception;

        File external = InstrumentationRegistry.getContext().getExternalFilesDir(null);
        if (external != null) {
            assertTrue(new File(external.getAbsolutePath() + "/LiquidPlayer/_").exists());
            assertTrue(new File(external.getAbsolutePath() + "/LiquidPlayer/_/test.txt").exists());
        }

        File media = null;
        String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state) ||
                Environment.MEDIA_MOUNTED_READ_ONLY.equals(state) ||
                Environment.MEDIA_SHARED.equals(state)){
            media = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
            if (media != null) {
                assertTrue(new File(media.getAbsolutePath() + "/test2.txt").exists());
            }
        }

        Process.uninstall(InstrumentationRegistry.getContext(), "_", Process.UninstallScope.Global);

        if (external != null) {
            assertFalse(new File(external.getAbsolutePath() + "/LiquidPlayer/_").exists());
            assertFalse(new File(external.getAbsolutePath() + "/LiquidPlayer/_/test.txt").exists());
        }
        if (media != null) {
            assertTrue(new File(media.getAbsolutePath() + "/test2.txt").exists());
            assertTrue(new File(media.getAbsolutePath() + "/test2.txt").delete());
        }
    }
}