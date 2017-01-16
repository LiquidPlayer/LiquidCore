package org.liquidplayer.service;

import android.content.Context;
import android.net.Uri;

import org.json.JSONException;
import org.json.JSONObject;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSON;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.Process;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Scanner;
import java.util.UUID;
import java.util.zip.GZIPInputStream;

public class MicroService implements Process.EventListener {

    public interface EventListener {
        void onEvent(MicroService service, String event, JSONObject payload);
    }
    public interface ServiceStartListener {
        void onStart(MicroService service);
    }
    public interface ServiceExitListener {
        void onExit(MicroService service);
    }
    public interface ServiceErrorListener {
        void onError(MicroService service, Exception e);
    }

    public class ServiceAlreadyStartedError extends RuntimeException {
    }

    private ServiceStartListener startListener;
    private ServiceExitListener exitListener;
    private ServiceErrorListener errorListener;
    private final URI serviceURI;
    private final Context androidCtx;
    private final String uuid;
    private String [] argv;
    private String code;
    private JSObject emitter;
    private JSFunction safeStringify;
    private boolean started = false;
    private Process process;

    public MicroService(Context ctx, URI serviceURI, ServiceStartListener start,
                        ServiceErrorListener error, ServiceExitListener exit) {
        this.serviceURI = serviceURI;
        this.startListener = start;
        this.exitListener = exit;
        this.errorListener = error;
        this.androidCtx = ctx;
        uuid = UUID.randomUUID().toString();
        serviceMap.put(uuid, this);
    }
    public MicroService(Context ctx, URI serviceURI, ServiceStartListener start,
                        ServiceErrorListener error) {
        this(ctx,serviceURI,start,error,null);
    }
    public MicroService(Context ctx, URI serviceURI, ServiceStartListener start) {
        this(ctx,serviceURI,start,null);
    }
    public MicroService(Context ctx, URI serviceURI) {
        this(ctx,serviceURI,null);
    }

    private HashMap<String, HashMap<EventListener, JSFunction>> listeners =
            new HashMap<>();

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
                    listener.onEvent(MicroService.this, event,
                            new JSONObject(safeStringify.call(null,value).toString()));
                }
            };
            HashMap<EventListener, JSFunction> eventMap = listeners.get(event);
            if (eventMap == null) {
                eventMap = new HashMap<>();
                listeners.put(event, eventMap);
            }
            eventMap.put(listener,jsListener);
            emitter.property("on").toFunction().call(emitter,event,jsListener);
        }
    }

    public String getId() {
        return uuid;
    }

    public Process getProcess() {
        return process;
    }

    public void removeEventListener(String event, final EventListener listener) {
        HashMap<EventListener, JSFunction> eventMap = listeners.get(event);
        if (eventMap != null) {
            JSFunction func = eventMap.get(listener);
            if (func != null) {
                eventMap.remove(listener);
                if (eventMap.size() == 0) {
                    listeners.remove(event);
                }
            }
            if (emitter != null) {
                emitter.property("removeListener").toFunction().call(emitter, event, func);
            }
        }
    }

    public void emit(String event, JSONObject payload) {
        if (emitter != null) {
            JSValue o = JSON.parse(emitter.getContext(), payload.toString());
            emitter.property("emit").toFunction().call(emitter, event, o);
        }
    }

    public synchronized void start(String ... argv) {
        if (started) throw new ServiceAlreadyStartedError();
        started = true;
        this.argv = argv;
        new Thread(startAsync).run();
    }

    public File getSharedPath() {
        File external = androidCtx.getExternalFilesDir(null);
        try {
            if (external != null) {
                String path = external.getAbsolutePath() +
                        "/LiquidPlayer/" + URLEncoder.encode(serviceURI.toString(), "UTF-8");
                return new File(path);
            }
        } catch (UnsupportedEncodingException e) {
            android.util.Log.e("getSharedPath", e.toString());
        }
        return null;
    }

    private Runnable startAsync = new Runnable() {
        @Override
        public void run() {
            try {
                String scheme = serviceURI.getScheme();
                InputStream in;
                if ("http".equals(scheme) || "https".equals(scheme)) {
                    URL url = serviceURI.toURL();
                    URLConnection connection = url.openConnection();
                    connection.setReadTimeout(10000);
                    if (connection.getHeaderField("Content-Encoding")!=null &&
                            connection.getHeaderField("Content-Encoding").equals("gzip")){
                        in = new GZIPInputStream(connection.getInputStream());
                    } else {
                        in = connection.getInputStream();
                    }
                    in.close();
                } else {
                    in = androidCtx.getContentResolver()
                            .openInputStream(Uri.parse(serviceURI.toString()));
                }
                if (in == null) {
                    throw new FileNotFoundException();
                }
                code = new Scanner(in, "UTF-8").useDelimiter("\\A").next();
                in.close();
                process = new Process(androidCtx, serviceURI.toString(),
                        Process.kMediaAccessPermissionsRW, MicroService.this);
            } catch (Exception e) {
                onProcessFailed(null,e);
            }
        }
    };

    @Override
    public void onProcessStart(Process process, JSContext context) {
        // Create LiquidEvents EventEmitter
        context.evaluateScript(
                "class LiquidEvents_ extends require('events') {}\n" +
                "var LiquidEvents = new LiquidEvents_();"
        );
        emitter = context.property("LiquidEvents").toObject();

        // Notify host that service is ready to accept event listeners
        if (startListener != null) {
            startListener.onStart(this);
            startListener = null;
        }

        // Construct process.argv
        ArrayList<String> args = new ArrayList<>();
        args.add("node");
        args.add(serviceURI.toString());
        args.addAll(Arrays.asList(argv));
        context.property("process").toObject().property("argv", args);

        // Execute code
        context.evaluateScript(code);
    }

    static private Map<String,MicroService> serviceMap = new HashMap<>();

    @Override
    public void onProcessAboutToExit(Process process, int exitCode) {
        if (exitListener != null) {
            exitListener.onExit(this);
        }
        exitListener = null;
        errorListener = null;
        emitter = null;
        for (Map.Entry<String,MicroService> entry : serviceMap.entrySet()) {
            if (entry.getValue() == this) {
                serviceMap.remove(entry.getKey());
                process.removeEventListener(this);
                break;
            }
        }
        this.process = null;
    }

    @Override
    public void onProcessExit(Process process, int exitCode) {
        if (exitListener != null) {
            exitListener.onExit(this);
        }
        exitListener = null;
        errorListener = null;
        emitter = null;
        for (Map.Entry<String,MicroService> entry : serviceMap.entrySet()) {
            if (entry.getValue() == this) {
                serviceMap.remove(entry.getKey());
                process.removeEventListener(this);
                break;
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
        for (Map.Entry<String,MicroService> entry : serviceMap.entrySet()) {
            if (entry.getValue() == this) {
                serviceMap.remove(entry.getKey());
                process.removeEventListener(this);
                break;
            }
        }
        this.process = null;
    }

    public static MicroService getService(String id) {
        return serviceMap.get(id);
    }

}
