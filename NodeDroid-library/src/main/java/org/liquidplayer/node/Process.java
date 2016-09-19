package org.liquidplayer.node;

import org.liquidplayer.v8.JSContext;
import org.liquidplayer.v8.JSContextGroup;
import org.liquidplayer.v8.JSFunction;

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

@SuppressWarnings("JniMissingFunction")
public class Process {

    static {
        JSContext.dummy();
    }

    private class ProcessContext extends JSContext {
        public ProcessContext(long contextRef, JSContextGroup group) {
            super(contextRef, group);
        }

        public void setDefunct() {
            isDefunct = true;
        }

        public void keepAlive() {
            if (handleRef == 0 && !isDefunct) {
                handleRef = Process.this.keepAlive(ctxRef());
            }
        }

        public void letDie() {
            if (handleRef != 0 && !isDefunct) {
                Process.this.letDie(handleRef);
            }
        }

        private long handleRef = 0L;
    }

    public interface EventListener {
        void onProcessStart(final Process process, final JSContext context);
        void onProcessExit(final Process process, int exitCode);
        void onProcessFailed(final Process process, Exception error);
    }

    public Process(EventListener listener, String [] fsDirWhitelist) {
        addEventListener(listener);
        processRef = start();
    }
    public Process(EventListener listener) {
        this(listener,null);
    }

    public void addEventListener(EventListener listener) {
        if (!listeners.contains(listener)) {
            listeners.add(listener);
        }
        if (context != null) {
            listener.onProcessStart(this, context);
        }
    }

    public void removeEventListener(EventListener listener) {
        listeners.remove(listener);
    }

    public boolean isActive() {
        return isActive;
    }

    public void kill() {
        if (isActive()) {
        }
    }

    public void keepAlive() {
        if (context != null) context.keepAlive();
    }

    public void letDie() {
        if (context != null) context.letDie();
    }

    protected void eventOnStart() {
        for (EventListener listener : listeners) {
            listener.onProcessStart(this, context);
        }
    }
    protected void eventOnExit(long code) {
        for (EventListener listener : listeners) {
            listener.onProcessExit(this, Long.valueOf(code).intValue());
        }
    }

    protected ProcessContext context = null;
    protected boolean isActive = false;

    private ArrayList<EventListener> listeners = new ArrayList<>();

    @SuppressWarnings("unused") // called from native code
    private void onNodeStarted(long mainContext, long ctxGroupRef) {
        context = new ProcessContext(mainContext, new JSContextGroup(ctxGroupRef));
        isActive = true;
        context.property("__nodedroid_onLoad", new JSFunction(context, "__nodedroid_onLoad") {
            @SuppressWarnings("unused")
            public void __nodedroid_onLoad() {
                context.deleteProperty("__nodedroid_onLoad");
                eventOnStart();
            }
        });
    }

    @SuppressWarnings("unused") // called from native code
    private void onNodeExit(long exitCode) {
        context.setDefunct();
        context = null;
        isActive = false;
        eventOnExit(exitCode);
        (new Thread() {
            @Override
            public void run() {
                dispose(processRef);
                processRef = 0L;
            }
        }).start();
    }

    @SuppressWarnings("unused") // called from native code
    private void onStdout(byte [] chars) {
        android.util.Log.d("Node stdout", new String(chars, StandardCharsets.UTF_8));
    }

    private long processRef = 0L;

    private native long start();
    private native void dispose(long processRef);
    private native long keepAlive(long contextRef);
    private native void letDie(long handleRef);
}
