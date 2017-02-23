//
// ConsoleView.java
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
import android.graphics.Paint;
import android.os.Handler;
import android.os.Looper;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.v4.widget.NestedScrollView;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

class ConsoleView extends RelativeLayout implements AnsiConsoleTextView.Listener {
    public ConsoleView(Context context) {
        this(context, null);
    }

    public ConsoleView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ConsoleView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        LayoutInflater.from(context).inflate(R.layout.console_view, this);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        loadViews();
    }

    protected void resize(int columns, int rows) {
    }

    protected void processCommand(String command) {
    }

    protected EditText inputBox;
    protected NestedScrollView scrollView;
    protected ImageButton upHistory;
    protected ImageButton downHistory;
    protected AnsiConsoleTextView consoleTextView;

    private void setInputBoxText(final CharSequence text) {
        uiThread.post(new Runnable() {
            @Override
            public void run() {
                inputBox.setText(text);
            }
        });
    }
    private void setConsoleText(final CharSequence text) {
        uiThread.post(new Runnable() {
            @Override
            public void run() {
                consoleTextView.setDisplayText(text);
            }
        });
    }
    public void reset() {
        if (getId() != View.NO_ID) {
            history.clear();
            item = 0;
            setInputBoxText("");
            setConsoleText("");
        }
    }

    private EditText.OnEditorActionListener onEditorAction = new TextView.OnEditorActionListener() {
        @Override
        public boolean onEditorAction(TextView textView, int actionId, KeyEvent keyEvent) {
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                enterCommand();
                return true;
            }
            return false;
        }
    };

    private View.OnKeyListener onKeyListener = new OnKeyListener() {
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
    };

    private View.OnClickListener onUpClick = new OnClickListener() {
        @Override
        public void onClick(View view) {
            upHistory();
        }
    };

    private View.OnClickListener onDownClick = new OnClickListener() {
        @Override
        public void onClick(View view) {
            downHistory();
        }
    };

    private ViewTreeObserver.OnGlobalLayoutListener onLayout =
        new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                //scrollView.getViewTreeObserver().removeOnGlobalLayoutListener(this);

                Paint textPaint = consoleTextView.getPaint();
                int width = Math.round(textPaint.measureText("X") * consoleTextView.getTextScaleX());
                int height = Math.round(textPaint.getTextSize() * consoleTextView.getTextScaleX());
                int rows = scrollView.getMeasuredHeight() / height;
                int columns = scrollView.getMeasuredWidth() / width;

                resize(columns,rows);
            }
    };

    private Runnable scrollToBottom = new Runnable() {
        @Override
        public void run() {
            onDisplayUpdated();
        }
    };

    private Handler uiThread = new Handler(Looper.getMainLooper());

    private void loadViews() {
        inputBox = (EditText) findViewById(R.id.inputBox);
        scrollView = (NestedScrollView) findViewById(R.id.scroll);

        inputBox.setFocusableInTouchMode(true);
        inputBox.requestFocus();
        inputBox.setOnEditorActionListener(onEditorAction);
        inputBox.setOnKeyListener(onKeyListener);
        inputBox.setEnabled(false);

        upHistory = (ImageButton) findViewById(R.id.up_history);
        downHistory = (ImageButton) findViewById(R.id.down_history);

        upHistory.setOnClickListener(onUpClick);
        downHistory.setOnClickListener(onDownClick);
        setButtonEnabled(upHistory,false);
        setButtonEnabled(downHistory,false);

        ViewTreeObserver vto = scrollView.getViewTreeObserver();
        vto.addOnGlobalLayoutListener(onLayout);

        consoleTextView = (AnsiConsoleTextView) findViewById(R.id.console_text);
        consoleTextView.addListener(this);
        // FIXME: Don't forget to removeListener on shutdown

        uiThread.post(scrollToBottom);
    }

    @Override
    public void onDisplayUpdated() {
        View lastChild = scrollView.getChildAt(scrollView.getChildCount() - 1);
        int bottom = lastChild.getBottom() + scrollView.getPaddingBottom();
        int sy = scrollView.getScrollY();
        int sh = scrollView.getHeight();
        int delta = bottom - (sy + sh);

        scrollView.smoothScrollBy(0, delta);
    }

    protected void setButtonEnabled(ImageButton button, boolean enable) {
        button.setEnabled(enable);
        button.setAlpha(enable?1f : 0.5f);
        button.setClickable(enable);
    }

    private void upHistory() {
        if (upHistory.isEnabled()) {
            item--;
            setInputBoxText(history.get(item));
            if (item == 0) {
                setButtonEnabled(upHistory, false);
            }
            setButtonEnabled(downHistory, true);
            inputBox.setSelection(inputBox.getText().length());
        }
    }

    private void downHistory() {
        if (downHistory.isEnabled()) {
            item++;
            if (item >= history.size()) {
                setButtonEnabled(downHistory, false);
                setInputBoxText("");
            } else {
                setInputBoxText(history.get(item));
            }
            setButtonEnabled(upHistory, true);
            inputBox.setSelection(inputBox.getText().length());
        }
    }

    private void enterCommand() {
        final String cmd = inputBox.getText().toString();

        consoleTextView.println("\u001b[30;1m> " + cmd);
        setInputBoxText("");
        history.add(cmd);
        item = history.size();
        setButtonEnabled(upHistory,true);
        setButtonEnabled(downHistory,false);

        processCommand(cmd);
    }

    /* -- parcelable privates -- */
    private int item = 0;
    private ArrayList<String> history = new ArrayList<>();

    @Override
    protected Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedState ss = new SavedState(superState);
        ss.textView = consoleTextView.getText();
        if (ss.inputBox != null)
            setInputBoxText(ss.inputBox);
        ss.item = item;
        ss.history = history;
        return ss;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        SavedState ss = (SavedState) state;
        super.onRestoreInstanceState(ss.getSuperState());
        setConsoleText(ss.textView);
        inputBox.setText(ss.inputBox);
        item = ss.item;
        history = new ArrayList<>(ss.history);
    }

    static class SavedState extends BaseSavedState {
        private CharSequence textView;
        private CharSequence inputBox;
        private int item;
        private List<String> history;

        SavedState(Parcelable superState) {
            super(superState);
        }

        private SavedState(Parcel in) {
            super(in);
            textView = TextUtils.CHAR_SEQUENCE_CREATOR.createFromParcel(in);
            inputBox = TextUtils.CHAR_SEQUENCE_CREATOR.createFromParcel(in);
            item = in.readInt();
            in.readStringList(history);
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            TextUtils.writeToParcel(textView,out,flags);
            TextUtils.writeToParcel(inputBox,out,flags);
            out.writeInt(item);
            out.writeStringList(history);
        }

        public static final Parcelable.Creator<SavedState> CREATOR
                = new Parcelable.Creator<SavedState>() {
            public SavedState createFromParcel(Parcel in) {
                return new SavedState(in);
            }

            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
    }

}
