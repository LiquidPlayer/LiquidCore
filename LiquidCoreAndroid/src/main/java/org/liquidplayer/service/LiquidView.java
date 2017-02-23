package org.liquidplayer.service;

import android.content.Context;
import android.content.res.TypedArray;
import android.os.Handler;
import android.os.Looper;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.Toast;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.node.Process;
import org.liquidplayer.node.R;

import java.lang.reflect.Field;
import java.net.URI;
import java.util.ArrayList;
import java.util.List;

public class LiquidView extends RelativeLayout {

    public LiquidView(Context context) {
        this(context, null);
    }

    public LiquidView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    @SuppressWarnings("unchecked")
    public LiquidView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setSaveEnabled(true);

        TypedArray a = context.getTheme().obtainStyledAttributes(
                attrs,
                R.styleable.LiquidView,
                0, 0);
        try {
            if (a.hasValue(R.styleable.LiquidView_liquidcore_URI)) {
                String uri_ = a.getString(R.styleable.LiquidView_liquidcore_URI);
                if (uri_ != null) {
                    uri = URI.create(uri_);
                }
            }
            if (a.hasValue(R.styleable.LiquidView_liquidcore_argv))
                argv = getResources()
                        .getStringArray(a.getResourceId(R.styleable.LiquidView_liquidcore_argv, 0));
            if (a.hasValue(R.styleable.LiquidView_liquidcore_surface)) {
                String [] surfaces = getResources().getStringArray(
                        a.getResourceId(R.styleable.LiquidView_liquidcore_surface, 0));
                for (String surface : surfaces) {
                    if (surface.startsWith(".")) surface = "org.liquidplayer.surfaces" + surface;
                    try {
                        enableSurface((Class<? extends Surface>)
                                getClass().getClassLoader().loadClass(surface));
                    } catch (ClassNotFoundException e) {
                        e.printStackTrace();
                    }
                }
            }
        } finally {
            a.recycle();
        }

        LayoutInflater.from(context).inflate(R.layout.liquid_view, this);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        if (surfaceId == View.NO_ID && uri != null) {
            start(uri, argv);
            uri = null;
        }
    }

    private void attach(final String surface, final JSFunction callback) {
        final MicroService service = MicroService.getService(serviceId);
        try {
            if (service == null)
                throw new Exception("service not available");
            if (surface == null || surfaceNames == null ||
                    !surfaceNames.contains(surface)) {
                throw new Exception("Invalid surface");
            }
            Class<? extends Surface> clss = null;
            for (MicroService.AvailableSurface sfc : availableSurfaces()) {
                if (sfc.cls.getCanonicalName().equals(surface)) {
                    clss = sfc.cls;
                }
            }
            if (clss == null) {
                throw new Exception("Surface " + surface +
                        "not available");
            }
            final Class<? extends Surface> cls = clss;
            if (surfaceId == View.NO_ID)
                surfaceId = View.generateViewId();
            canonicalSurface = surface;
            service.getProcess().keepAlive();
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override @SuppressWarnings("unchecked")
                public void run() {
                    try {
                        Surface s = cls.getConstructor(Context.class)
                                .newInstance(LiquidView.this.getContext());
                        surfaceView = (View) s;
                        surfaceView.setId(surfaceId);
                        addView(surfaceView);
                        if (childrenStates != null) {
                            for (int i = 0; i < getChildCount(); i++) {
                                getChildAt(i).restoreHierarchyState(childrenStates);
                            }
                            childrenStates = null;
                        }
                        s.attach(service, new Runnable() {
                            @Override
                            public void run() {
                                if (callback != null) {
                                    callback.call(null);
                                }
                                service.getProcess().letDie();
                            }
                        });
                    } catch (Exception e) {
                        e.printStackTrace();
                        android.util.Log.d("exception", e.toString());
                        service.getProcess().letDie();
                    }
                }
            });
            /* This is necessary if the view has been restored */
            service.getProcess().addEventListener(new Process.EventListener() {
                public void onProcessStart(Process process, JSContext context) {}
                public void onProcessAboutToExit(Process process, int exitCode) {
                    detach(null);
                    process.removeEventListener(this);
                }
                public void onProcessExit(Process process, int exitCode) {
                    detach(null);
                    process.removeEventListener(this);
                }
                public void onProcessFailed(Process process, Exception error) {
                    detach(null);
                    process.removeEventListener(this);
                }
            });
        } catch (Exception e) {
            android.util.Log.d("exception", e.toString());
            if (callback != null) {
                callback.call(null, e.getMessage());
            }
            detach(null);
        }
    }

    private void detach(final JSFunction callback) {
        surfaceId = NO_ID;
        canonicalSurface = null;
        if (surfaceView != null) {
            ((Surface)surfaceView).detach();
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    removeView(surfaceView);
                    surfaceView = null;
                }
            });
        }
        if(callback != null) {
            callback.call();
        }
    }

    public void addServiceStartListener(MicroService.ServiceStartListener listener) {
        if (!startListeners.contains(listener))
            startListeners.add(listener);
    }

    private ArrayList<MicroService.ServiceStartListener> startListeners = new ArrayList<>();

    public MicroService start(final URI uri, String ... argv) {
        if (getId() == View.NO_ID) {
            setId(View.generateViewId());
        }
        if (uri != null) {
            if (surfaceNames == null) {
                MicroService.AvailableSurface[] surfaces = availableSurfaces();
                surfaceNames = new ArrayList<>();
                surfaceVersions = new ArrayList<>();
                for (MicroService.AvailableSurface surface : surfaces) {
                    surfaceNames.add(surface.cls.getCanonicalName());
                    surfaceVersions.add(surface.version == null ? "0" : surface.version);
                }
            }

            MicroService svc =
                    new MicroService(getContext(), uri, new MicroService.ServiceStartListener(){
                @Override
                public void onStart(final MicroService service) {
                    serviceId = service.getId();
                    service.getProcess().addEventListener(new Process.EventListener() {
                        @Override
                        public void onProcessStart(Process process, JSContext context) {
                            JSObject liquidCore = context.property("LiquidCore").toObject();
                            JSObject availableSurfaces = new JSObject(context);
                            for (int i=0; surfaceNames != null && surfaceVersions != null &&
                                    i<surfaceNames.size(); i++){
                                availableSurfaces.property(surfaceNames.get(i),
                                        surfaceVersions.get(i));
                            }
                            liquidCore.property("availableSurfaces", availableSurfaces);
                            liquidCore.property("attach", new JSFunction(context,"attach_") {
                                @SuppressWarnings("unused")
                                public void attach_(String s, JSFunction cb) {
                                    attach(s,cb);
                                }
                            });
                            liquidCore.property("detach", new JSFunction(context,"detach_") {
                                @SuppressWarnings("unused")
                                public void detach_(JSFunction cb) {
                                    detach(cb);
                                }
                            });
                            for (MicroService.ServiceStartListener listener : startListeners) {
                                listener.onStart(service);
                            }
                            startListeners.clear();
                        }

                        @Override public void onProcessAboutToExit(Process process, int exitCode) {
                            process.removeEventListener(this);
                        }
                        @Override public void onProcessExit(Process process, int exitCode) {
                            process.removeEventListener(this);
                        }
                        @Override public void onProcessFailed(Process process, Exception error) {
                            process.removeEventListener(this);
                        }
                    });
                }
            },
            new MicroService.ServiceErrorListener() {
                @Override
                public void onError(MicroService service, Exception e) {
                    detach(null);
                    new Handler(Looper.getMainLooper()).post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(getContext(),"Failed to start service at " + uri,
                                    Toast.LENGTH_LONG).show();
                        }
                    });
                }
            });
            svc.setAvailableSurfaces(availableSurfaces());
            svc.start(argv);
            return svc;
        }

        return null;
    }

    public void enableSurface(Class<? extends Surface> cls) {
        try {
            Field f = cls.getDeclaredField("SURFACE_VERSION");
            surfaces.add(new MicroService.AvailableSurface(cls,f.get(null).toString()));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private ArrayList<MicroService.AvailableSurface> surfaces = new ArrayList<>();

    private MicroService.AvailableSurface[] availableSurfaces() {
        return surfaces.toArray(new MicroService.AvailableSurface[surfaces.size()]);
    }

    /* -- local privates -- */
    private URI uri;
    private String [] argv;
    private View surfaceView;

    /* -- parcelable privates -- */
    private int surfaceId = View.NO_ID;
    private ArrayList<String> surfaceNames = null;
    private ArrayList<String> surfaceVersions = null;
    private String serviceId;
    private String canonicalSurface;
    private SparseArray childrenStates;

    @Override @SuppressWarnings("unchecked")
    public Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedState ss = new SavedState(superState);
        ss.surfaceId = surfaceId;
        ss.surfaceNames = surfaceNames;
        ss.surfaceVersions = surfaceVersions;
        ss.serviceId = serviceId;
        ss.canonicalSurface = canonicalSurface;
        ss.childrenStates = new SparseArray();
        for (int i = 0; i < getChildCount(); i++) {
            getChildAt(i).saveHierarchyState(ss.childrenStates);
        }
        return ss;
    }

    @Override @SuppressWarnings("unchecked")
    public void onRestoreInstanceState(Parcelable state) {
        SavedState ss = (SavedState) state;
        super.onRestoreInstanceState(ss.getSuperState());
        surfaceId = ss.surfaceId;
        if (ss.surfaceNames == null) {
            surfaceNames = new ArrayList<>();
        } else {
            surfaceNames = new ArrayList<>(ss.surfaceNames);
        }
        if (ss.surfaceVersions == null) {
            ss.surfaceVersions = new ArrayList<>();
        } else {
            surfaceVersions = new ArrayList<>(ss.surfaceVersions);
        }
        for (String surface : surfaceNames) {
            try {
                enableSurface((Class<? extends Surface>)
                        getClass().getClassLoader().loadClass(surface));
            } catch (ClassNotFoundException e) {
                e.printStackTrace();
            }
        }

        serviceId = ss.serviceId;
        canonicalSurface = ss.canonicalSurface;
        childrenStates = ss.childrenStates;
        if (canonicalSurface != null) {
            attach(canonicalSurface, null);
        }
    }

    @Override
    protected void dispatchSaveInstanceState(SparseArray<Parcelable> container) {
        dispatchFreezeSelfOnly(container);
    }

    @Override
    protected void dispatchRestoreInstanceState(SparseArray<Parcelable> container) {
        dispatchThawSelfOnly(container);
    }

    @SuppressWarnings("unused")
    static class SavedState extends BaseSavedState {
        SparseArray childrenStates;
        private int surfaceId;
        private List<String> surfaceNames;
        private List<String> surfaceVersions;
        private String serviceId;
        private String canonicalSurface;

        SavedState(Parcelable superState) {
            super(superState);
        }

        private SavedState(Parcel in, ClassLoader classLoader) {
            super(in);
            surfaceId = in.readInt();
            in.readStringList(surfaceNames);
            in.readStringList(surfaceVersions);
            serviceId = in.readString();
            canonicalSurface = in.readString();
            childrenStates = in.readSparseArray(classLoader);
        }

        @Override @SuppressWarnings("unchecked")
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeInt(surfaceId);
            out.writeStringList(surfaceNames);
            out.writeStringList(surfaceVersions);
            out.writeString(serviceId);
            out.writeString(canonicalSurface);
            out.writeSparseArray(childrenStates);
        }

        public static final ClassLoaderCreator<SavedState> CREATOR
                = new ClassLoaderCreator<SavedState>() {
            @Override
            public SavedState createFromParcel(Parcel source, ClassLoader loader) {
                return new SavedState(source, loader);
            }

            @Override
            public SavedState createFromParcel(Parcel source) {
                return createFromParcel(source, null);
            }

            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
    }
}
