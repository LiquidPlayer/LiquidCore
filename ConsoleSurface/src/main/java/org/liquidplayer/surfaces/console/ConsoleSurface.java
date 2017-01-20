//
// ConsoleSurfaceView.java
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
package org.liquidplayer.surfaces.console;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSException;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.Process;
import org.liquidplayer.service.MicroService;
import org.liquidplayer.service.Surface;

/**
 * A ConsoleSurface is a node.js ANSI text console.  ConsoleSurface operates by manipulating
 * the 'process' object in node.  It captures output written to stdout and stderr as well as
 * traps and displays any JavaScript exceptions.
 *
 * So long as the underlying MicroService is still running, ConsoleSurface can inject javascript
 * into a running process through a command line.
 *
 * ConsoleSurface is intended to be used mostly for debugging.
 */
public class ConsoleSurface extends ConsoleView
    implements Process.EventListener, JSContext.IJSExceptionHandler, Surface {

    public ConsoleSurface(Context context) {
        this(context, null);
    }

    public ConsoleSurface(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ConsoleSurface(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void updateState() {
        super.updateState();

    }

    private Process process = null;
    private JSContext js = null;
    private JSFunction console_log = null;
    private boolean processedException = false;
    private int columns = 0, rows = 0;

    @Override
    public void reset() {
        detach();
        super.reset();
    }

    @Override
    protected void resize(int columns, int rows) {
        this.columns = columns;
        this.rows = rows;
        if (state().getString("uuid") != null) {
            do_attach(state().getString("uuid"));
        }
    }

    /**
     * ConsoleSurface is somewhat unusal for Surfaces.  It can be attached and detached at any
     * time during the MicroService lifecycle, as it dynamically overrides stderr, stdout and
     * handles exceptions.  There is no special interface required on the JavaScript side since
     * it operates purely on standard streams.  Therefore, no setup is required before
     * running the service.
     * @param service  The MicroService to attach
     */
    @Override
    public void attach(MicroService service) {
        do_attach(service.getId());
        resize(columns, rows);
    }

    private void do_attach(final String serviceId) {
        if (state().getString("uuid") != null && !serviceId.equals(state().getString("uuid"))) {
            detach();
        }
        if (state().getString("uuid") == null) {
            state().putString("uuid", serviceId);
            MicroService service = MicroService.getService(serviceId);
            if (service != null) {
                process = service.getProcess();
                process.addEventListener(ConsoleSurface.this);
            }
        }
    }

    @Override
    public void detach() {
        if (state().getString("uuid") != null) {
            if (process != null) {
                JSObject stdout =
                        js.property("process").toObject().property("stdout").toObject();
                JSObject stderr =
                        js.property("process").toObject().property("stderr").toObject();
                teardownStream(stdout);
                teardownStream(stderr);

                console_log = null;
                js = null;
                process.removeEventListener(this);
                process = null;
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        inputBox.setEnabled(false);
                        inputBox.setClickable(false);
                        inputBox.setAlpha(0.5f);
                        setButtonEnabled(downHistory,false);
                        setButtonEnabled(upHistory,false);
                    }
                });
            }
            state().remove("uuid");
        }
    }

    @Override
    protected void processCommand(final String cmd) {
        processedException = false;
        new Thread(new Runnable() {
            @Override
            public void run() {
                JSValue output = js.evaluateScript(cmd);
                if (!processedException && console_log != null) {
                    console_log.call(null, output);
                }
            }
        }).start();
    }

    private interface StreamCallback {
        void callback(String string);
    }
    private void setupStream(final JSObject stream, final StreamCallback callback) {
        stream.property("write", new JSFunction(stream.getContext(), "write") {
            @SuppressWarnings("unused")
            public void write(final String string) {
                callback.callback(string);
            }
        });
        stream.property("clearScreenDown", new JSFunction(stream.getContext(),"clearScreenDown",
                "this.write('\\x1b[0J');"));
        stream.property("moveCursor", new JSFunction(stream.getContext(),"moveCursor",
                "var out = ''; c = c || 0; r = r || 0;" +
                        "if (c>0) out += '\\x1b['+c+'C'; else if (c<0) out+='\\x1b['+(-c)+'D';"+
                        "if (r>0) out += '\\x1b['+r+'B'; else if (r<0) out+='\\x1b['+(-r)+'A';"+
                        "this.write(out);",
                "c", "r"));
        stream.property("rows", rows);
        stream.property("columns", columns);
    }
    private void teardownStream(final JSObject stream) {
        // FIXME: restore previous function
        stream.property("write", new JSFunction(js));
        stream.deleteProperty("clearScreenDown");
        stream.deleteProperty("moveCursor");
    }

    @Override
    public void onProcessStart(Process process, JSContext context) {
        js = context;
        js.setExceptionHandler(this);
        console_log = js.property("console").toObject().property("log").toFunction();
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                inputBox.setEnabled(true);
            }
        });

        JSObject stdout =
                js.property("process").toObject().property("stdout").toObject();
        JSObject stderr =
                js.property("process").toObject().property("stderr").toObject();
        setupStream(stdout, new StreamCallback() {
            @Override
            public void callback(String string) {
                consoleTextView.print(string);
            }
        });
        setupStream(stderr, new StreamCallback() {
            @Override
            public void callback(String string) {
                // Make it red!
                string = "\u001b[31m" + string;
                consoleTextView.print(string);
                android.util.Log.e("stderr", string);
            }
        });
    }

    @Override
    public void onProcessAboutToExit(Process process, int exitCode) {
        consoleTextView.println("\u001B[31mProcess about to exit with code " + exitCode);
        android.util.Log.i("onProcessAboutToExit", "Process about to exit with code " + exitCode);
        detach();
    }

    @Override
    public void onProcessExit(Process process, int exitCode) {
        android.util.Log.i("onProcessExit", "exiting");
        consoleTextView.println("\u001B[31mProcess exited with code " + exitCode);
        this.process = null;
    }

    @Override
    public void onProcessFailed(Process process, Exception error) {

    }

    @Override
    public void handle(final JSException e) {
        processedException = true;
        consoleTextView.println("\u001b[31m" + e.stack());
    }
}
