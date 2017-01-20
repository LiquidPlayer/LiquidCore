//
// AnsiConsoleTextView.java
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
import android.text.Html;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.widget.TextView;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;

class AnsiConsoleTextView extends TextView {
    public AnsiConsoleTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
        stream = new ConsoleOutputStream(new ByteArrayOutputStream());
    }

    public void print(final String string) {
        stream.print(string, false);
    }
    public void println(final String string) {
        stream.print(string, true);
    }

    interface Listener {
        void onDisplayUpdated();
    }

    private ArrayList<Listener> listeners = new ArrayList<>();

    public void addListener(Listener listener) {
        if (!listeners.contains(listener))
            listeners.add(listener);
    }

    public void removeListener(Listener listener) {
        listeners.remove(listener);
    }

    private final ConsoleOutputStream stream;

    private class ConsoleOutputStream extends HtmlAnsiOutputStream {

        ConsoleOutputStream(ByteArrayOutputStream byteArrayOutputStream) {
            super(byteArrayOutputStream);
            os = byteArrayOutputStream;
            displayText = getText();
            index = displayText.length();
            consoleThread = new Thread(consoleThreadRunnable);
            consoleThread.start();
        }

        private ByteArrayOutputStream os;

        private int index;
        private CharSequence displayText;

        private Boolean refreshQueued = false;
        private final Object lock = new Object();

        private Runnable updateText = new Runnable() {
            @Override
            public void run() {
                synchronized (lock) {
                    setText(displayText);
                    refreshQueued = false;
                }
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        for (Listener listener : listeners) {
                            listener.onDisplayUpdated();
                        }
                    }
                });
            }
        };

        private final Object consoleThreadLock = new Object();
        private final Semaphore consoleSemaphore = new Semaphore(0);
        private final ArrayList<String> consoleStrings = new ArrayList<>();

        private final Thread consoleThread;
        /** @noinspection FieldCanBeLocal */
        private final Runnable consoleThreadRunnable = new Runnable() {
            @Override
            public void run() {
                while (true) {
                    try {
                        int strings;
                        String out = "";
                        consoleSemaphore.acquire();
                        synchronized (consoleThreadLock) {
                            strings = consoleStrings.size();
                            for (String string : consoleStrings) {
                                out = out.concat(string);
                            }
                            consoleStrings.clear();
                        }
                        try {
                            write(out.getBytes("UTF-8"));
                            flush();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        if (strings > 1) {
                            consoleSemaphore.acquire(strings - 1);
                        }
                    } catch (InterruptedException e) {
                        break;
                    }
                }
            }
        };

        @Override
        public void close() throws IOException {
            super.close();
            consoleThread.interrupt();
        }

        void print(String string, final boolean addCr) {
            if (addCr) string = string.concat("\n");
            synchronized (consoleThreadLock) {
                consoleStrings.add(string);
                consoleSemaphore.release();
            }
        }

        @Override
        public void flush() throws IOException {
            synchronized (lock) {
                CharSequence text_;

                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
                    text_ = Html.fromHtml(new String(os.toByteArray(), "UTF-8"),
                            Html.FROM_HTML_MODE_LEGACY);
                } else {
                    //noinspection deprecation
                    text_ = Html.fromHtml(new String(os.toByteArray(), "UTF-8"));
                }
                os.reset();

                if (index == displayText.length()) {
                    displayText = TextUtils.concat(displayText, text_);
                } else if (index < displayText.length()) {
                    if (index + text_.length() >= displayText.length()) {
                        displayText = TextUtils.concat(displayText.subSequence(0, index), text_);
                    } else {
                        CharSequence first = displayText.subSequence(0, index);
                        CharSequence last =
                                displayText.subSequence(index+text_.length(), displayText.length());
                        displayText = TextUtils.concat(first, text_, last);
                    }
                } else {
                    android.util.Log.e("Console flush","index is greater than total buffer length");
                    displayText = TextUtils.concat(displayText, text_);
                    index = displayText.length();
                }
                index += text_.length();

                if (!refreshQueued) {
                    refreshQueued = true;
                    new Handler(Looper.getMainLooper()).post(updateText);
                }
            }
        }

        @Override
        protected void processEraseScreen(int eraseOption) throws IOException {
            synchronized (lock) {
                switch(eraseOption) {
                    case ERASE_SCREEN_TO_END:
                        displayText = displayText.subSequence(0,index);
                        break;
                    case ERASE_SCREEN_TO_BEGINING:
                        // FIXME
                        displayText = "";
                        index = 0;
                        break;
                    case ERASE_SCREEN:
                        displayText = "";
                        index = 0;
                        break;
                }
            }
            flush();
        }

        @Override
        protected void processCursorTo(int row, int col) throws IOException {
            synchronized (lock) {
                // Move to row 'row'
                index = 0;
                int count = 1;
                String text = displayText.toString();
                while (count < row && index < text.length()) {
                    int next = text.indexOf('\n', index);
                    if (next < 0) break;
                    count++;
                    index = next + 1;
                }

                // Move to column 'col'
                for (count = 1;
                     count < col && index < text.length() && text.charAt(index) != '\n';
                     count++)
                    index++;
            }
            flush();
        }

        private int[] currentCursorPos() {
            int pos[] = {1,1};
            String text = displayText.toString();

            for (int i=0; i<index; i++) {
                if (text.charAt(i) == '\n') {
                    pos[0] ++;
                    pos[1] = 0;
                } else {
                    pos[1] ++;
                }
            }

            return pos;
        }

        @Override
        protected void processCursorToColumn(int x) throws IOException {
            int pos[] = currentCursorPos();
            processCursorTo(pos[0], Math.max(1,x));
        }

        @Override
        protected void processCursorUpLine(int count) throws IOException {
            int pos[] = currentCursorPos();
            processCursorTo(Math.max(pos[0] - count,1), 1);
        }

        @Override
        protected void processCursorDownLine(int count) throws IOException {
            int pos[] = currentCursorPos();
            processCursorTo(pos[0] + count, 1);
        }

        @Override
        protected void processCursorLeft(int count) throws IOException {
            int pos[] = currentCursorPos();
            processCursorTo(pos[0], Math.max(pos[1] - count,1));
        }

        @Override
        protected void processCursorRight(int count) throws IOException {
            int pos[] = currentCursorPos();
            processCursorTo(pos[0], pos[1] + count);
        }

        @Override
        protected void processCursorDown(int count) throws IOException {
            int pos[] = currentCursorPos();
            processCursorTo(pos[0] + count, pos[1]);
        }

        @Override
        protected void processCursorUp(int count) throws IOException {
            int pos[] = currentCursorPos();
            processCursorTo(Math.max(pos[0] - count,1), pos[1]);
        }

        private final ArrayList<Integer[]> cursorPositionStack = new ArrayList<>();

        @Override
        protected void processRestoreCursorPosition() throws IOException {
            if (cursorPositionStack.size() > 0) {
                Integer [] pos = cursorPositionStack.get(cursorPositionStack.size()-1);
                cursorPositionStack.remove(cursorPositionStack.size()-1);

                processCursorTo(pos[0], pos[1]);
            }
        }

        @Override
        protected void processSaveCursorPosition() throws IOException {
            int [] p = currentCursorPos();
            Integer [] pp = {p[0],p[1]};
            cursorPositionStack.add(pp);
        }

        @Override
        protected void processScrollDown(int optionInt) throws IOException {
            android.util.Log.d("ConsoleTextView", "processScrollDown");
        }

        @Override
        protected void processScrollUp(int optionInt) throws IOException {
            android.util.Log.d("ConsoleTextView", "processScrollUp");
        }

        @Override
        protected void processEraseLine(int eraseOption) throws IOException {
            synchronized (lock) {
                switch(eraseOption) {
                    case ERASE_LINE_TO_END: {
                        String dt = displayText.toString();
                        int next = dt.indexOf('\n', index);
                        if (next >= 0) {
                            displayText = TextUtils.concat(displayText.subSequence(0, index)
                                    ,displayText.subSequence(next,dt.length()));
                        } else {
                            displayText = displayText.subSequence(0, index);
                        }
                        break;
                    }
                    case ERASE_LINE_TO_BEGINING: {
                        String dt = displayText.toString();
                        int currIndex = index;
                        int[] currPos = currentCursorPos();
                        processCursorTo(currPos[0], 1);
                        displayText = TextUtils.concat(displayText.subSequence(0, index)
                                , displayText.subSequence(currIndex, dt.length()));
                        break;
                    }
                    case ERASE_LINE: {
                        int[] currPos = currentCursorPos();
                        processCursorTo(currPos[0], 1);
                        processEraseLine(ERASE_LINE_TO_END);
                        break;
                    }
                }
            }
            flush();
        }

    }
}
