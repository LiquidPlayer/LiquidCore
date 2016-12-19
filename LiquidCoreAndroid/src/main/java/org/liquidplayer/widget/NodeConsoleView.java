package org.liquidplayer.widget;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Looper;
import android.support.v4.content.LocalBroadcastManager;
import android.util.AttributeSet;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSException;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.NodeProcessService;
import org.liquidplayer.node.Process;

import java.util.UUID;

public class NodeConsoleView extends ConsoleView
    implements Process.EventListener, JSContext.IJSExceptionHandler {

    public interface Listener {
        void onJSReady(JSContext ctx);
    }

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
    private Listener listener;

    public void setListener(Listener listener) {
        this.listener = listener;
        if (state().getString("uuid") == null) {
            String uuid = UUID.randomUUID().toString();
            state().putString("uuid", uuid);
        }
        resize(columns, rows);
    }

    @Override
    public void reset() {
        if (process != null) {
            process.removeEventListener(this);
            process = null;
        }
        super.reset();
    }

    @Override
    protected void resize(int columns, int rows) {
        this.columns = columns;
        this.rows = rows;
        if (state().getString("uuid") != null) {
            Context context = getContext();
            LocalBroadcastManager.getInstance(context).registerReceiver(
                    new BroadcastReceiver() {
                        @Override
                        public void onReceive(Context context, Intent intent) {
                            process = NodeProcessService.getProcess(state().getString("uuid"));
                            process.addEventListener(NodeConsoleView.this);
                            LocalBroadcastManager.getInstance(context).unregisterReceiver(this);
                        }
                    },
                    new IntentFilter(state().getString("uuid")));

            Intent serviceIntent = new Intent(context, NodeProcessService.class);
            serviceIntent.putExtra("org.liquidplayer.node.Process", state().getString("uuid"));
            context.startService(serviceIntent);
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

    @Override
    public void onProcessStart(Process process, JSContext context) {
        js = context;
        js.setExceptionHandler(this);
        console_log = js.property("console").toObject().property("log").toFunction();
        process.keepAlive();
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

        if (listener != null) listener.onJSReady(context);
    }

    @Override
    public void onProcessAboutToExit(Process process, int exitCode) {
        JSObject stdout =
                js.property("process").toObject().property("stdout").toObject();
        JSObject stderr =
                js.property("process").toObject().property("stderr").toObject();
        JSFunction writenull = new JSFunction(js, "_writenull", "");
        stdout.property("write", writenull);
        stderr.property("write", writenull);
        js.property("process").toObject().deleteProperty("clearScreenDown");
        js.property("process").toObject().deleteProperty("moveCursor");

        console_log = null;
        js = null;
        process.letDie();
        this.process = null;
        consoleTextView.println("\u001B[31mProcess about to exit with code " + exitCode);
        android.util.Log.e("onProcessAboutToExit", "Process about to exit with code " + exitCode);
        process.removeEventListener(this);
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                android.util.Log.d("looper", "setting stuff");
                inputBox.setEnabled(false);
                inputBox.setClickable(false);
                inputBox.setAlpha(0.5f);
                setButtonEnabled(downHistory,false);
                setButtonEnabled(upHistory,false);
                android.util.Log.d("looper", "done setting stuff");
            }
        });
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
