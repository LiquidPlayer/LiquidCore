package org.liquidplayer.service;

import android.support.test.InstrumentationRegistry;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;
import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.node.Process;

import java.net.URI;
import java.util.concurrent.Semaphore;

import static org.junit.Assert.*;

public class MicroServiceTest {

    @Test
    public void testMicroService() throws Exception {
        class Consts {
            int port;
            int mask;
        }
        final Consts consts = new Consts();
        consts.mask = 0;
        final Semaphore waitForServer = new Semaphore(0);
        final Semaphore waitForClient = new Semaphore(0);
        final URI serverURI = URI.create("android.resource://" +
                InstrumentationRegistry.getContext().getPackageName() + "/raw/server");

        // First, start a MicroService from a file.  This service creates a small HTTP file server
        final MicroService server = new MicroService(InstrumentationRegistry.getContext(),serverURI,
        new MicroService.ServiceStartListener() {
            @Override
            public void onStart(MicroService service) {
                service.addEventListener("listening", new MicroService.EventListener() {
                    @Override
                    public void onEvent(MicroService service, String event, JSONObject payload){
                        try {
                            consts.port = payload.getInt("port");
                            waitForServer.release();
                        } catch (JSONException e) {
                            assertTrue(false);
                        }
                    }
                });
            }
        });
        server.start();
        waitForServer.acquire();
        waitForServer.release();

        final URI clientURI = URI.create("http://localhost:" + consts.port + "/hello.js");

        // Next, start a MicroService that is served from the server
        MicroService client = new MicroService(InstrumentationRegistry.getContext(), clientURI,
            new MicroService.ServiceStartListener() {
                @Override
                public void onStart(MicroService service) {
                    service.addEventListener("msg", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            try {
                                assertEquals("Hello, World!", payload.getString("msg"));
                                payload.put("msg", "Hallo die Weld!");
                                service.emit("js_msg", payload);
                            } catch (JSONException e) {
                                assertTrue(false);
                            }
                        }
                    });
                    service.addEventListener("null", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            assertNull(payload);
                            service.emit("js_null");
                        }
                    });
                    service.addEventListener("number", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            try {
                                assertEquals(5.2, payload.getDouble("_"), 0.1);
                                service.emit("js_number", 2.5);
                            } catch (JSONException e) {
                                assertTrue(false);
                            }
                        }
                    });
                    service.addEventListener("string", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            try {
                                assertEquals("foo", payload.getString("_"));
                                service.emit("js_string", "bar");
                            } catch (JSONException e) {
                                assertTrue(false);
                            }
                        }
                    });
                    service.addEventListener("boolean", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            try {
                                assertTrue(payload.getBoolean("_"));
                                service.emit("js_boolean", false);
                            } catch (JSONException e) {
                                assertTrue(false);
                            }
                        }
                    });
                    service.addEventListener("array", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            try {
                                JSONArray arr = payload.getJSONArray("_");
                                assertEquals(1,arr.getInt(0));
                                assertEquals("two",arr.getString(1));
                                assertTrue(arr.getBoolean(2));
                                JSONObject str = arr.getJSONObject(3);
                                assertEquals("bar", str.getString("str"));

                                JSONArray arr2 = new JSONArray();
                                arr2.put(5);
                                service.emit("js_array", arr2);
                            } catch (JSONException e) {
                                assertTrue(false);
                            }
                        }
                    });
                }
            },
            new MicroService.ServiceErrorListener() {
                @Override
                public void onError(MicroService service, Exception e) {
                    android.util.Log.e("client error", e.toString());
                    assertTrue(false);
                }
            },
            new MicroService.ServiceExitListener() {
                @Override
                public void onExit(MicroService service, Integer exitCode) {
                    assertEquals(0L, exitCode.longValue());
                    waitForClient.release();
                }
            }
        );

        server.getProcess().addEventListener(new Process.EventListener() {
            public void onProcessStart(Process process, JSContext context) {}
            public void onProcessAboutToExit(Process process, int exitCode) {}
            public void onProcessExit(Process process, int exitCode) {
                process.removeEventListener(this);
                waitForServer.release();
            }
            public void onProcessFailed(Process process, Exception error) {}
        });

        client.start();

        waitForClient.acquire();
        waitForServer.acquire();

        MicroService.uninstall(InstrumentationRegistry.getContext(),serverURI);
        MicroService.uninstall(InstrumentationRegistry.getContext(),clientURI);
    }
}