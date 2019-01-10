/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.service;

import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.support.annotation.Nullable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSException;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSON;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.BuildConfig;
import org.liquidplayer.node.Process;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.InvocationTargetException;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URL;
import java.net.URLEncoder;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;
import java.util.UUID;
import java.util.zip.GZIPInputStream;

/**
 * A MicroService is the basic building block of LiquidCore.  It encapsulates the runtime
 * environment for a client-side micro app.  A MicroService is a complete virtual machine
 * whose operation is defined by the code referenced by the service URI.  When a MicroService
 * is instantiated, its Node.js environment is set up, its code downloaded (or fetched from cache)
 * from the URI, and is executed in a VM.  The host may interact with the VM via a simple
 * message-based API, or a UI may be exposed by attaching to a Surface.
 */
public class MicroService implements Process.EventListener {

    /**
     * Listens for a specific event emitted by the MicroService
     */
    public interface EventListener {
        /**
         * Called asynchronously when the MicroService javascript code calls 'LiquidCore.emit()'
         * @param service  The MicroService which emitted the event
         * @param event  The event id
         * @param payload  A JSON data object emitted by the MicroService
         */
        void onEvent(MicroService service, String event, JSONObject payload);
    }

    /**
     * Listens for when the MicroService has been inititialized and the environment is ready
     * to receive event listeners.  This is called after the Node.js environment has been set
     * up, but before the MicroService code has been executed.  It is safe to add any event
     * listeners here, but emitted events will not be seen by the MicroService until its code
     * has been run.  The MicroService should emit an event to let the host know that it is ready
     * to receive events.
     */
    public interface ServiceStartListener {
        /**
         * Called after the environment is set up, but before the MicroService javascript is
         * executed.
         * @param service  The MicroService which is now started
         */
        void onStart(MicroService service);
    }

    /**
     * Listens for when the MicroService has exited gracefully.  The MicroService is no longer
     * available and is shutting down.
     */
    public interface ServiceExitListener {
        /**
         * Called immediately before the MicroService exits.  This is a graceful exit, and is
         * mutually exclusive with the ServiceErrorListener.  Only one of either the exit listener
         * or error listener will be called from any MicroService.
         * @param service  The MicroService which is shutting down
         * @param exitCode The code returned by the Node.js process
         */
        void onExit(MicroService service, Integer exitCode);
    }

    /**
     * Listens for any errors that may cause the MicroService to shut down unexpectedly.  The
     * MicroService is no longer available and may have already crashed.
     */
    public interface ServiceErrorListener {
        /**
         * Called upon an exception state.  This is an unexpected exit, and is
         * mutually exclusive with the ServiceExitListener.  Only one of either the exit listener
         * or error listener will be called from any MicroService.
         * @param service  The MicroService which failed
         * @param e  The thrown exception
         */
        void onError(MicroService service, Exception e);
    }

    /**
     * This runtime exception is thrown if an attempt is made to re-start a MicroService if it
     * has already been started.  A MicroService instance may be executed only once.
     */
    public class ServiceAlreadyStartedError extends RuntimeException {
    }

    /**
     * Generates a URI for fetching from a development server on the loopback address (10.0.2.2).
     * @param fileName The name of the bundled javascript file to fetch (default: liquid.js)
     * @param port The server's port (default: 8082)
     * @return A service URI for use in the MicroService constructor
     */
    public static URI DevServer(@Nullable String fileName, @Nullable Integer port) {
        if (fileName == null) {
            fileName = "liquid.js";
        }
        if (port == null) {
            port = 8082;
        }
        if (fileName.endsWith(".js"))
            fileName = fileName.substring(0,fileName.length()-3);
        if (!fileName.endsWith((".bundle"))) {
            fileName = fileName.concat(".bundle");
        }
        URI loopback = URI.create("http://10.0.2.2:" + port + "/" + fileName + "?platform=android&dev=true");
        return loopback;
    }

    /**
     * Generates a URI for fetching from a development server on the loopback address (10.0.2.2).
     * Assumes the entry js file is 'liquid.js' and is served from port 8082 on the emulator's host
     * machine.
     * @return A service URI for use in the MicroService constructor
     */
    public static URI DevServer() {
        return DevServer(null, null);
    }

    private ServiceStartListener startListener;
    private ServiceExitListener exitListener;
    private ServiceErrorListener errorListener;
    private final URI serviceURI;
    private String serviceId;
    private final Context androidCtx;
    private final String uuid;
    private String [] argv;
    private String module;
    private JSObject emitter;
    private JSFunction safeStringify;
    private boolean started = false;
    private Process process;

    static class AvailableSurface {
        Class<? extends Surface> cls;
        String version;
        AvailableSurface(Class <? extends Surface> cls, String version) {
            this.cls = cls;
            this.version = version;
        }
    }
    private AvailableSurface [] availableSurfaces = new AvailableSurface[0];
    void setAvailableSurfaces(AvailableSurface[] availableSurfaces) {
        this.availableSurfaces = availableSurfaces;
    }

    /**
     * Creates a new instance of the MicroService referenced by serviceURI
     * @param ctx  The android context of this app
     * @param serviceURI  The URI (can be a network URL or local file/resource) of the MicroService
     *                    code
     * @param start  The ServiceStartListener (optional)
     * @param error  The ServiceErrorListener (optional)
     * @param exit   The ServiceExitListener (optional)
     */
    public MicroService(Context ctx, URI serviceURI, ServiceStartListener start,
                        ServiceErrorListener error, ServiceExitListener exit) {
        this.serviceURI = serviceURI;
        try {
            this.serviceId = URLEncoder.encode(
                serviceURI.toString().substring(0,serviceURI.toString().lastIndexOf('/')), "UTF-8");
        } catch (UnsupportedEncodingException e) {
            android.util.Log.e("MicrosService", e.toString());
        }
        this.startListener = start;
        this.exitListener = exit;
        this.errorListener = error;
        this.androidCtx = ctx;
        uuid = UUID.randomUUID().toString();
        serviceMap.put(uuid, this);
    }
    /**
     * Creates a new instance of the MicroService referenced by serviceURI
     * @param ctx  The android context of this app
     * @param serviceURI  The URI (can be a network URL or local file/resource) of the MicroService
     *                    code
     * @param start  The ServiceStartListener (optional)
     * @param error  The ServiceErrorListener (optional)
     */
    public MicroService(Context ctx, URI serviceURI, ServiceStartListener start,
                        ServiceErrorListener error) {
        this(ctx,serviceURI,start,error,null);
    }
    /**
     * Creates a new instance of the MicroService referenced by serviceURI
     * @param ctx  The android context of this app
     * @param serviceURI  The URI (can be a network URL or local file/resource) of the MicroService
     *                    code
     * @param start  The ServiceStartListener (optional)
     */
    public MicroService(Context ctx, URI serviceURI, ServiceStartListener start) {
        this(ctx,serviceURI,start,null);
    }
    /**
     * Creates a new instance of the MicroService referenced by serviceURI
     * @param ctx  The android context of this app
     * @param serviceURI  The URI (can be a network URL or local file/resource) of the MicroService
     *                    code
     */
    public MicroService(Context ctx, URI serviceURI) {
        this(ctx,serviceURI,null);
    }

    private final Map<String, Map<EventListener, JSFunction>> listeners =
            Collections.synchronizedMap(new HashMap<String,Map<EventListener,JSFunction>>());
    private final Object listenersMutex = new Object();

    /**
     * Adds an event listener for an event triggered by 'LiquidCore.emit(event, payload)' in
     * JavaScript.  Example:<br/>
     * <code>
     *     LiquidCore.emit('my_event', { stringData: 'foo', bar : 6 });<br/>
     * </code>
     * This will trigger the 'listener' added here, with the JavaScript object represented as a
     * JSONObject payload.
     * @param event  The String event id
     * @param listener  The event listener
     */
    public void addEventListener(final String event, final EventListener listener) {
        final String safeStringifyBody = "" +
            "   function serializer(replacer, cycleReplacer) {\n" +
            "       var stack = [], keys = []\n" +
            "       if (cycleReplacer == null) cycleReplacer = function(key, value) {\n"+
            "           if (stack[0] === value) return '[Circular ~]'\n"+
            "           return'[Circular ~.' + keys.slice(0, stack.indexOf(value)).join('.')+']'\n"+
            "       }\n"+
            "       return function(key, value) {\n"+
            "           if (stack.length > 0) {\n"+
            "               var thisPos = stack.indexOf(this)\n"+
            "               ~thisPos ? stack.splice(thisPos + 1) : stack.push(this)\n"+
            "               ~thisPos ? keys.splice(thisPos, Infinity, key) : keys.push(key)\n"+
            "               if (~stack.indexOf(value))value=cycleReplacer.call(this, key, value)\n"+
            "           }\n"+
            "           else stack.push(value)\n"+
            "           return replacer == null ? value : replacer.call(this, key, value)\n"+
            "       }\n"+
            "   }\n"+
            "   return JSON.stringify(obj, serializer(replacer, cycleReplacer), spaces)\n";

        if (emitter != null) {
            if (safeStringify == null) {
                safeStringify = new JSFunction(emitter.getContext(), "safeStringify",
                        safeStringifyBody,
                        "obj", "replacer", "spaces", "cycleReplacer");
            }
            JSFunction jsListener = new JSFunction(emitter.getContext(), "on") {
                @SuppressWarnings("unused")
                public void on(JSValue value) throws JSONException {
                    JSONObject o = null;
                    if (value != null && !value.isNull() && !value.isUndefined()) {
                        if (value.isArray()) {
                            JSONArray a = new JSONArray(safeStringify.call(null, value).toString());
                            o = new JSONObject();
                            o.put("_", a);
                        } else if (value.isObject() && !value.isDate()) {
                            o = new JSONObject(safeStringify.call(null,value).toString());
                        } else {
                            o = new JSONObject();
                            o.put("_", value.toString());
                        }
                    }
                    listener.onEvent(MicroService.this, event, o);
                }
            };
            synchronized (listenersMutex) {
                Map<EventListener, JSFunction> eventMap = listeners.get(event);
                if (eventMap == null) {
                    eventMap = Collections.synchronizedMap(new HashMap<EventListener,JSFunction>());
                    listeners.put(event, eventMap);
                }
                eventMap.put(listener, jsListener);
            }
            emitter.property("on").toFunction().call(emitter,event,jsListener);
        }
    }

    /**
     * Each MicroService instance is mapped to a unique String id.  This id can be serialized
     * in UIs and the instance retrieved by a call to MicroService.getService(id)
     * @return  The unique id of this instance
     */
    public String getId() {
        return uuid;
    }

    /**
     * Returns the Node.js Process object.  This is only intended to be used by Surfaces, and
     * directly accessing the Process is not recommended.  May deprecate in the future, so don't
     * get used to it.
     * @return  The Node.js Process object
     */
    public Process getProcess() {
        return process;
    }

    /**
     * Returns the URI from which the service was started.
     * @return The URI of the service
     */
    public URI getServiceURI() { return serviceURI; }

    /**
     * Removes an EventListener previously added with addEventListener.
     * @param event  The event for which to unregister the listener
     * @param listener  The listener to unregister
     */
    public void removeEventListener(String event, final EventListener listener) {
        Map<EventListener, JSFunction> eventMap;
        synchronized (listenersMutex) {
            eventMap = listeners.get(event);
        }
        if (eventMap != null) {
            JSFunction func;
            synchronized (listenersMutex) {
                func = eventMap.get(listener);
                if (func != null) {
                    eventMap.remove(listener);
                    if (eventMap.size() == 0) {
                        listeners.remove(event);
                    }
                }
            }
            if (emitter != null) {
                emitter.property("removeListener").toFunction().call(emitter, event, func);
            }
        }
    }

    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function(payload) {<br/>
     *        // Do something with the payload data<br/>
     *        console.log(payload.hello);<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     JSONObject foo = new JSONObject();<br/>
     *     foo.putString("hello", "world");<br/>
     *     myService.emit("my_event", foo);<br/>
     * </code><br/>
     * @param event  The event to trigger
     * @param payload  The JSONObject data structure to emit
     */
    public void emit(String event, JSONObject payload) {
        if (emitter != null) {
            JSValue o;
            if (payload != null) {
                o = JSON.parse(emitter.getContext(), payload.toString());
            } else {
                o = new JSValue(emitter.getContext());
            }
            emitter.property("emit").toFunction().call(emitter, event, o);
        }
    }
    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function() {<br/>
     *        console.log('Received my_event');<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     myService.emit("my_event");<br/>
     * </code><br/>
     * @param event  The event to trigger
     */
    public void emit(String event) {
        emit(event, (JSONObject) null);
    }
    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function(val) {<br/>
     *        console.log('Received: ' + val);<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     myService.emit("my_event", 5);<br/>
     * </code><br/>
     * @param event  The event to trigger
     * @param v value to send
     */
    public void emit(String event, Integer v) {
        if (emitter != null) {
            emitter.property("emit").toFunction().call(emitter, event, v);
        }
    }
    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function(val) {<br/>
     *        console.log('Received: ' + val);<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     myService.emit("my_event", 5L);<br/>
     * </code><br/>
     * @param event  The event to trigger
     * @param v value to send
     */
    public void emit(String event, Long v) {
        if (emitter != null) {
            emitter.property("emit").toFunction().call(emitter, event, v);
        }
    }
    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function(val) {<br/>
     *        console.log('Received: ' + val);<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     myService.emit("my_event", 5.2f);<br/>
     * </code><br/>
     * @param event  The event to trigger
     * @param v value to send
     */
    public void emit(String event, Float v) {
        if (emitter != null) {
            emitter.property("emit").toFunction().call(emitter, event, v);
        }
    }
    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function(val) {<br/>
     *        console.log('Received: ' + val);<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     myService.emit("my_event", 15.6);<br/>
     * </code><br/>
     * @param event  The event to trigger
     * @param v value to send
     */
    public void emit(String event, Double v) {
        if (emitter != null) {
            emitter.property("emit").toFunction().call(emitter, event, v);
        }
    }
    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function(val) {<br/>
     *        console.log('Received: ' + val);<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     myService.emit("my_event", "foo");<br/>
     * </code><br/>
     * @param event  The event to trigger
     * @param v value to send
     */
    public void emit(String event, String v) {
        if (emitter != null) {
            emitter.property("emit").toFunction().call(emitter, event, v);
        }
    }
    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function(val) {<br/>
     *        console.log('Received: ' + val);<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     myService.emit("my_event", true);<br/>
     * </code><br/>
     * @param event  The event to trigger
     * @param v value to send
     */
    public void emit(String event, Boolean v) {
        if (emitter != null) {
            emitter.property("emit").toFunction().call(emitter, event, v);
        }
    }
    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidCore.on('my_event', function(val) {<br/>
     *        console.log('Received: ' + val);<br/>
     *     });<br/>
     * </code><br/>
     * On the Java side:<br/>
     * <code>
     *     JSONArray a = new JSONArray();
     *     a.put(0);
     *     a.put("two");
     *     myService.emit("my_event", a);<br/>
     * </code><br/>
     * @param event  The event to trigger
     * @param v value to send
     */
    public void emit(String event, JSONArray v) {
        if (emitter != null) {
            JSValue o;
            if (v != null) {
                o = JSON.parse(emitter.getContext(), v.toString());
            } else {
                o = new JSValue(emitter.getContext());
            }
            emitter.property("emit").toFunction().call(emitter, event, o);
        }
    }

    /**
     * Starts the MicroService.  This method will return immediately and initialization and
     * startup will occur asynchronously in a separate thread.  It will download the code from
     * the service URI (if not cached), set the arguments in `process.argv` and execute the script.
     * @param argv  The list of arguments to sent to the MicroService.  This is similar to running
     *              node from a command line. The first two arguments will be the application (node)
     *              followed by the local module code (/home/module/[service.js].  'argv' arguments
     *              will then be appended in process.argv[2:]
     */
    public synchronized void start(String ... argv) {
        if (started) throw new ServiceAlreadyStartedError();
        started = true;
        this.argv = argv;
        process = new Process(androidCtx, serviceId, Process.kMediaAccessPermissionsRW,
                MicroService.this);
    }

    /**
     * Uninstalls the MicroService from this host, and removes any global data associated with the
     * service
     * @param ctx The Android context
     * @param serviceURI The URI of the service (should be the same URI that the service was started
     *                   with)
     */
    public static void uninstall(Context ctx, URI serviceURI) {
        try {
            String serviceId = URLEncoder.encode(
                serviceURI.toString().substring(0,serviceURI.toString().lastIndexOf('/')), "UTF-8");
            Process.uninstall(ctx,serviceId, Process.UninstallScope.Global);
        } catch (UnsupportedEncodingException e) {
            android.util.Log.e("MicrosService", e.toString());
        }
    }

    private File getModulePath() {
        final String suffix = "/__org.liquidplayer.node__/_" + serviceId;
        String modules = androidCtx.getFilesDir().getAbsolutePath() + suffix + "/module";
        return new File(modules);
    }

    // FIXME: We want to use the symlinked version so that we are only capturing those modules exposed to this service
    private File getNodeModulesPath() {
        String node_modules = androidCtx.getFilesDir().getAbsolutePath() +
                "/__org.liquidplayer.node__/node_modules";
        return new File(node_modules);
    }

    private void fetchService() throws IOException {
        String path = serviceURI.getPath();
        module = path.substring(path.lastIndexOf('/') + 1);
        if (!module.endsWith(".js")) {
            module = module + ".js";
        }
        File moduleF = new File(getModulePath().getAbsolutePath() + "/" + module);
        long lastModified = moduleF.lastModified();

        String scheme = serviceURI.getScheme();
        InputStream in = null;
        if ("http".equals(scheme) || "https".equals(scheme)) {
            URL url = serviceURI.toURL();
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setReadTimeout(10000);
            if (lastModified > 0) {
                SimpleDateFormat sdf =
                        new SimpleDateFormat("EEE, d MMM yyyy HH:mm:ss", Locale.US);
                sdf.setTimeZone(TimeZone.getTimeZone("UTC"));
                connection.setRequestProperty("If-Modified-Since",
                        sdf.format(new Date(lastModified)) + " GMT");
            }
            connection.setRequestProperty("Accept-Encoding", "gzip");
            String version = BuildConfig.VERSION_NAME;
            String info = "Android; API " + Build.VERSION.SDK_INT;
            String bindings = "";
            for (File binding : getNodeModulesPath().listFiles()) {
                if (binding.isDirectory()) {
                    if (!"".equals(bindings)) {
                        bindings += "; ";
                    }
                    bindings += binding.getName();
                }
            }
            String surfaces = "";
            for (AvailableSurface surface : availableSurfaces) {
                if (!"".equals(surfaces)) {
                    surfaces += "; ";
                }
                surfaces += surface.cls.getCanonicalName();
                if (surface.version != null) {
                    surfaces += "/" + surface.version;
                }
            }
            String userAgent = "LiquidCore/" + version + " (" + info + ")";
            if (!"".equals(surfaces)) {
                userAgent += " Surface (" + surfaces + ")";
            }
            if (!"".equals(bindings)) {
                userAgent += " Binding (" + bindings + ")";
            }
            android.util.Log.d("MicroService", "User-Agent : " + userAgent);
            connection.setRequestProperty("User-Agent", userAgent);
            int responseCode = connection.getResponseCode();
            if (responseCode == 200) {
                if (connection.getHeaderField("Content-Encoding") != null &&
                        connection.getHeaderField("Content-Encoding").equals("gzip")) {
                    in = new GZIPInputStream(connection.getInputStream());
                } else {
                    in = connection.getInputStream();
                }
            } else if (responseCode != 304) { // 304 just means the file has not changed
                android.util.Log.e("FileNotFound", "responseCode = " + responseCode);
                throw new FileNotFoundException();
            }
        } else if ("jarfile".equals(scheme)) {
            int loc = serviceURI.getPath().lastIndexOf("/");
            in = getClass().getClassLoader().getResourceAsStream(serviceURI.getPath().substring(loc+1));
        } else {
            if ("file".equals(scheme) && path.startsWith("/android_asset/")) {
                // Open javascript file located in the android asset directory.
                in = androidCtx.getAssets().open(path.substring(15));
            } else {
                in = androidCtx.getContentResolver().openInputStream(Uri.parse(serviceURI.toString()));
            }
        }
        if (in != null) {
            // Write file to /home/module
            OutputStream out = new FileOutputStream(moduleF);
            byte[] buf = new byte[16 * 1024];
            int len;
            while((len=in.read(buf))>0){
                out.write(buf,0,len);
            }
            out.close();
            in.close();
        }
    }

    private static String canon(String name) {
        name = name.replaceAll("[^a-zA-Z0-9]", "*");
        StringBuilder canonical = new StringBuilder();
        boolean nextCapital = true;
        for (int i=0; i<name.length(); i++) {
            if (name.substring(i,i+1).equals("*")) {
                nextCapital = true;
            } else {
                char ch = name.charAt(i);
                if (nextCapital && ch >= 'a' && ch <= 'z') {
                    ch = (char)('A' + ch - 'a');
                }
                nextCapital = false;
                canonical.append(ch);
            }
        }
        return canonical.toString();
    }

    @SuppressWarnings("unchecked")
    private JSValue bindings(JSContext context, String module, JSFunction require) {
        final int fndx = module.lastIndexOf('/');
        final String fname = module.substring(fndx+1);
        final int endx = fname.lastIndexOf('.');
        final String ext = (endx >= 0) ? fname.substring(endx+1) : "";
        final String moduleName = (endx<0) ? fname : fname.substring(0,endx);

        if (!("node".equals(ext))) {
            return require.call(null,module);
        }

        Class<? extends AddOn> addOnClass = AddOn.class;
        try {
            Class klass = context.getClass().getClassLoader()
                    .loadClass("org.liquidplayer.addon." + canon(moduleName));

            if (AddOn.class.isAssignableFrom(klass)) {
                addOnClass = (Class<? extends AddOn>) klass;
                AddOn addOn = addOnClass.getConstructor(Context.class).newInstance(androidCtx);
                addOn.register(moduleName);

                context.evaluateScript("fs.writeFileSync('/home/temp/" + fname + "', '')");
                JSValue binding = require.call(null, "/home/temp/" + fname);
                addOn.require(binding);
                return binding;
            } else {
                android.util.Log.e("LiquidCore.bindings", "Class " +
                        klass.getCanonicalName() +
                        " does not implement org.liquidplayer.service.AddOn");
            }
        } catch (JSException e) {
            android.util.Log.e("LiquidCore.bindings", e.stack());
        } catch (ClassNotFoundException e) {
            android.util.Log.e("bindings", "Class org.liquidplayer.addon." + moduleName +
                    " not found.");
        } catch (NoSuchMethodException e) {
            android.util.Log.e("LiquidCore AddOn", "" + addOnClass.getCanonicalName() +
                    " must have a default constructor.");
        } catch (IllegalAccessException e) {
            android.util.Log.e("LiquidCore AddOn", "" + addOnClass.getCanonicalName() +
                    " must be public.");
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (InstantiationException e) {
            e.printStackTrace();
        }

        return new JSValue(context);
    }

    @Override
    public void onProcessStart(Process process, final JSContext context) {
        // Create LiquidCore EventEmitter
        context.evaluateScript(
                "(()=>{\n" +
                "   class LiquidCore extends require('events') {}\n" +
                "   global.LiquidCore = new LiquidCore();\n" +
                "   return global.LiquidCore;" +
                "})()"
        );
        emitter = context.property("LiquidCore").toObject();

        // Override require() function to handle module binding
        final JSValue require = context.property("require");
        JSFunction bindings = new JSFunction(context,"bindings") {
            @SuppressWarnings("unused") public JSValue bindings(String module) {
                return MicroService.this.bindings(context, module, require.toFunction());
            }
        };
        bindings.prototype(require);
        context.property("require", bindings);

        try {
            fetchService();

            // Notify host that service is ready to accept event listeners
            if (startListener != null) {
                startListener.onStart(this);
                startListener = null;
            }

            // Construct process.argv
            ArrayList<String> args = new ArrayList<>();
            args.add("node");
            args.add("/home/module/" + module);
            if (argv != null) {
                args.addAll(Arrays.asList(argv));
            }
            context.property("process").toObject().property("argv", args);

            // Execute code
            final String script =
                    "(()=>{" +
                    "  const fs = require('fs'), vm = require('vm'); " +
                    "  (new vm.Script(fs.readFileSync('/home/module/" + module + "'), "+
                    "     {filename: '" + module + "'} )).runInThisContext();" +
                    "})()";
            try {
                context.evaluateScript(script);
            } catch (JSException e) {
                android.util.Log.e("JSEXCEPTION", "Unhandled exception: " + e.getError().toString());
                android.util.Log.e("JSEXCEPTION", e.getError().stack());
            }

        } catch (IOException e) {
            e.printStackTrace();
            onProcessFailed(null, e);
        }
    }

    static final private Map<String,MicroService> serviceMap =
            Collections.synchronizedMap(new HashMap<String,MicroService>());
    static final private Object serviceMapMutex = new Object();

    @Override
    public void onProcessAboutToExit(Process process, int exitCode) {
        if (exitListener != null) {
            exitListener.onExit(this, exitCode);
        }
        exitListener = null;
        errorListener = null;
        emitter = null;
        synchronized (serviceMapMutex) {
            for (Map.Entry<String, MicroService> entry : serviceMap.entrySet()) {
                if (entry.getValue() == this) {
                    serviceMap.remove(entry.getKey());
                    process.removeEventListener(this);
                    break;
                }
            }
        }
        this.process = null;
    }

    @Override
    public void onProcessExit(Process process, int exitCode) {
        if (exitListener != null) {
            exitListener.onExit(this, exitCode);
        }
        exitListener = null;
        errorListener = null;
        emitter = null;
        synchronized (serviceMapMutex) {
            for (Map.Entry<String, MicroService> entry : serviceMap.entrySet()) {
                if (entry.getValue() == this) {
                    serviceMap.remove(entry.getKey());
                    process.removeEventListener(this);
                    break;
                }
            }
        }
        this.process = null;
    }

    @Override
    public void onProcessFailed(Process process, Exception error) {
        if (errorListener != null) {
            errorListener.onError(this, error);
        }
        exitListener = null;
        errorListener = null;
        emitter = null;
        synchronized (serviceMapMutex) {
            for (Map.Entry<String, MicroService> entry : serviceMap.entrySet()) {
                if (entry.getValue() == this) {
                    serviceMap.remove(entry.getKey());
                    if (process != null) {
                        process.removeEventListener(this);
                    }
                    break;
                }
            }
        }
        this.process = null;
    }

    /**
     * Each MicroService instance is mapped to a unique String id.  This id can be serialized
     * in UIs and the instance retrieved by a call to MicroService.getService(id)
     * @param id  An id returned by MicroService.getId()
     * @return  The associated MicroService or null if no such service is active
     */
    public static MicroService getService(String id) {
        synchronized (serviceMapMutex) {
            return serviceMap.get(id);
        }
    }

}
