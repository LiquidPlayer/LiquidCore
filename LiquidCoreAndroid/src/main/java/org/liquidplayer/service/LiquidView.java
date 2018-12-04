/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.service;

import android.content.Context;
import android.content.res.TypedArray;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.Toast;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.Process;
import org.liquidplayer.node.R;

import java.io.IOException;
import java.lang.reflect.Field;
import java.net.URI;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;

import dalvik.system.DexFile;

/**
 * LiquidView exposes a MicroService through a UI.  A MicroService attaches to a UI
 * in JavaScript by calling <code>LiquidCore.attach(surface, callback)</code> where
 * 'surface' is a string representing the Surface class
 * (e.g. 'org.liquidplayer.surfaces.console.ConsoleSurface') and 'callback' is a
 * callback function which accepts an 'error' parameter.  If 'error' is undefined, then the
 * Surface was attached correctly and is ready for use.  Otherwise, 'error' is a descriptive
 * error message.
 */
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
        registerSurfaces(context);

        setSaveEnabled(true);

        TypedArray a = context.getTheme().obtainStyledAttributes(
                attrs,
                R.styleable.LiquidView,
                0, 0);
        try {
            if (a.hasValue(R.styleable.LiquidView_liquidcore_URI)) {
                String uri_ = a.getString(R.styleable.LiquidView_liquidcore_URI);
                if (uri_ != null && uri_.length() != 0) {
                    uri = URI.create(uri_);
                }
            } else if (attrs != null) {
                uri = MicroService.DevServer();
            }
            if (a.hasValue(R.styleable.LiquidView_liquidcore_argv))
                argv = getResources()
                        .getStringArray(a.getResourceId(R.styleable.LiquidView_liquidcore_argv, 0));
            if (a.hasValue(R.styleable.LiquidView_liquidcore_surface)) {
                String [] surfaces;
                final TypedValue v = a.peekValue(R.styleable.LiquidView_liquidcore_surface);
                if (v != null && v.type == TypedValue.TYPE_STRING) {
                    String s = a.getString(R.styleable.LiquidView_liquidcore_surface);
                    surfaces = new String[] {s};
                } else {
                    surfaces = getResources().getStringArray(
                            a.getResourceId(R.styleable.LiquidView_liquidcore_surfaces, 0));
                }
                for (String surface : surfaces) {
                    if (surface.startsWith(".")) surface = "org.liquidplayer.surface" + surface;
                    enableSurface(surface);
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

    private static final AtomicInteger sNextGeneratedId = new AtomicInteger(1);

    /**
     * Generate a value suitable for use in {@link #setId(int)}.
     * This value will not collide with ID values generated at build time by aapt for R.id.
     *
     * @return a generated ID value
     */
    private static int generateViewIdCommon() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            return generateViewId();
        } else {
            for (; ; ) {
                final int result = sNextGeneratedId.get();
                // aapt-generated IDs have the high byte nonzero; clamp to the range under that.
                int newValue = result + 1;
                if (newValue > 0x00FFFFFF) newValue = 1; // Roll over to 1, not 0.
                if (sNextGeneratedId.compareAndSet(result, newValue)) {
                    return result;
                }
            }
        }
    }
    private static final String createPromiseObject =
            "(()=>{" +
            "  var po = {}; var clock = true;" +
            "  var timer = setInterval(()=>{if(!clock) clearTimeout(timer);}, 100); "+
            "  po.promise = new Promise((resolve,reject)=>{po.resolve=resolve;po.reject=reject});" +
            "  po.promise.then(()=>{clock=false}).catch(()=>{clock=false});" +
            "  return po;" +
            "})();";

    private JSObject bind(final JSContext context, final String surface_, final JSValue config) {
        final MicroService service = MicroService.getService(serviceId);
        final JSObject promiseObj = context.evaluateScript(createPromiseObject).toObject();
        try {
            /*
            if (boundCanonicalSurfaces.contains(surface_)) {
                // This surface is already bound or in the process of binding.  Don't do it again.
                // FIXME: The more elegant way to handle this is to resolve the promise if and when
                // the original binding completes, but this would require some refactoring.  For now,
                // just reject the promise.
                throw new Error("Surface " + surface_ + " is already bound.");
            }
            */
            boundCanonicalSurfaces.add(surface_);
            if (service == null)
                throw new Exception("service not available");
            android.util.Log.d("bind", "surface_: " + surface_);

            boolean found = false;
            for (MicroService.AvailableSurface sfc : availableSurfaces()) {
                if (sfc.cls.getCanonicalName().equals(surface_)) {
                    found = true;
                    final Class<? extends Surface> cls = sfc.cls;
                    final String boundSurfaceId = UUID.randomUUID().toString();
                    final JSObject surfaceObject = new JSObject(context);
                    surfaceObject.property("attach", new JSFunction(context,"attach_") {
                        @SuppressWarnings("unused")
                        public JSObject attach_() {
                            Surface surface = surfaceHashMap.get(boundSurfaceId);
                            return attach(context, getThis(), surface);
                        }
                    });
                    surfaceObject.property("detach", new JSFunction(context,"detach_") {
                        @SuppressWarnings("unused")
                        public JSObject detach_(JSFunction cb) {
                            return detach(context);
                        }
                    });

                    new Handler(Looper.getMainLooper()).post(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                Surface s = cls.getConstructor(Context.class)
                                        .newInstance(LiquidView.this.getContext());
                                surfaceHashMap.put(boundSurfaceId, s);
                                boundSurfaces.add(boundSurfaceId);
                                s.bind(service, context, surfaceObject, config, new Runnable() {
                                    @Override
                                    public void run() {
                                        context.getGroup().schedule(new Runnable() {
                                            @Override
                                            public void run() {
                                                promiseObj.property("resolve").toFunction()
                                                        .call(null, surfaceObject);
                                            }
                                        });
                                    }
                                }, new Surface.ReportErrorRunnable() {
                                    @Override
                                    public void run(final String errorMessage) {
                                        context.getGroup().schedule(new Runnable() {
                                            @Override
                                            public void run() {
                                                promiseObj.property("reject").toFunction()
                                                        .call(null, errorMessage);
                                            }
                                        });
                                    }
                                });
                            } catch (final Exception e) {
                                e.printStackTrace();
                                android.util.Log.d("exception", e.toString());
                                context.getGroup().schedule(new Runnable() {
                                    @Override
                                    public void run() {
                                        promiseObj.property("reject").toFunction()
                                                .call(null, e.getMessage());
                                    }
                                });
                            }
                        }
                    });
                    break;
                }
            }
            if (!found) {
                throw new Exception("Surface " + surface_ + " is not available.");
            }

        } catch (Exception e) {
            android.util.Log.d("exception", e.toString());
            promiseObj.property("reject").toFunction().call(null, e.getMessage());
        }

        return promiseObj.property("promise").toObject();
    }

    private JSObject attach(@Nullable final JSContext context, @Nullable final JSObject thiz,
                            final Surface surface) {
        final MicroService service = MicroService.getService(serviceId);
        final JSObject promiseObj;
        if (context != null) promiseObj = context.evaluateScript(createPromiseObject).toObject();
        else promiseObj = null;
        try {
            if (service == null)
                throw new Exception("service not available");
            android.util.Log.d("attach", "surface_: " + surface.getClass().getCanonicalName());
            if (surfaceId == View.NO_ID)
                surfaceId = generateViewIdCommon();

            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override @SuppressWarnings("unchecked")
                public void run() {
                    try {
                        surfaceView = surface.attach(service, new Runnable() {
                            @Override
                            public void run() {
                                attachedSurface = null;
                                for (String s : surfaceHashMap.keySet()) {
                                    if (surfaceHashMap.get(s) == surface) {
                                        attachedSurface = s;
                                        break;
                                    }
                                }

                                if (promiseObj != null) {
                                    promiseObj.property("resolve").toFunction()
                                            .call(null, thiz);
                                }
                            }
                        }, new Surface.ReportErrorRunnable() {
                            @Override
                            public void run(final String errorMessage) {
                                context.getGroup().schedule(new Runnable() {
                                    @Override
                                    public void run() {
                                        promiseObj.property("reject").toFunction()
                                                .call(null, errorMessage);
                                    }
                                });

                            }
                        });
                        surfaceView.setId(surfaceId);
                        ViewGroup parent = (ViewGroup) surfaceView.getParent();
                        if (parent != null) {
                            parent.removeView(surfaceView);
                        }
                        addView(surfaceView);
                        if (childrenStates != null) {
                            for (int i = 0; i < getChildCount(); i++) {
                                getChildAt(i).restoreHierarchyState(childrenStates);
                            }
                            childrenStates = null;
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                        android.util.Log.d("exception", e.toString());
                        if (promiseObj != null) {
                            promiseObj.property("reject").toFunction()
                                    .call(null, e.getMessage());
                        }
                    }
                }
            });
            /* This is necessary if the view has been restored */
            service.getProcess().addEventListener(new Process.EventListener() {
                public void onProcessStart(Process process, JSContext context) {}
                public void onProcessAboutToExit(Process process, int exitCode) {
                    detach(context);
                    process.removeEventListener(this);
                }
                public void onProcessExit(Process process, int exitCode) {
                    detach(context);
                    process.removeEventListener(this);
                }
                public void onProcessFailed(Process process, Exception error) {
                    detach(context);
                    process.removeEventListener(this);
                }
            });
        } catch (Exception e) {
            android.util.Log.d("exception", e.toString());
            if (promiseObj != null) {
                promiseObj.property("reject").toFunction()
                        .call(null, e.getMessage());
            }
            detach(context);
        }
        if (promiseObj != null) {
            return promiseObj.property("promise").toObject();
        } else {
            return null;
        }
    }

    private JSObject detach(@Nullable final JSContext context) {
        final JSObject promiseObj;
        if (context != null) promiseObj = context.evaluateScript(createPromiseObject).toObject();
        else promiseObj = null;
        surfaceId = NO_ID;
        if (attachedSurface != null && attachedSurface.length() > 0 &&
                surfaceHashMap.get(attachedSurface) != null) {
            String s = attachedSurface;
            attachedSurface = null;
            surfaceHashMap.get(s).detach();
        }
        if (surfaceView != null) {
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    removeView(surfaceView);
                    if (promiseObj != null) {
                        promiseObj.property("resolve").toFunction()
                                .call(null);
                    }
                }
            });
        } else if (promiseObj != null) {
            promiseObj.property("resolve").toFunction()
                    .call(null);
        }

        if (promiseObj != null) {
            return promiseObj.property("promise").toObject();
        } else {
            return null;
        }
    }

    public void addServiceStartListener(MicroService.ServiceStartListener listener) {
        if (!startListeners.contains(listener))
            startListeners.add(listener);
    }

    private ArrayList<MicroService.ServiceStartListener> startListeners = new ArrayList<>();

    /**
     * Starts a MicroService asynchronously.  In XML layout, this can be auto-started using
     * the "custom:liquidcore.URI" and "custom:liquidcore.argv" attributes.
     * @param uri  The MicroService URI
     * @param argv Optional arguments loaded into process.argv[2:]
     * @return the MicroService
     */
    public MicroService start(final URI uri, String ... argv) {
        if (getId() == View.NO_ID) {
            setId(generateViewIdCommon());
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
                        public void onProcessStart(Process process, final JSContext context) {
                            JSObject liquidCore = context.property("LiquidCore").toObject();
                            JSObject availableSurfaces = new JSObject(context);
                            for (int i=0; surfaceNames != null && surfaceVersions != null &&
                                    i<surfaceNames.size(); i++){
                                availableSurfaces.property(surfaceNames.get(i),
                                        surfaceVersions.get(i));
                            }
                            liquidCore.property("availableSurfaces", availableSurfaces);
                            liquidCore.property("bind", new JSFunction(context, "bind_") {
                               @SuppressWarnings("unused")
                               public JSObject bind_(String s, JSValue config) {
                                   return bind(context, s, config); }
                            });

                            for (MicroService.ServiceStartListener listener : startListeners) {
                                listener.onStart(service);
                            }
                            startListeners.clear();
                        }

                        @Override public void onProcessAboutToExit(Process process, int exitCode) {
                            process.removeEventListener(this);
                            for (String boundSurface : boundSurfaces) {
                                surfaceHashMap.remove(boundSurface);
                            }
                            boundSurfaces.clear();
                        }
                        @Override public void onProcessExit(Process process, int exitCode) {
                            process.removeEventListener(this);
                            for (String boundSurface : boundSurfaces) {
                                surfaceHashMap.remove(boundSurface);
                            }
                            boundSurfaces.clear();
                        }
                        @Override public void onProcessFailed(Process process, Exception error) {
                            process.removeEventListener(this);
                            for (String boundSurface : boundSurfaces) {
                                surfaceHashMap.remove(boundSurface);
                            }
                            boundSurfaces.clear();
                        }
                    });
                }
            },
            new MicroService.ServiceErrorListener() {
                @Override
                public void onError(MicroService service, Exception e) {
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

    /**
     * Makes a Surface available to the MicroService.  Must be called prior to start().  In XML
     * layout, an array of available surfaces can be provided using the "custom:liquidcore.surface"
     * attribute.
     * @param canonicalName The canonical name of the surface to enable for this view
     */
    public void enableSurface(String canonicalName) {
        if (surfaces == registeredSurfaces) {
            surfaces = new ArrayList<>();
        }
        for (MicroService.AvailableSurface surface : registeredSurfaces) {
            if (surface.cls.getCanonicalName().equalsIgnoreCase(canonicalName)) {
                surfaces.add(surface);
                return;
            }
        }
    }

    /**
     * Registers a `Surface` class with LiquidCore.
     * @param cls Must be an instantiable `View` subclass that implements the `Surface` interface.
     */
    public static void registerSurface(Class<? extends Surface> cls) {
        try {
            Field f = cls.getDeclaredField("SURFACE_VERSION");
            MicroService.AvailableSurface as =
                    new MicroService.AvailableSurface(cls,f.get(null).toString());
            registeredSurfaces.add(as);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static ArrayList<MicroService.AvailableSurface> registeredSurfaces = new ArrayList<>();
    private ArrayList<MicroService.AvailableSurface> surfaces = registeredSurfaces;

    private MicroService.AvailableSurface[] availableSurfaces() {
        return surfaces.toArray(new MicroService.AvailableSurface[0]);
    }

    /* -- statics -- */
    private static HashMap<String,Surface> surfaceHashMap = new HashMap<>();

    /* -- local privates -- */
    private URI uri;
    private String [] argv;
    private View surfaceView;

    /* -- parcelable privates -- */
    private int surfaceId = View.NO_ID;
    private ArrayList<String> surfaceNames = null;
    private ArrayList<String> surfaceVersions = null;
    private String serviceId;
    private SparseArray childrenStates;
    private ArrayList<String> boundSurfaces = new ArrayList<>();
    private String attachedSurface = null;
    private ArrayList<String> boundCanonicalSurfaces = new ArrayList<>();

    @Override @SuppressWarnings("unchecked")
    public Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedState ss = new SavedState(superState);
        ss.surfaceId = surfaceId;
        ss.surfaceNames = surfaceNames;
        ss.surfaceVersions = surfaceVersions;
        ss.serviceId = serviceId;
        ss.childrenStates = new SparseArray();
        for (int i = 0; i < getChildCount(); i++) {
            getChildAt(i).saveHierarchyState(ss.childrenStates);
        }
        ss.boundSurfaces = boundSurfaces;
        ss.attachedSurface = attachedSurface;
        ss.boundCanonicalSurfaces = boundCanonicalSurfaces;
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
            enableSurface(surface);
        }

        serviceId = ss.serviceId;
        childrenStates = ss.childrenStates;
        if (attachedSurface != null) {
            Surface s = surfaceHashMap.get(attachedSurface);
            attach(null, null, s);
        }
        if (ss.boundSurfaces == null) {
            boundSurfaces = new ArrayList<>();
        } else {
            boundSurfaces = new ArrayList<>(ss.boundSurfaces);
        }
        attachedSurface = ss.attachedSurface;
        if (ss.boundCanonicalSurfaces == null) {
            boundCanonicalSurfaces = new ArrayList<>();
        } else {
            boundCanonicalSurfaces = new ArrayList<>(ss.boundSurfaces);
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
        private List<String> boundSurfaces;
        private String attachedSurface;
        private List<String> boundCanonicalSurfaces;

        SavedState(Parcelable superState) {
            super(superState);
        }

        private SavedState(Parcel in, ClassLoader classLoader) {
            super(in);
            surfaceId = in.readInt();
            in.readStringList(surfaceNames);
            in.readStringList(surfaceVersions);
            serviceId = in.readString();
            childrenStates = in.readSparseArray(classLoader);
            in.readStringList(boundSurfaces);
            attachedSurface = in.readString();
            in.readStringList(boundCanonicalSurfaces);
        }

        @Override @SuppressWarnings("unchecked")
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeInt(surfaceId);
            out.writeStringList(surfaceNames);
            out.writeStringList(surfaceVersions);
            out.writeString(serviceId);
            out.writeSparseArray(childrenStates);
            out.writeStringList(boundSurfaces);
            out.writeString(attachedSurface);
            out.writeStringList(boundCanonicalSurfaces);
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

    private static String[] getClassesOfPackage(Context context) {
        ArrayList<String> classes = new ArrayList<>();
        final String packageName = "org.liquidplayer.surface";

        try {
            String packageCodePath = context.getPackageCodePath();
            DexFile df = new DexFile(packageCodePath);
            for (Enumeration<String> iter = df.entries(); iter.hasMoreElements(); ) {
                String className = iter.nextElement();
                if (className.contains(packageName)) {
                    classes.add(className);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return classes.toArray(new String[0]);
    }

    private static boolean isInit = false;
    @SuppressWarnings("unchecked")
    private static void registerSurfaces(Context context) {
        if (isInit) return;
        isInit = true;

        String [] classes = getClassesOfPackage(context);

        for (String cls : classes) {
            try {
                Class<?> klass = context.getClassLoader().loadClass(cls);
                if (Surface.class.isAssignableFrom(klass)) {
                    Class<? extends Surface> surfaceClass = (Class<? extends Surface>)klass;
                    registerSurface(surfaceClass);
                }
            } catch (ClassNotFoundException e) {
                // Nothing to do, just ignore it
            }
        }
    }
}
