package org.liquidplayer.node;

import android.content.Context;

import org.liquidplayer.v8.JSContext;
import org.liquidplayer.v8.JSContextGroup;
import org.liquidplayer.v8.JSFunction;
import org.liquidplayer.v8.JSObject;

import java.io.File;
import java.lang.ref.WeakReference;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

@SuppressWarnings("JniMissingFunction")
class Process {

    final public static int kContextFinalizedButProcessStillActive = -222;

    /**
     * Clients must subclass an EventListener to get state change information from the
     * node.js process
     */
    interface EventListener {
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
    Process(Context androidContext, String uniqueID, EventListener listener) {
        addEventListener(listener);
        processRef = start();
        androidCtx = androidContext;
        this.uniqueID = uniqueID;
    }

    /**
     * Adds an EventListener to this Process
     * @param listener the listener interface object
     */
    void addEventListener(final EventListener listener) {
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
    public void removeEventListener(EventListener listener) {
        listeners.remove(listener);
    }

    /**
     * Determines if the process is currently active.  If it is inactive, either it hasn't
     * yet been started, or the process completed. Use an @EventListener to determine the
     * state.
     * @return true if active, false otherwise
     */
    boolean isActive() {
        return isActive && jscontext.get() != null;
    }

    /**
     * Instructs the VM to halt execution as quickly as possible
     * @param exitc The exit code
     */
    void exit(final int exitc) {
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
    void keepAlive() {
        if (isActive()) {
            jscontext.get().keepAlive();
        }
    }

    /**
     * Instructs the VM to not keep itself alive if no more callbacks are pending.
     */
    void letDie() {
        if (isActive()) {
            jscontext.get().letDie();
        }
    }

    /** -- private methods -- **/

    private void eventOnStart(JSContext ctx) {
        for (EventListener listener : listeners) {
            listener.onProcessStart(this, ctx);
        }
    }
    private void eventOnAboutToExit(long code) {
        exitCode = code;
        for (EventListener listener : listeners) {
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
        }
    }

    private long exitCode;

    protected WeakReference<ProcessContext> jscontext = null;
    private boolean isActive = false;
    private boolean isDone = false;

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
                    // set exit handler
                    JSFunction onExit = new JSFunction(context, "onExit") {
                        @SuppressWarnings("unused")
                        public void onExit(int code) {
                            eventOnAboutToExit(code);
                        }
                    };
                    FileSystem fs = new FileSystem(ctx);
                    setFileSystem(mainContext, fs.valueRef());
                    new JSFunction(context, "__onExit", new String[]{"exitFunc"},
                            "process.on('exit',exitFunc);" +
                                    "process.chdir('/home');", null, 0).call(null, onExit);
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

    @SuppressWarnings("unused") // called from native code
    private void onStdout(byte [] chars) {
    }

    private final long processRef;
    private final String uniqueID;
    private final Context androidCtx;

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

    /**
     * FileSystem class
     */
    private class FileSystem extends JSObject {
        final private String script = "" +
                "try { file = require('path').resolve(file); } catch (e) {}"+
                "var access = 3;"+
                "for (var p in this.aliases_) {"+
                    "if (file.startsWith(this.aliases_[p] + '/')) {"+
                        "file = p + '/' + file.substring(this.aliases_[p].length + 1);"+
                        "break;"+
                    "} else if (file == this.aliases_[p]) {"+
                        "file = p;"+
                        "break;"+
                    "}"+
                "}"+
                "for (var p in this.access_) {"+
                    "if (file.startsWith(p + '/') || p==file) {"+
                        "access = this.access_[p];"+
                        "break;"+
                    "}"+
                "}"+
                "var newfile = file;"+
                "for (var p in this.aliases_) {"+
                    "if (file.startsWith(p + '/')) {"+
                        "newfile = this.aliases_[p] + '/' + file.substring(p.length + 1);"+
                        "break;"+
                    "} else if (file == p) {"+
                        "newfile = this.aliases_[p];"+
                        "break;"+
                    "}"+
                "}"+
                "return [access,newfile];";

        final private String toAliasScript = "" +
                "for (var p in this.aliases_) {"+
                    "if (file.startsWith(this.aliases_[p] + '/')) {"+
                        "file = p + '/' + file.substring(this.aliases_[p].length + 1);"+
                        "break;"+
                    "} else if (file == this.aliases_[p]) {"+
                        "file = p;"+
                        "break;"+
                    "}"+
                "}"+
                "return file;";

        FileSystem(JSContext ctx) {
            super(ctx);
            access_  = new JSObject(ctx);
            aliases_ = new JSObject(ctx);
            property("fs", new JSFunction(ctx, "fs", new String[]{"file"}, script, null, 0),
                    JSPropertyAttributeReadOnly);
            property("alias", new JSFunction(ctx, "alias", new String[]{"file"},
                    toAliasScript, null, 0), JSPropertyAttributeReadOnly);
            property("access_", access_,JSPropertyAttributeReadOnly);
            property("aliases_",aliases_,JSPropertyAttributeReadOnly);

            // Create a home directory based on the unique ID
            String lDir = androidCtx.getFilesDir().getAbsolutePath() +
                    "/__org.liquidplayer.node__/_" + uniqueID;
            // Get the real directory (with resolved symlinks)
            new File(lDir).mkdirs();
            localDir = ctx.evaluateScript(
                    "(function(){return require('fs').realpathSync('"+lDir+"');})()"
            ).toString();

            aliases_.property("/home", localDir);

            // Disable access to the rest of the app's directory
            access_.property(localDir, 0);
            access_.property("/data", 0);

            // We can also manage access to other assets, like photos, etc. here
        }

        final private String localDir;

        private final JSObject access_;
        private final JSObject aliases_;
    }
}
