package org.liquidplayer.node;

import android.content.Context;
import android.os.Build;
import android.os.Environment;

import org.liquidplayer.v8.JSArray;
import org.liquidplayer.v8.JSArrayBuffer;
import org.liquidplayer.v8.JSContext;
import org.liquidplayer.v8.JSContextGroup;
import org.liquidplayer.v8.JSFunction;
import org.liquidplayer.v8.JSObject;
import org.liquidplayer.v8.JSUint8Array;

import java.io.File;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

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

    /** -- private methods -- **/

    private synchronized void eventOnStart(JSContext ctx) {
        for (EventListener listener : listeners) {
            listener.onProcessStart(this, ctx);
        }
    }
    private synchronized void eventOnAboutToExit(long code) {
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
                    fs = new FileSystem(ctx, mediaAccessMask);
                    setFileSystem(mainContext, fs.valueRef());

                    // set exit handler
                    JSFunction onExit = new JSFunction(context, "onExit") {
                        @SuppressWarnings("unused")
                        public void onExit(int code) {
                            eventOnAboutToExit(code);
                        }
                    };
                    new JSFunction(context, "__onExit", new String[]{"exitFunc"},
                            "process.on('exit',exitFunc);" +
                                    "process.chdir('/home');", null, 0).call(null, onExit);

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

    /**
     * FileSystem class
     */

    /**
     * This creates a JavaScript object that is used by nodedroid_file.cc in the native code
     * to control access to the file system.  We will create an alias filesystem with the
     * following structure
     *
     * /
     * /cache
     * /home
     * /external
     *        /cache
     *        /home
     * /media [if permissions set]
     *        /Pictures
     *        /Movies
     *        /Downloads
     *        /... (android standard)
     *
     * Everything else will result in a ENOACES (access denied) error
     */
    private class FileSystem extends JSObject {

        @jsexport(attributes = JSPropertyAttributeReadOnly)
        private Property<JSObject> access_;

        @jsexport(attributes = JSPropertyAttributeReadOnly)
        private Property<JSObject> aliases_;

        @jsexport(attributes = JSPropertyAttributeReadOnly)
        private Property<JSFunction> fs;

        @jsexport(attributes = JSPropertyAttributeReadOnly)
        private Property<JSFunction> alias;

        private String realDir(String dir) {
            return getContext().evaluateScript(
                    "(function(){return require('fs').realpathSync('" + dir + "');})()"
            ).toString();
        }
        private String mkdir(String dir) {
            new File(dir).mkdirs();
            return realDir(dir);
        }
        private List<String> toclean = new ArrayList<>();
        private void symlink(String target, String linkpath) {
            getContext().evaluateScript(
                    "(function(){require('fs').symlinkSync('" + target + "','" + linkpath +"');})()"
            );
        }
        private boolean isSymlink(File file) {
            try {
                return file.getCanonicalFile().equals(file.getAbsoluteFile());
            } catch (IOException e) {
                return true;
            }
        }
        private void deleteRecursive(File fileOrDirectory) {
            if (fileOrDirectory.isDirectory() && !isSymlink(fileOrDirectory))
                for (File child : fileOrDirectory.listFiles())
                    deleteRecursive(child);

            if (!fileOrDirectory.delete()) {
                android.util.Log.d("nodedroid", "Failed to delete directory");
            }
        }

        private void setUp() {
            final String suffix = "/__org.liquidplayer.node__/_" + uniqueID;
            String random = "" + UUID.randomUUID();
            String sessionSuffix = suffix + "/" + random;

            String home  = mkdir(androidCtx.getFilesDir().getAbsolutePath() +
                    sessionSuffix + "/home");
            toclean.add(home);
            aliases_.get().property("/home", home);
            access_ .get().property("/home", kMediaAccessPermissionsRW);

            String cache = mkdir(androidCtx.getCacheDir().getAbsolutePath() +
                    sessionSuffix + "/cache");
            toclean.add(cache);
            symlink(cache, home + "/cache");
            aliases_.get().property("/home/cache", cache);

            String persistent = mkdir(androidCtx.getFilesDir().getAbsolutePath() +
                    suffix + "/persistent");
            symlink(persistent, home + "/persistent");
            aliases_.get().property("/home/persistent", persistent);

            new File(home + "/external").mkdirs();
            if (androidCtx.getExternalCacheDir()!=null) {
                String externalCache = mkdir(androidCtx.getExternalCacheDir().getAbsolutePath() +
                        sessionSuffix + "/cache");
                symlink(externalCache, home + "/external/cache");
                toclean.add(externalCache);
                aliases_.get().property("/home/external/cache", externalCache);
            } else {
                android.util.Log.d("setUp", "No external cache dir");
            }

            File external = androidCtx.getExternalFilesDir(null);
            if (external!=null) {
                String externalPersistent = mkdir(external.getAbsolutePath() +
                        "/LiquidPlayer/" + uniqueID);
                symlink(externalPersistent, home + "/external/persistent");
                aliases_.get().property("/home/external/persistent", externalPersistent);
            }

            external = androidCtx.getExternalFilesDir(Environment.DIRECTORY_PICTURES);
            androidCtx.getExternalFilesDir(Environment.DIRECTORY_DCIM);
            androidCtx.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
            androidCtx.getExternalFilesDir(Environment.DIRECTORY_MOVIES);
            androidCtx.getExternalFilesDir(Environment.DIRECTORY_MUSIC);
            androidCtx.getExternalFilesDir(Environment.DIRECTORY_NOTIFICATIONS);
            androidCtx.getExternalFilesDir(Environment.DIRECTORY_PODCASTS);
            androidCtx.getExternalFilesDir(Environment.DIRECTORY_RINGTONES);
            if (external!=null) {
                String media = realDir(external.getAbsolutePath() + "/..");
                symlink(media, home + "/media");
                aliases_.get().property("/home/media", media);
            }
        }

        private void tearDown() {
            for (String dir : toclean) {
                deleteRecursive(new File(dir));
            }
        }

        FileSystem(JSContext ctx, int mediaPermissionsMask) {
            super(ctx);

            aliases_.set(new JSObject(ctx));
            access_ .set(new JSObject(ctx));

            setUp();


            /*
            String state = Environment.getExternalStorageState();
            int access = Environment.MEDIA_MOUNTED_READ_ONLY.equals(state) ?
                    kMediaAccessPermissionsRead :
                    Environment.MEDIA_MOUNTED.equals(state) ? kMediaAccessPermissionsRW : 0;
            access &= mediaPermissionsMask;

            if (access > 0) {
                for (int i = 0; i < media.length; i++) {
                    String localDir = ctx.evaluateScript(
                        "(function(){return require('fs').realpathSync('" + publics[i] + "');})()"
                    ).toString();

                    aliases_.get().property(media[i], localDir);
                    access_.get().property(media[i], access);
                }
            }
            */

            fs.set(new JSFunction(ctx, "fs", ""+
                    "try { file = require('path').resolve(file); } catch (e) {}"+
                    "var access = 0;"+
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
                    "return [access,newfile];",
            "file"));

            alias.set(new JSFunction(ctx, "alias", ""+
                    "for (var p in this.aliases_) {"+
                        "if (file.startsWith(this.aliases_[p] + '/')) {"+
                            "file = p + '/' + file.substring(this.aliases_[p].length + 1);"+
                            "break;"+
                        "} else if (file == this.aliases_[p]) {"+
                            "file = p;"+
                            "break;"+
                        "}"+
                    "}"+
                    "return file;",
            "file"));

        }

        void cleanUp() {
            tearDown();
        }
    }
}
