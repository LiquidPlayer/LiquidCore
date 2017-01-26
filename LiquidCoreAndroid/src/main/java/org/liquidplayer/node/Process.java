//
// Process.java
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

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;

import java.lang.ref.WeakReference;
import java.util.ArrayList;

@SuppressWarnings("JniMissingFunction")
public class Process {

    final public static int kContextFinalizedButProcessStillActive = -222;

    final public static int kMediaAccessPermissionsNone = 0;
    final public static int kMediaAccessPermissionsRead = 1;
    final public static int kMediaAccessPermissionsWrite = 2;
    final public static int kMediaAccessPermissionsRW = 3;

    /**
     * Clients must subclass an EventListener to get state change information from the
     * node.js process
     */
    public interface EventListener {
        /**
         * Called when a node.js process is actively running.  This is the one and only opportunity
         * to start running JavaScript in the process.  If the receiver returns from this method
         * without executing asynchronous JS, the process will exit almost immediately afterward.
         * This is the time to execute scripts.  If the caller requires this process to remain
         * alive so scripts can be executed later, Process.keepAlive() should be called here.
         *
         * This method is called inside of the node.js process thread, so any JavaScript code called
         * here is executed synchronously.
         *
         * In the event that an EventListener is added by Process.addEventListener() after a
         * process has already started, this method will be called immediately (in the process
         * thread) if the process is still active.  Otherwise, onProcessExit() will be called.
         *
         * The caller must hold a reference to 'context' until it is done with the process.  This
         * is a safeguard to protect against misbehavior, intended or not, where a caller starts
         * a process, calls Process.keepAlive() or puts in an endless loop, and then exits.  To
         * avoid having the process run indefinitely, if the context gets garbage collected, it will
         * kill the process during the finalization sequence with exit code
         * kContextFinalizedButProcessStillActive.
         *
         * @param process The node.js Process object
         * @param context The JavaScript JSContext for this process
         */
        void onProcessStart(final Process process, final JSContext context);

        /**
         * Called when a node.js process has completed all of its callbacks and has nothing left
         * to do.  If there is any cleanup required, now is the time to do it.  If the caller would
         * like the process to abort its exit sequence, this is the time to either call
         * Process.keepAlive() or execute an asynchronous operation.  If no callbacks are pending
         * after this method returns, then the process will exit.
         *
         * This method is called inside of the node.js process thread, so any JavaScript code called
         * here is executed synchronously.
         *
         * @param process The node.js Process object
         * @param exitCode The node.js exit code
         */
        void onProcessAboutToExit(final Process process, int exitCode);

        /**
         * Called after a process has exited.  The process is no longer active and cannot be used.
         *
         * In the event that an EventListener is added by Process.addEventListener() after a
         * process has already exited, this method will be called immediately.
         *
         * @param process The defunct node.js Process object
         * @param exitCode The node.js exit code
         */
        void onProcessExit(final Process process, int exitCode);

        /**
         * Called in the event of a Process failure
         *
         * @param process  The node.js process object
         * @param error The thrown exception
         */
        void onProcessFailed(final Process process, Exception error);
    }

    /**
     * Creates a node.js process and attaches an event listener
     * @param listener the listener interface object
     */
    public Process(Context androidContext, String uniqueID, int mediaAccessMask,
                   EventListener listener) {
        addEventListener(listener);
        processRef = start();
        androidCtx = androidContext;
        this.uniqueID = uniqueID;
        this.mediaAccessMask = mediaAccessMask;
    }

    /**
     * Adds an EventListener to this Process
     * @param listener the listener interface object
     */
    public synchronized void addEventListener(final EventListener listener) {
        if (!listeners.contains(listener)) {
            listeners.add(listener);
        }
        if (isActive()) {
            jscontext.get().async(new Runnable() {
                @Override
                public void run() {
                    if (isActive()) {
                        listener.onProcessStart(Process.this, jscontext.get());
                    } else {
                        listener.onProcessExit(Process.this, Long.valueOf(exitCode).intValue());
                    }
                }
            });
        } else if (isDone) {
            listener.onProcessExit(Process.this, Long.valueOf(exitCode).intValue());
        }
    }

    /**
     * Removes a listener from this Process
     * @param listener the listener interface object to remove
     */
    public synchronized void removeEventListener(EventListener listener) {
        android.util.Log.d("removeEventListener", "There were " + listeners.size() + " listeners");
        listeners.remove(listener);
        android.util.Log.d("removeEventListener", "There are now " + listeners.size() + " listeners");
    }

    /**
     * Determines if the process is currently active.  If it is inactive, either it hasn't
     * yet been started, or the process completed. Use an @EventListener to determine the
     * state.
     * @return true if active, false otherwise
     */
    public boolean isActive() {
        return isActive && jscontext.get() != null;
    }

    /**
     * Instructs the VM to halt execution as quickly as possible
     * @param exitc The exit code
     */
    public void exit(final int exitc) {
        if (isActive()) {
            jscontext.get().async(new Runnable() {
                @Override
                public void run() {
                    if (isActive()) {
                        jscontext.get().evaluateScript("process.exit(" + exitc + ");");
                    }
                }
            });
        }
    }

    /**
     * Instructs the VM not to shutdown the process when no more callbacks are pending.  In effect,
     * this method indefinitely leaves a callback pending until @letDie() is called.  This must
     * be followed up by a call to letDie() or the process will remain active indefinitely.
     */
    public void keepAlive() {
        if (isActive()) {
            jscontext.get().keepAlive();
        }
    }

    /**
     * Instructs the VM to not keep itself alive if no more callbacks are pending.
     */
    public void letDie() {
        if (isActive()) {
            jscontext.get().letDie();
        }
    }

    /**
     * Determines the scope of an uninstallation.  A Local uninstallation will only clear
     * data and files related to instances on this host.  A Global uninstallation wiil
     * clear also public data shared between hosts.
     */
    public enum UninstallScope {
        Local,
        Global
    }

    /**
     * Uninstalls a given process class identified by it uniqueID
     * @param ctx The Android context
     * @param uniqueID The id of the process class
     * @param scope scope in which to uninstall the process class
     */
    public static void uninstall(Context ctx, String uniqueID, UninstallScope scope) {
        FileSystem.uninstallLocal(ctx, uniqueID);
        if (scope == UninstallScope.Global) {
            FileSystem.uninstallGlobal(ctx, uniqueID);
        }
    }

    /** -- private methods -- **/

    private synchronized void eventOnStart(JSContext ctx) {
        for (EventListener listener : listeners.toArray(new EventListener[listeners.size()])) {
            listener.onProcessStart(this, ctx);
        }
    }
    private synchronized void eventOnAboutToExit(long code) {
        exitCode = code;
        for (EventListener listener : listeners.toArray(new EventListener[listeners.size()])) {
            listener.onProcessAboutToExit(this, Long.valueOf(code).intValue());
        }
    }

    private boolean notifiedExit = false;
    private void eventOnExit(long code) {
        exitCode = code;
        if (!notifiedExit) {
            notifiedExit = true;
            for (EventListener listener : listeners) {
                listener.onProcessExit(this, Long.valueOf(code).intValue());
            }
            if (fs != null) fs.cleanUp();
            fs = null;
        }
    }

    private long exitCode;

    protected WeakReference<ProcessContext> jscontext = null;
    private boolean isActive = false;
    private boolean isDone = false;
    private FileSystem fs = null;

    private ArrayList<EventListener> listeners = new ArrayList<>();

    @SuppressWarnings("unused") // called from native code
    private void onNodeStarted(final long mainContext, long ctxGroupRef) {
        final ProcessContext ctx = new ProcessContext(mainContext, new JSContextGroup(ctxGroupRef));
        jscontext = new WeakReference<>(ctx);
        isActive = true;
        ctx.property("__nodedroid_onLoad", new JSFunction(ctx, "__nodedroid_onLoad") {
            @SuppressWarnings("unused")
            public void __nodedroid_onLoad() {
                if (isActive()) {
                    jscontext.get().deleteProperty("__nodedroid_onLoad");

                    // set file system
                    fs = new FileSystem(ctx, androidCtx, uniqueID, mediaAccessMask);
                    setFileSystem(mainContext, fs.valueRef());

                    // set exit handler
                    JSFunction onExit = new JSFunction(context, "onExit") {
                        @SuppressWarnings("unused")
                        public void onExit(int code) {
                            eventOnAboutToExit(code);
                        }
                    };
                    new JSFunction(context, "__onExit", new String[]{"exitFunc"},
                            "process.on('exit',exitFunc);", null, 0).call(null, onExit);

                    // set unhandled exception handler
                    JSFunction onUncaughtException =
                            new JSFunction(context, "onUncaughtException") {
                        @SuppressWarnings("unused")
                        public void onUncaughtException(JSObject error) {
                            android.util.Log.d("Unhandled", "There is an unhandled exception!");
                            context.evaluateScript("process.exit(process.exitCode || -1)");
                        }
                    };
                    new JSFunction(context, "__onUncaughtException", new String[]{"handleFunc"},
                            "process.on('uncaughtException',handleFunc);",
                                    null, 0).call(null, onUncaughtException);

                    // intercept stdout and stderr
                    JSObject stdout =
                            ctx.property("process").toObject().property("stdout").toObject();
                    stdout.property("write", new JSFunction(stdout.getContext(), "write") {
                        @SuppressWarnings("unused")
                        public void write(String string) {
                            android.util.Log.i("stdout", string);
                        }
                    });

                    JSObject stderr =
                            ctx.property("process").toObject().property("stderr").toObject();
                    stderr.property("write", new JSFunction(stderr.getContext(), "write") {
                        @SuppressWarnings("unused")
                        public void write(String string) {
                            android.util.Log.e("stderr", string);
                        }
                    });

                    // Ready to start
                    eventOnStart(jscontext.get());
                }
            }
        });
    }

    @SuppressWarnings("unused") // called from native code
    private void onNodeExit(long exitCode) {
        isActive = false;
        if (jscontext != null && jscontext.get() != null) {
            ProcessContext ctx = jscontext.get();
            if (ctx != null) {
                ctx.setDefunct();
            }
        }
        jscontext = null;
        isDone = true;
        eventOnExit(exitCode);
        (new Thread() {
            @Override
            public void run() {
                dispose(processRef);
            }
        }).start();
    }

    private final long processRef;
    private final String uniqueID;
    private final Context androidCtx;
    private final int mediaAccessMask;

    /* Ensure the shared libraries get loaded first */
    static {
        JSContext.dummy();
    }

    /**
     * ProcessContext class -- subclasses a JSContext tied to a node.js process
     */
    private class ProcessContext extends JSContext {
        ProcessContext(long contextRef, JSContextGroup group) {
            super(contextRef, group);
        }

        void setDefunct() {
            isDefunct = true;
        }

        void keepAlive() {
            if (handleRef == 0 && !isDefunct) {
                handleRef = Process.this.keepAlive(ctxRef());
            }
        }

        public void async(Runnable runnable) {
            super.async(runnable);
        }

        void letDie() {
            if (handleRef != 0 && !isDefunct) {
                Process.this.letDie(handleRef);
                handleRef = 0L;
            }
        }

        @Override
        public void finalize() throws Throwable {
            exit(kContextFinalizedButProcessStillActive);
            eventOnExit(exitCode);
            super.finalize();
            isActive = false;
        }

        private long handleRef = 0L;
    }

    /* Native JNI functions */
    private native long start();
    private native void dispose(long processRef);
    private native long keepAlive(long contextRef);
    private native void letDie(long handleRef);
    private native long setFileSystem(long contextRef, long fsObject);
}
