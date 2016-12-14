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
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AbsListView;
import android.widget.ArrayAdapter;
import android.widget.FrameLayout;
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

import java.io.File;
import java.util.List;
import java.util.Scanner;

/**
 * This is a custom array adapter used to populate the listview whose items will
 * expand to display extra content in addition to the default display.
 */
public class CustomArrayAdapter extends ArrayAdapter<ExpandableListItem> {

    private List<ExpandableListItem> mData;
    private int mLayoutViewResourceId;
    private ExpandingListView mListView;

    public CustomArrayAdapter(Context context, int layoutViewResourceId,
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

        if(convertView == null) {
            LayoutInflater inflater = ((Activity) getContext()).getLayoutInflater();
            convertView = inflater.inflate(mLayoutViewResourceId, parent, false);

            RelativeLayout linearLayout = (RelativeLayout) (convertView.findViewById(
                    R.id.item_linear_layout));
            LinearLayout.LayoutParams linearLayoutParams = new LinearLayout.LayoutParams
                    (AbsListView.LayoutParams.MATCH_PARENT, object.getCollapsedHeight());
            linearLayout.setLayoutParams(linearLayoutParams);

            TextView titleView = (TextView) convertView.findViewById(R.id.firstLine);
            final ProgressBar progressBar = (ProgressBar) convertView.findViewById(R.id.secondLine);
            progressBar.setMax(1000);
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
            //textView.setText(object.getText());

            final ImageButton download = (ImageButton) convertView.findViewById(R.id.icon);
            download.setAlpha(0.5f);
            download.setEnabled(false);

            final ImageView trash = (ImageView) convertView.findViewById(R.id.trash);
            trash.setAlpha(0.5f);
            trash.setEnabled(false);

            convertView.setLayoutParams(new ListView.LayoutParams(AbsListView.LayoutParams.MATCH_PARENT,
                    AbsListView.LayoutParams.WRAP_CONTENT));

            ExpandingLayout expandingLayout = (ExpandingLayout) convertView.findViewById(R.id
                    .expanding_layout);
            final LinearLayout fragLayout = (LinearLayout) convertView.findViewById(R.id.fragment);

            final FrameLayout fl = new FrameLayout((Activity) getContext());
            fl.setId(View.generateViewId());

            FragmentManager fragMan = ((Activity) getContext()).getFragmentManager();
            FragmentTransaction fragTransaction = fragMan.beginTransaction();
            NodeConsoleFragment fragment = NodeConsoleFragment.newInstance("fragment" + position);
            fragment.setListener(new NodeConsoleFragment.Listener() {
                @Override
                public void onJSReady(final JSContext ctx) {

                    final JSFunction log = ctx.property("console").toObject()
                            .property("log").toFunction();
                    ctx.evaluateScript("process.chdir('/home/external/persistent')");

                    ctx.property("onInfoHash", new JSFunction(ctx, "onInfoHash_") {
                        @SuppressWarnings("unused")
                        void onInfoHash_(JSObject torrent) {
                            android.util.Log.d("torrent", "onInfoHash");
                        }
                    });

                    final View.OnClickListener downloadClickListener = new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            download.setAlpha(0.5f);
                            download.setEnabled(false);
                            String webtorrent_js = new Scanner(getClass().getClassLoader()
                                    .getResourceAsStream("webtorrent.js"), "UTF-8")
                                    .useDelimiter("\\A").next();
                            ctx.evaluateScript(webtorrent_js);
                        }
                    };

                    ctx.property("onTorrentDone", new JSFunction(ctx, "onTorrentDone_") {
                        @SuppressWarnings("unused")
                        void onTorrentDone_(JSObject torrent) {
                            android.util.Log.d("torrent", "onTorrentDone");
                            if (torrent.property("files").isArray() && torrent.property("length").toNumber() > 0) {
                                @SuppressWarnings("unchecked")
                                JSObject file = ((JSArray<JSValue>)torrent.property("files").toJSArray()).get(0).toObject();
                                String fileName = file.property("path").toString();
                                android.util.Log.d("torrent", "file = " + fileName);
                                object.setFileName(fileName);
                                new Handler(Looper.getMainLooper()).post(new Runnable() {
                                    @Override
                                    public void run() {
                                        download.setImageResource(Resources.getSystem()
                                            .getIdentifier("ic_media_play", "drawable", "android"));
                                        download.setAlpha(1f);
                                        download.setEnabled(true);
                                        download.setOnClickListener(new View.OnClickListener() {
                                            @Override
                                            public void onClick(View view) {
                                                final Dialog dialog = new Dialog(CustomArrayAdapter.this.getContext());// add here your class name
                                                dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
                                                dialog.setContentView(R.layout.video_dialog);//add your own xml with defied with and height of videoview
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
                                        });
                                        trash.setAlpha(1f);
                                        trash.setEnabled(true);
                                        trash.setOnClickListener(new View.OnClickListener() {
                                            @Override
                                            public void onClick(View view) {
                                                File external = CustomArrayAdapter.this.getContext().getExternalFilesDir(null);
                                                String path = external.getAbsolutePath() +
                                                        "/LiquidPlayer/node_console/" + object.getFileName();
                                                new File(path).delete();
                                                trash.setAlpha(0.5f);
                                                trash.setEnabled(false);
                                                download.setImageResource(Resources.getSystem()
                                                        .getIdentifier("stat_sys_download", "drawable", "android"));
                                                download.setOnClickListener(downloadClickListener);
                                            }
                                        });
                                    }
                                });
                            }
                        }
                    });

                    ctx.property("onDraw", new JSFunction(ctx, "onDraw_") {
                        @SuppressWarnings("unused")
                        void onDraw_(JSObject torrent) {
                            final double progress = torrent.property("progress").toNumber();
                            new Handler(Looper.getMainLooper()).post(new Runnable() {
                                @Override
                                public void run() {
                                    progressBar.setProgress((int) (progress * 1000));
                                }
                            });
                        }
                    });

                    ctx.property("process").toObject().property("argv", new String[] {
                        "node", "webtorrent.js", "-p", "" + (8080+position), "download",
                        object.getUrl()
                    });

                    log.call(null, "all set up");

                    new Handler(Looper.getMainLooper()).post(new Runnable() {
                        @Override
                        public void run() {
                            download.setAlpha(1f);
                            download.setEnabled(true);
                            download.setOnClickListener(downloadClickListener);
                        }
                    });
                }
            });

            fragTransaction.replace(fl.getId(), fragment, "fragment" + position);
            fragTransaction.commit();

            fragLayout.addView(fl);

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

    /**
     * Crops a circle out of the thumbnail photo.
     */
    public Bitmap getCroppedBitmap(Bitmap bitmap) {
        Bitmap output = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(),
                Config.ARGB_8888);

        final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());

        Canvas canvas = new Canvas(output);

        final Paint paint = new Paint();
        paint.setAntiAlias(true);

        int halfWidth = bitmap.getWidth()/2;
        int halfHeight = bitmap.getHeight()/2;

        canvas.drawCircle(halfWidth, halfHeight, Math.max(halfWidth, halfHeight), paint);

        paint.setXfermode(new PorterDuffXfermode(Mode.SRC_IN));

        canvas.drawBitmap(bitmap, rect, rect, paint);

        return output;
    }


}