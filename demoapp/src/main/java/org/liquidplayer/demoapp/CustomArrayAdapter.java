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
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.v4.content.LocalBroadcastManager;
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
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.NodeProcessService;
import org.liquidplayer.node.Process;
import org.liquidplayer.widget.NodeConsoleView;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.List;
import java.util.Scanner;

/**
 * This is a custom array adapter used to populate the listview whose items will
 * expand to display extra content in addition to the default display.
 */
class CustomArrayAdapter extends ArrayAdapter<ExpandableListItem> {

    static private String webtorrent_js = null;

    private Uri consoleURI =
            Uri.parse("android.resource://" + getContext().getPackageName() + "/raw/webtorrent");

    private final Handler uiThread = new Handler(Looper.getMainLooper());
    private static int port = 8080;

    private class UIObject {
        ImageButton download;
        ImageView trash;
        ProgressBar progressBar;
        int position;
        NodeConsoleView consoleView;

        final Runnable setDownloadPlayButton = new Runnable() {
            @Override
            public void run() {
                if (mData.get(position).getFileName() != null) {
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

                    if (mData.get(position).isDownloading()) {
                        download.setAlpha(0.5f);
                        download.setEnabled(false);
                    } else {
                        download.setAlpha(1f);
                        download.setEnabled(true);
                        download.setOnClickListener(downloader);
                    }

                    trash.setAlpha(0.5f);
                    trash.setEnabled(false);
                }
            }
        };

        final View.OnClickListener playVideo = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try {
                    final Dialog dialog = new Dialog(CustomArrayAdapter.this.getContext());
                    dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
                    dialog.setContentView(R.layout.video_dialog);
                    dialog.show();
                    WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                            WindowManager.LayoutParams.WRAP_CONTENT,
                            WindowManager.LayoutParams.WRAP_CONTENT);
                    if (dialog.getWindow() != null) {
                        lp.copyFrom(dialog.getWindow().getAttributes());
                        dialog.getWindow().setAttributes(lp);
                    }
                    File external = CustomArrayAdapter.this.getContext().getExternalFilesDir(null);
                    if (external != null) {
                        String path = external.getAbsolutePath() +
                                "/LiquidPlayer/" + URLEncoder.encode(consoleURI.toString(), "UTF-8")
                                + "/" + mData.get(position).getFileName();
                        Uri uriPath = Uri.fromFile(new File(path));

                        VideoView videoView = (VideoView) dialog.findViewById(R.id.video_view);
                        videoView.setVideoURI(uriPath);
                        videoView.start();
                    } else {
                        android.util.Log.e("playVideo", "No external storage");
                    }
                } catch (UnsupportedEncodingException e) {
                    android.util.Log.e("playVideo", e.toString());
                }
            }
        };

        final View.OnClickListener deleter = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try {
                    File external = CustomArrayAdapter.this.getContext().getExternalFilesDir(null);
                    if (external != null) {
                        String path = external.getAbsolutePath() +
                                "/LiquidPlayer/" + URLEncoder.encode(consoleURI.toString(), "UTF-8")
                                + "/" + mData.get(position).getFileName();
                        if (!new File(path).delete()) {
                            android.util.Log.e("deleter", "Failed to delete file " + path);
                        }
                    }

                    consoleView.reset();
                    mData.get(position).setFileName(null);
                    progressBar.setProgress(0);
                    mData.get(position).setProgress(0);
                    trash.setAlpha(0.5f);
                    trash.setEnabled(false);
                    setDownloadPlayButton.run();
                } catch (UnsupportedEncodingException e) {
                    android.util.Log.e("deleter", e.toString());
                }
            }
        };

        final View.OnClickListener downloader = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                download.setAlpha(0.5f);
                download.setEnabled(false);
                LocalBroadcastManager.getInstance(CustomArrayAdapter.this.getContext()).
                    registerReceiver(
                        new BroadcastReceiver() {
                            @Override
                            public void onReceive(Context context, Intent intent) {
                                final String instanceId = intent.getExtras().getString(
                                        NodeProcessService.kMicroAppInstance);
                                Process process = NodeProcessService.getProcess(instanceId);
                                process.addEventListener(new Process.EventListener() {
                                    @Override
                                    public void onProcessStart(Process process, JSContext ctx) {
                                        consoleView.attach(instanceId);
                                        process.removeEventListener(this);
                                        new CallbackObject(ctx, position);
                                    }
                                    @Override public void onProcessAboutToExit(Process p, int x) {}
                                    @Override public void onProcessExit(Process p, int x) {}
                                    @Override public void onProcessFailed(Process p, Exception x) {}
                                });
                                LocalBroadcastManager.getInstance(context).unregisterReceiver(this);
                            }
                        },
                        new IntentFilter(consoleURI.toString())
                    );

                Intent serviceIntent = new Intent(CustomArrayAdapter.this.getContext(),
                        NodeProcessService.class);
                serviceIntent.putExtra(NodeProcessService.kMicroAppURI, consoleURI.toString());
                CustomArrayAdapter.this.getContext().startService(serviceIntent);
            }
        };
    }

    private class CallbackObject extends JSObject {
        private final int position;

        CallbackObject(JSContext ctx, int position) {
            super(ctx);
            this.position = position;
            ctx.property("callbackObject", this);
            ctx.evaluateScript(
                    "process.chdir('/home/external/persistent')");
            ctx.property("process").toObject().property("argv",
                    new String[] {
                            "node", "webtorrent.js", "-p",
                            "" + (++port), "download",
                            mData.get(position).getUrl()
                    }
            );
            mData.get(position).setDownloading(true);
            ctx.evaluateScript(webtorrent_js);
        }

        @SuppressWarnings("unused")
        @JSObject.jsexport
        void onTorrentDone(JSObject torrent) {
            mData.get(position).setDownloading(false);
            if (torrent.property("files").isArray() && torrent.property("length").toNumber() > 0) {
                final UIObject uiObject = (UIObject) mData.get(position).getData();
                @SuppressWarnings("unchecked")
                JSObject file =
                    ((JSArray<JSValue>) torrent.property("files").toJSArray()).get(0).toObject();
                String fileName = file.property("path").toString();
                mData.get(position).setFileName(fileName);
                getContext().evaluateScript("global.callbackObject={onTorrentDone:function(){},"+
                        "onDraw:function(){}};");
                uiObject.consoleView.detach();
                uiThread.post(new Runnable() {
                    @Override
                    public void run() {
                        uiObject.progressBar.setProgress(1000);
                        mData.get(position).setProgress(1000);
                        uiObject.setDownloadPlayButton.run();
                    }
                });
            }
        }

        @SuppressWarnings("unused")
        @JSObject.jsexport
        void onDraw(JSObject torrent) {
            final double progress = torrent.property("progress").toNumber();
            final UIObject uiObject = (UIObject) mData.get(position).getData();
            uiThread.post(new Runnable() {
                @Override
                public void run() {
                    uiObject.progressBar.setProgress((int) (progress * 1000));
                    mData.get(position).setProgress((int) (progress * 1000));
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

            convertView.setLayoutParams(new ListView.LayoutParams(
                    AbsListView.LayoutParams.MATCH_PARENT,
                    AbsListView.LayoutParams.WRAP_CONTENT));

            final LinearLayout ll = (LinearLayout) convertView.findViewById(R.id.fragment);
            ll.setId(ViewStub.generateViewId());

            NodeConsoleView consoleView = new NodeConsoleView(getContext());
            consoleView.setId(id);
            consoleView.setLayoutParams(
                    new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT,
                            (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 300,
                                    getContext().getResources().getDisplayMetrics())));

            if (webtorrent_js == null) {
                try {
                    InputStream is = getContext().getContentResolver().openInputStream(consoleURI);
                    if (is == null) {
                        throw new FileNotFoundException();
                    }
                    webtorrent_js = new Scanner(is, "UTF-8").useDelimiter("\\A").next();
                } catch (FileNotFoundException e) {
                    android.util.Log.e("webtorrent_js", e.toString());
                }

            }

            ll.addView(consoleView);
        }

        UIObject uiObject = new UIObject();

        uiObject.position = position;
        uiObject.consoleView = (NodeConsoleView) convertView.findViewById(id);
        uiObject.download = (ImageButton) convertView.findViewById(R.id.icon);
        uiObject.trash = (ImageView) convertView.findViewById(R.id.trash);
        uiObject.progressBar = (ProgressBar) convertView.findViewById(R.id.secondLine);

        mData.get(position).setData(uiObject);

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

        ExpandingLayout expandingLayout = (ExpandingLayout) convertView.findViewById(R.id
                .expanding_layout);

        uiObject.progressBar.setMax(1000);
        uiObject.progressBar.setProgress(object.getProgress());

        uiObject.setDownloadPlayButton.run();

        expandingLayout.setExpandedHeight(object.getExpandedHeight());
        expandingLayout.setSizeChangedListener(object);

        if (!object.isExpanded()) {
            expandingLayout.setVisibility(View.GONE);
        } else {
            expandingLayout.setVisibility(View.VISIBLE);
        }

        return convertView;
    }

}