package org.liquidplayer.torrentexample;

import android.app.Dialog;
import android.content.Context;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AbsListView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.VideoView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.liquidplayer.service.LiquidView;
import org.liquidplayer.service.MicroService;
import org.liquidplayer.surfaces.console.ConsoleSurface;

import java.io.File;
import java.net.URI;

public class ItemView extends LinearLayout {

    private final URI consoleURI =
            URI.create("android.resource://"+ getContext().getPackageName()+ "/raw/webtorrent_cli");
    private static int port = 8080;

    private final Handler uiThread = new Handler(Looper.getMainLooper());
    private ImageButton download;
    private ImageView trash;
    private ProgressBar progressBar;
    private LiquidView liquidView;
    private ExpandingListView mListView;
    private ExpandableListItem object;

    public static ItemView inflate(ViewGroup parent) {
        return (ItemView) LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_view, parent, false);
    }

    public ItemView(Context c) {
        this(c, null);
    }

    public ItemView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ItemView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        LayoutInflater.from(context).inflate(R.layout.item_view_children, this, true);
        setupChildren();
    }

    public void setItem(ExpandableListItem item) {
        object = item;

        ItemView oldView = object.getCurrentView();
        Parcelable liquidViewState = null;
        if (oldView != null) {
            LiquidView v = (LiquidView) oldView.findViewById(R.id.console1);
            if (v != null) {
                liquidViewState = v.onSaveInstanceState();
            }
        }
        object.setCurrentView(this);

        RelativeLayout relativeLayout = (RelativeLayout) (findViewById(
                R.id.item_linear_layout));
        LinearLayout.LayoutParams linearLayoutParams = new LinearLayout.LayoutParams
                (AbsListView.LayoutParams.MATCH_PARENT, object.getCollapsedHeight());
        relativeLayout.setLayoutParams(linearLayoutParams);

        TextView titleView = (TextView) findViewById(R.id.firstLine);
        titleView.setText(object.getTitle());

        final LinearLayout ll = (LinearLayout) findViewById(R.id.liquidview);

        liquidView = new LiquidView(getContext());
        liquidView.setId(R.id.console1);
        liquidView.setLayoutParams(
                new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT,
                        (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 300,
                                getContext().getResources().getDisplayMetrics())));

        if (liquidViewState != null) {
            liquidView.onRestoreInstanceState(liquidViewState);
        }
        ll.addView(liquidView);

        ImageView consoleButton = (ImageView) findViewById(R.id.console);
        consoleButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (object.isExpanded()) {
                    mListView.collapseView(ItemView.this);
                } else {
                    mListView.expandView(ItemView.this);
                }
            }
        });
        progressBar.setProgress(object.getProgress());

        ExpandingLayout expandingLayout = (ExpandingLayout) findViewById(R.id.expanding_layout);

        progressBar.setMax(1000);

        expandingLayout.setExpandedHeight(object.getExpandedHeight());
        expandingLayout.setSizeChangedListener(object);

        if (!object.isExpanded()) {
            expandingLayout.setVisibility(View.GONE);
        } else {
            expandingLayout.setVisibility(View.VISIBLE);
        }

        setDownloadPlayButton();
    }

    public void setListView(ExpandingListView view) {
        mListView = view;
    }

    private void setupChildren() {
        setLayoutParams(new ListView.LayoutParams(
                AbsListView.LayoutParams.MATCH_PARENT,
                AbsListView.LayoutParams.WRAP_CONTENT));

        download = (ImageButton) findViewById(R.id.icon);
        trash = (ImageView) findViewById(R.id.trash);

        progressBar = (ProgressBar) findViewById(R.id.secondLine);
        progressBar.setMax(1000);
    }

    private void setDownloadPlayButton() {
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

            if (object.isDownloading()) {
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

    private final View.OnClickListener playVideo = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            final Dialog dialog = new Dialog(getContext());
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
            Uri uriPath = Uri.fromFile(new File(object.getFileName()));

            VideoView videoView = (VideoView) dialog.findViewById(R.id.video_view);
            videoView.setVideoURI(uriPath);
            videoView.start();
        }
    };

    private final View.OnClickListener deleter = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            if (!new File(object.getFileName()).delete()) {
                android.util.Log.e("deleter", "Failed to delete file");
            }
            object.setFileName(null);
            progressBar.setProgress(0);
            object.setProgress(0);
            trash.setAlpha(0.5f);
            trash.setEnabled(false);
            setDownloadPlayButton();
        }
    };

    private final MicroService.EventListener drawListener =
            new MicroService.EventListener() {
                @Override
                public void onEvent(MicroService service, String event,
                                    JSONObject torrent) {
                    try {
                        final double progress = torrent.getDouble("progress");
                        object.setProgress((int)(progress * 1000));
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                if (object.getCurrentView() != null) {
                                    object.getCurrentView().
                                            progressBar.setProgress(object.getProgress());
                                }
                            }
                        });
                    } catch (JSONException e) {
                        android.util.Log.e("drawListener", "Malformed JSON");
                    }
                }
            };

    private final MicroService.EventListener doneListener =
            new MicroService.EventListener() {
                @Override
                public void onEvent(MicroService service, String event,
                                    JSONObject torrent) {
                    object.setDownloading(false);
                    object.setServiceId(null);
                    try {
                        JSONArray files = torrent.getJSONArray("files");
                        String fileName = Environment.getExternalStoragePublicDirectory(
                                Environment.DIRECTORY_MOVIES).getAbsolutePath() + "/" +
                                files.getJSONObject(0).getString("path");
                        object.setFileName(fileName);
                        object.setProgress(1000);
                        service.removeEventListener("torrent_done", this);
                        service.removeEventListener("draw", drawListener);
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                if (object.getCurrentView() != null) {
                                    object.getCurrentView().
                                            progressBar.setProgress(object.getProgress());
                                    object.getCurrentView().setDownloadPlayButton();
                                }
                            }
                        });
                    } catch (JSONException e) {
                        android.util.Log.e("doneListener", "Malformed JSON");
                    }
                }
            };

    private final View.OnClickListener downloader = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            download.setAlpha(0.5f);
            download.setEnabled(false);
            object.setDownloading(true);
            liquidView.enableSurface(ConsoleSurface.class);
            liquidView.addServiceStartListener(new MicroService.ServiceStartListener() {
                @Override public void onStart(MicroService service) {
                    object.setServiceId(service.getId());
                    service.addEventListener("torrent_done", doneListener);
                    service.addEventListener("draw", drawListener);
                }
            });
            liquidView.start(
                    consoleURI,
                    "-o", "/home/public/media/Movies",
                    "-p", "" + (++port),
                    "download", object.getUrl()
            );
        }
    };

    @Override
    protected Parcelable onSaveInstanceState() {
        android.util.Log.d("ItemView", "onSaveInstanceState");
        return super.onSaveInstanceState();
    }

}
