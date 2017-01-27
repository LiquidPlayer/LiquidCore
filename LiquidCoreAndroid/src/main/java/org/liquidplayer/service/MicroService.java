//
// MicroService.java
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
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URL;
import java.net.URLEncoder;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
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
         * Called asynchronously when the MicroService javascript code calls 'LiquidEvents.emit()'
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
         */
        void onExit(MicroService service);
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

    private HashMap<String, HashMap<EventListener, JSFunction>> listeners =
            new HashMap<>();

    /**
     * Adds an event listener for an event triggered by 'LiquidEvents.emit(event, payload)' in
     * JavaScript.  Example:<br/>
     * <code>
     *     LiquidEvents.emit('my_event', { stringData: 'foo', bar : 6 });<br/>
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
     * Removes an EventListener previously added with addEventListener.
     * @param event  The event for which to unregister the listener
     * @param listener  The listener to unregister
     */
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

    /**
     * Emits an event that can be received by the JavaScript code, if the MicroService has
     * registered a listener.  Example:<br/>
     * <code>
     *     LiquidEvents.on('my_event', function(payload) {<br/>
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
            JSValue o = JSON.parse(emitter.getContext(), payload.toString());
            emitter.property("emit").toFunction().call(emitter, event, o);
        }
    }

    /**
     * Starts the MicroService.  This method will return immediately and initialization and
     * startup will occur asynchronously in a separate thread.  It will download the code from
     * the service URI (if not cached), set the arguments in `process.argv` and execute the script.
     * @param argv  The list of arguments to sent to the MicroService.  This is similar to running
     *              node from a command line. The first two arguments will be the application (node)
     *              followed by the code reference (service URI).  'argv' arguments will then be
     *              appended in process.argv[1:]
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

    private void fetchService() throws IOException {
        // See if the file already exists
        File modules = getModulePath();
        if (modules == null) {
            throw new FileNotFoundException();
        }

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
                connection.setRequestProperty("Accept-Encoding", "gzip");
            }
            int responseCode = connection.getResponseCode();
            if (responseCode == 200) {
                if (connection.getHeaderField("Content-Encoding") != null &&
                        connection.getHeaderField("Content-Encoding").equals("gzip")) {
                    in = new GZIPInputStream(connection.getInputStream());
                } else {
                    in = connection.getInputStream();
                }
            } else if (responseCode != 304) { // 304 just means the file has not changed
                throw new FileNotFoundException();
            }
        } else {
            in = androidCtx.getContentResolver().openInputStream(Uri.parse(serviceURI.toString()));
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

    @Override
    public void onProcessStart(Process process, JSContext context) {
        // Create LiquidEvents EventEmitter
        context.evaluateScript(
                "class LiquidEvents_ extends require('events') {}\n" +
                "var LiquidEvents = new LiquidEvents_();"
        );
        emitter = context.property("LiquidEvents").toObject();

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
            args.addAll(Arrays.asList(argv));
            context.property("process").toObject().property("argv", args);

            // Execute code
            final String script =
                    "eval(String(require('fs').readFileSync('/home/module/" + module + "')))";
            context.evaluateScript(script);
        } catch (IOException e) {
            onProcessFailed(null, e);
        }
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
                if (process != null) {
                    process.removeEventListener(this);
                }
                break;
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
        return serviceMap.get(id);
    }

}
