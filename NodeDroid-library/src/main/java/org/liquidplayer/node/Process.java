package org.liquidplayer.node;

import org.liquidplayer.v8.JSContext;
import org.liquidplayer.v8.JSContextGroup;

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

    }

    protected void eventOnStart() {
        isActive = true;
        for (EventListener listener : listeners) {
            listener.onProcessStart(this, context);
        }
    }
    protected void eventOnExit(long code) {
        isActive = false;
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
        eventOnStart();
    }

    @SuppressWarnings("unused") // called from native code
    private void onNodeExit(long exitCode) {
        context.setDefunct();
        context = null;
        isActive = false;
        processRef = 0L;
        eventOnExit(exitCode);
    }

    private static boolean libsLoaded = false;
    private long processRef = 0L;

    private native long start();
}
