package org.liquidplayer.node;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

import org.junit.Test;
import org.liquidplayer.v8.JSContext;

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
            new Process(InstrumentationRegistry.getContext(),"_",this);
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

    @Test
    public void testFileSystem1() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        String dirx = context.getFilesDir() + "/__org.liquidplayer.node__/__";
        new File(context.getFilesDir() + "/__org.liquidplayer.node__/test.txt").delete();

        final String script = "" +
                "var fs = require('fs');" +
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
                assertEquals("test.txt",ctx.property("files").toString());
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
                assertTrue(ctx.property("a").toString().contains("EACCES"));
                assertTrue(ctx.property("b").toString().contains("EACCES"));
                assertTrue(ctx.property("c").toString().contains("EACCES"));
                assertTrue(ctx.property("d").toString().contains("EACCES"));
                assertTrue(ctx.property("e").toString().contains("EACCES"));
            }
        }).processCompleted.acquire();

    }

}