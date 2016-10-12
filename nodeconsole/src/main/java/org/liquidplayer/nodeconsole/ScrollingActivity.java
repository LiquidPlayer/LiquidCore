package org.liquidplayer.nodeconsole;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.widget.NestedScrollView;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.Html;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import org.liquidplayer.node.Process;
import org.liquidplayer.v8.JSContext;
import org.liquidplayer.v8.JSException;
import org.liquidplayer.v8.JSFunction;
import org.liquidplayer.v8.JSValue;

import java.util.ArrayList;
import java.util.List;

public class ScrollingActivity extends AppCompatActivity
        implements Process.EventListener, JSContext.IJSExceptionHandler {

    private TextView textView;
    private EditText inputBox;
    private NestedScrollView scrollView;
    private ImageButton upHistory;
    private ImageButton downHistory;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_scrolling);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        new Process(this,"node_console",Process.kMediaAccessPermissionsRW,this);

        textView = (TextView) findViewById(R.id.console_text);
        inputBox = (EditText) findViewById(R.id.inputBox);
        scrollView = (NestedScrollView) findViewById(R.id.scroll);

        inputBox.setFocusableInTouchMode(true);
        inputBox.requestFocus();
        inputBox.setOnEditorActionListener(new EditText.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                android.util.Log.d("action", "something happened id = " + actionId);
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    enterCommand();
                    return true;
                }
                return false;
            }
        });
        inputBox.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View view, int keyCode, KeyEvent event) {
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_ENTER)) {
                    enterCommand();
                    return true;
                }
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_DPAD_UP)) {
                    upHistory();
                    return true;
                }
                if ((event.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_DPAD_DOWN)) {
                    downHistory();
                    return true;
                }
                return false;
            }
        });
        inputBox.setEnabled(false);

        upHistory = (ImageButton) findViewById(R.id.up_history);
        setButtonEnabled(upHistory,false);
        downHistory = (ImageButton) findViewById(R.id.down_history);
        setButtonEnabled(downHistory,false);

        upHistory.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                upHistory();
            }
        });
        downHistory.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                downHistory();
            }
        });

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            println(Html.fromHtml("", Html.FROM_HTML_MODE_LEGACY));
        } else {
            //noinspection deprecation
            println(Html.fromHtml(""));
        }
    }

    private void setButtonEnabled(ImageButton button, boolean enable) {
        button.setEnabled(enable);
        button.setAlpha(enable?1f : 0.5f);
        button.setClickable(enable);
    }

    private void upHistory() {
        if (upHistory.isEnabled()) {
            inputBox.setText(history.get(--item));
            if (item == 0) {
                setButtonEnabled(upHistory, false);
            }
            setButtonEnabled(downHistory, true);
            inputBox.setSelection(inputBox.getText().length());
        }
    }

    private void downHistory() {
        if (downHistory.isEnabled()) {
            if (++item >= history.size()) {
                setButtonEnabled(downHistory, false);
                inputBox.setText("");
            } else {
                inputBox.setText(history.get(item));
            }
            setButtonEnabled(upHistory, true);
            inputBox.setSelection(inputBox.getText().length());
        }
    }

    private void enterCommand() {
        processedException = false;
        final String cmd = inputBox.getText().toString();

        Spanned text;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            text = Html.fromHtml(
                    "<strong><font color=\"black\">&gt; " + cmd + "</font></strong>",
                    Html.FROM_HTML_MODE_LEGACY);
        } else {
            //noinspection deprecation
            text = Html.fromHtml(
                    "<strong><font color=\"black\">&gt; " + cmd + "</font></strong>");
        }

        println(text);
        inputBox.setText("");
        history.add(cmd);
        item = history.size();
        setButtonEnabled(upHistory,true);
        setButtonEnabled(downHistory,false);
        /*
        InputMethodManager imm =
                (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
        imm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
        */
        new Thread(new Runnable() {
            @Override
            public void run() {
                JSValue output = js.evaluateScript(cmd);
                if (!processedException) {
                    console_log.call(null, output);
                }
            }
        }).start();
    }

    private JSContext js = null;
    private JSFunction console_log = null;
    private List<String> history = new ArrayList<>();
    private int item = 0;

    private void println(final CharSequence string) {
        print(string,true);
    }
    private void print(final CharSequence string) {
        print(string,false);
    }
    private void print(final CharSequence string, final boolean addCr) {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
                    textView.append(addCr ?
                        TextUtils.concat(string,Html.fromHtml("<br>",Html.FROM_HTML_MODE_LEGACY)) :
                            string);
                } else {
                    //noinspection deprecation
                    textView.append(addCr ? TextUtils.concat(string,Html.fromHtml("<br>")) : string);
                }

                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        View lastChild = scrollView.getChildAt(scrollView.getChildCount() - 1);
                        int bottom = lastChild.getBottom() + scrollView.getPaddingBottom();
                        int sy = scrollView.getScrollY();
                        int sh = scrollView.getHeight();
                        int delta = bottom - (sy + sh);

                        scrollView.smoothScrollBy(0, delta);
                    }
                });
            }
        });
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
    }

    @Override
    public void onProcessAboutToExit(Process process, int exitCode) {
        console_log = null;
        js = null;
    }

    @Override
    public void onProcessExit(Process process, int exitCode) {

    }

    @Override
    public void onProcessFailed(Process process, Exception error) {

    }

    @Override
    public void onStdout(Process process, final String string) {
        CharSequence text = "";
        String [] str = string.split("\\n");
        for (String f : str) {
            Spanned text_;
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
                text_ = Html.fromHtml(Html.escapeHtml(f) + "<br>",
                        Html.FROM_HTML_MODE_LEGACY);
            } else {
                //noinspection deprecation
                text_ = Html.fromHtml(Html.escapeHtml(f) + "<br>");
            }
            text = TextUtils.concat(text,text_);
        }
        print(text);
    }

    @Override
    public void onStderr(Process process, final String string) {
        CharSequence text = "";
        String [] str = string.split("\\n");
        for (String f : str) {
            Spanned text_;
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
                text_ = Html.fromHtml("<font color=\"red\">" + Html.escapeHtml(f) + "<br></font>",
                        Html.FROM_HTML_MODE_LEGACY);
            } else {
                //noinspection deprecation
                text_ = Html.fromHtml("<font color=\"red\">" + Html.escapeHtml(f) + "<br></font>");
            }
            text = TextUtils.concat(text,text_);
        }
        print(text);
    }

    boolean processedException = false;

    @Override
    public void handle(final JSException e) {
        processedException = true;
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                CharSequence text = "";
                String [] str = e.stack().split("\\n");
                for (String f : str) {
                    Spanned text_;
                    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
                        text_ = Html.fromHtml("<font color=\"red\">" + Html.escapeHtml(f) + "<br></font>",
                                Html.FROM_HTML_MODE_LEGACY);
                    } else {
                        //noinspection deprecation
                        text_ = Html.fromHtml("<font color=\"red\">" + Html.escapeHtml(f) + "<br></font>");
                    }
                    text = TextUtils.concat(text,text_);
                }
                print(text);
            }
        });
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_scrolling, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
