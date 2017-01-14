package org.liquidplayer.widget;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSException;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.NodeProcessService;
import org.liquidplayer.node.Process;

public class NodeConsoleView extends ConsoleView
    implements Process.EventListener, JSContext.IJSExceptionHandler {

    public NodeConsoleView(Context context) {
        this(context, null);
    }

    public NodeConsoleView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public NodeConsoleView(Context context, AttributeSet attrs, int defStyle) {
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

    public void attach(final String nodeProcessId) {
        do_attach(nodeProcessId);
        resize(columns, rows);
    }

    private void do_attach(final String nodeProcessId) {
        if (state().getString("uuid") != null && !state().getString("uuid").equals(nodeProcessId)) {
            detach();
        }
        state().putString("uuid", nodeProcessId);
        process = NodeProcessService.getProcess(nodeProcessId);
        process.addEventListener(NodeConsoleView.this);
    }

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
        android.util.Log.e("onProcessAboutToExit", "Process about to exit with code " + exitCode);
        detach();
    }

    @Override
    public void onProcessExit(Process process, int exitCode) {
        android.util.Log.d("onProcessExit", "exiting");
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
