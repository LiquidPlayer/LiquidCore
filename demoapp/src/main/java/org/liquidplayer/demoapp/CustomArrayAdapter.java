//
// CustomArrayAdapter.java
// LiquidPlayer Project
//
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
/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.liquidplayer.demoapp;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.telecom.Call;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AbsListView;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.VideoView;

import org.liquidplayer.javascript.JSArray;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.widget.NodeConsoleView;

import java.io.File;
import java.util.List;
import java.util.Scanner;

/**
 * This is a custom array adapter used to populate the listview whose items will
 * expand to display extra content in addition to the default display.
 */
class CustomArrayAdapter extends ArrayAdapter<ExpandableListItem> {

    static String webtorrent_js = null;

    private class CallbackObject extends JSObject implements NodeConsoleView.Listener {
        CallbackObject(JSContext ctx, ExpandableListItem object,
                       View convertView, int position, NodeConsoleView consoleView) {
            super(ctx);
            this.object = object;
            this.position = position;
            this.convertView = convertView;

            if (webtorrent_js == null) {
                webtorrent_js = new Scanner(getClass().getClassLoader()
                        .getResourceAsStream("webtorrent.js"), "UTF-8")
                        .useDelimiter("\\A").next();

            }

            download = (ImageButton) convertView.findViewById(R.id.icon);
            trash = (ImageView) convertView.findViewById(R.id.trash);
            progressBar = (ProgressBar) convertView.findViewById(R.id.secondLine);

            uiThread.post(setDownloadPlayButton);

            progressBar.setMax(1000);

            this.consoleView = consoleView;
        }

        private final ExpandableListItem object;
        private final ImageButton download;
        private final ImageView trash;
        private final ProgressBar progressBar;
        private final int position;
        private final NodeConsoleView consoleView;
        private final View convertView;

        private final Handler uiThread = new Handler(Looper.getMainLooper());

        private Runnable setDownloadPlayButton = new Runnable() {
            @Override
            public void run() {
                if (object.getFileName() != null) {
                    download.setImageResource(Resources.getSystem()
                            .getIdentifier("ic_media_play", "drawable", "android"));
                    download.setAlpha(1f);
                    download.setEnabled(true);
                    download.setOnClickListener(playVideo);

                    trash.setAlpha(1f);
                    trash.setEnabled(true);
                    trash.setOnClickListener(deleter);
                } else {
                    download.setImageResource(Resources.getSystem()
                            .getIdentifier("stat_sys_download", "drawable", "android"));
                    download.setAlpha(1f);
                    download.setEnabled(true);
                    download.setOnClickListener(downloader);

                    trash.setAlpha(0.5f);
                    trash.setEnabled(false);
                }
            }
        };

        final View.OnClickListener downloader = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                download.setAlpha(0.5f);
                download.setEnabled(false);
                getContext().evaluateScript("process.chdir('/home/external/persistent')");
                getContext().property("process").toObject().property("argv", new String[]{
                        "node", "webtorrent.js", "-p", "" + (8080 + position), "download",
                        object.getUrl()
                });
                getContext().evaluateScript(webtorrent_js);
            }
        };

        private View.OnClickListener playVideo = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                final Dialog dialog = new Dialog(CustomArrayAdapter.this.getContext());
                dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
                dialog.setContentView(R.layout.video_dialog);
                dialog.show();
                WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                        WindowManager.LayoutParams.WRAP_CONTENT,
                        WindowManager.LayoutParams.WRAP_CONTENT);
                lp.copyFrom(dialog.getWindow().getAttributes());
                dialog.getWindow().setAttributes(lp);
                File external = CustomArrayAdapter.this.getContext().getExternalFilesDir(null);
                String path = external.getAbsolutePath() +
                        "/LiquidPlayer/node_console/" + object.getFileName();
                Uri uriPath = Uri.fromFile(new File(path));

                VideoView videoView = (VideoView) dialog.findViewById(R.id.video_view);
                videoView.setVideoURI(uriPath);
                videoView.start();
            }
        };

        private View.OnClickListener deleter = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                File external = CustomArrayAdapter.this.getContext().getExternalFilesDir(null);
                String path = external.getAbsolutePath() +
                        "/LiquidPlayer/node_console/" + object.getFileName();
                new File(path).delete();
                getContext().evaluateScript("global.callbackObject={onTorrentDone:function(){}, onDraw:function(){}};");
                consoleView.reset();
                object.setFileName(null);
                progressBar.setProgress(0);
                trash.setAlpha(0.5f);
                trash.setEnabled(false);
                download.setImageResource(Resources.getSystem()
                        .getIdentifier("stat_sys_download", "drawable", "android"));
                download.setAlpha(0.5f);
                download.setEnabled(false);

                consoleView.setListener(CallbackObject.this);
            }
        };

        @Override
        public void onJSReady(final JSContext ctx) {
            ctx.property("callbackObject",
                    new CallbackObject(ctx,object,convertView,position,consoleView));
        }

        @JSObject.jsexport
        void onTorrentDone(JSObject torrent) {
            android.util.Log.d("torrent", "onTorrentDone");
            if (torrent.property("files").isArray() && torrent.property("length").toNumber() > 0) {
                @SuppressWarnings("unchecked")
                JSObject file =
                    ((JSArray<JSValue>) torrent.property("files").toJSArray()).get(0).toObject();
                String fileName = file.property("path").toString();
                android.util.Log.d("torrent", "file = " + fileName);
                object.setFileName(fileName);
                uiThread.post(setDownloadPlayButton);
            }
        }

        @JSObject.jsexport
        void onDraw(JSObject torrent) {
            final double progress = torrent.property("progress").toNumber();
            uiThread.post(new Runnable() {
                @Override
                public void run() {
                    progressBar.setProgress((int) (progress * 1000));
            }
            });
        }

    }

    private List<ExpandableListItem> mData;
    private int mLayoutViewResourceId;
    private ExpandingListView mListView;

    CustomArrayAdapter(Context context, int layoutViewResourceId,
                              List<ExpandableListItem> data, ExpandingListView listView) {
        super(context, layoutViewResourceId, data);
        mData = data;
        mLayoutViewResourceId = layoutViewResourceId;
        mListView = listView;
    }

    /**
     * Populates the item in the listview cell with the appropriate data. This method
     * sets the thumbnail image, the title and the extra text. This method also updates
     * the layout parameters of the item's view so that the image and title are centered
     * in the bounds of the collapsed view, and such that the extra text is not displayed
     * in the collapsed state of the cell.
     */
    @Override
    public @NonNull View getView(final int position, View convertView, @NonNull ViewGroup parent) {

        final ExpandableListItem object = mData.get(position);

        int id = position==0 ? R.id.console1 : position==1 ? R.id.console2 : R.id.console3;
        if(convertView == null) {
            LayoutInflater inflater = ((Activity) getContext()).getLayoutInflater();
            convertView = inflater.inflate(mLayoutViewResourceId, parent, false);

            RelativeLayout linearLayout = (RelativeLayout) (convertView.findViewById(
                    R.id.item_linear_layout));
            LinearLayout.LayoutParams linearLayoutParams = new LinearLayout.LayoutParams
                    (AbsListView.LayoutParams.MATCH_PARENT, object.getCollapsedHeight());
            linearLayout.setLayoutParams(linearLayoutParams);

            TextView titleView = (TextView) convertView.findViewById(R.id.firstLine);

            ImageView consoleButton = (ImageView) convertView.findViewById(R.id.console);
            final View itemView = convertView;
            consoleButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (object.isExpanded()) {
                        mListView.collapseView(itemView);
                    } else {
                        mListView.expandView(itemView);
                    }
                }
            });

            titleView.setText(object.getTitle());

            convertView.setLayoutParams(new ListView.LayoutParams(
                    AbsListView.LayoutParams.MATCH_PARENT,
                    AbsListView.LayoutParams.WRAP_CONTENT));

            ExpandingLayout expandingLayout = (ExpandingLayout) convertView.findViewById(R.id
                    .expanding_layout);
            final LinearLayout ll = (LinearLayout) convertView.findViewById(R.id.fragment);
            ll.setId(ViewStub.generateViewId());

            final NodeConsoleView consoleView = new NodeConsoleView(getContext());
            consoleView.setId(id);
            consoleView.setLayoutParams(
                    new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT,
                            (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 300,
                                    getContext().getResources().getDisplayMetrics())));
            consoleView.setListener(new NodeConsoleView.Listener() {
                @Override
                public void onJSReady(final JSContext ctx) {
                    ctx.property("callbackObject",
                            new CallbackObject(ctx,object,itemView,position,consoleView));

                    final JSFunction log = ctx.property("console").toObject()
                            .property("log").toFunction();
                    log.call(null, "all set up");
                }
            });

            ll.addView(consoleView);

            expandingLayout.setExpandedHeight(object.getExpandedHeight());
            expandingLayout.setSizeChangedListener(object);

            if (!object.isExpanded()) {
                expandingLayout.setVisibility(View.GONE);
            } else {
                expandingLayout.setVisibility(View.VISIBLE);
            }
        }
        return convertView;
    }

}