/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.servicetest;

import android.support.test.InstrumentationRegistry;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;
import org.liquidplayer.service.MicroService;

import java.net.URI;
import java.util.Date;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class MicroServiceTest {

    @Test
    public void testMicroService() throws Exception {
        class Consts {
            int port;
        }
        final Consts consts = new Consts();
        final CountDownLatch waitForServer = new CountDownLatch(1);
        final CountDownLatch waitForFinish = new CountDownLatch(2);

        ClassLoader loader = getClass().getClassLoader();
        assertNotNull(loader);
        String serverURIString = loader.getResource("server.js").toString();
        serverURIString = serverURIString.replace("jar:file:", "jarfile:");
        final URI serverURI = URI.create(serverURIString);

        // First, start a MicroService from a file.  This service creates a small HTTP file server
        final MicroService server = new MicroService(InstrumentationRegistry.getContext(), serverURI,
                new MicroService.ServiceStartListener() {
                    @Override
                    public void onStart(MicroService service) {
                        service.addEventListener("listening", new MicroService.EventListener() {
                            @Override
                            public void onEvent(MicroService service, String event, JSONObject payload) {
                                try {
                                    consts.port = payload.getInt("port");
                                    waitForServer.countDown();
                                } catch (JSONException e) {
                                    fail();
                                }
                            }
                        });
                    }
                },
                new MicroService.ServiceErrorListener() {
                    @Override
                    public void onError(MicroService service, Exception e) {
                        android.util.Log.e("ServiceError", e.toString());
                    }
                },
                new MicroService.ServiceExitListener() {
                    @Override
                    public void onExit(MicroService service, Integer exitCode) {
                        waitForFinish.countDown();
                    }
                });
        boolean hasThrown = false;
        server.start();
        try {
            server.start();
        } catch (MicroService.ServiceAlreadyStartedError e) {
            hasThrown = true;
        }
        assertTrue(hasThrown);

        assertTrue(waitForServer.await(10L, TimeUnit.SECONDS));

        final URI clientURI = URI.create("http://localhost:" + consts.port + "/hello.js");

        final MicroService.EventListener removeMe = new MicroService.EventListener() {
            @Override
            public void onEvent(MicroService service, String event, JSONObject payload) {
                fail();
            }
        };

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
                                fail();
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
                                fail();
                            }
                        }
                    });
                    service.addEventListener("integer", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            try {
                                assertEquals(24, payload.getInt("_"));
                                service.emit("js_integer", 42);
                            } catch (JSONException e) {
                                fail();
                            }
                        }
                    });
                    service.addEventListener("float", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            try {
                                assertEquals(16.3, payload.getDouble("_"), 0.1);
                                service.emit("js_float", 36.1f);
                            } catch (JSONException e) {
                                fail();
                            }
                        }
                    });
                    service.addEventListener("long", new MicroService.EventListener() {
                        @Override
                        public void onEvent(MicroService service, String event, JSONObject payload){
                            try {
                                assertEquals(123456789L, payload.getLong("_"));
                                service.emit("js_long", 987654321L);
                            } catch (JSONException e) {
                                fail();
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
                                fail();
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
                                fail();
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
                                fail();
                            }
                        }
                    });

                    service.addEventListener("removeMe", removeMe);
                    service.removeEventListener("removeMe", removeMe);
                    service.emit("removeMe");
                }
            },
            new MicroService.ServiceErrorListener() {
                @Override
                public void onError(MicroService service, Exception e) {
                    android.util.Log.e("client error", e.toString());
                    fail();
                }
            },
            new MicroService.ServiceExitListener() {
                @Override
                public void onExit(MicroService service, Integer exitCode) {
                    assertEquals(0L, exitCode.longValue());
                    waitForFinish.countDown();
                    server.getProcess().exit(0);
                }
            }
        );
        client.start();

        String id = client.getId();
        MicroService fromId = MicroService.getService(id);
        assertEquals(client, fromId);

        URI fromService = client.getServiceURI();
        assertEquals(clientURI.toString(), fromService.toString());

        assert(waitForFinish.await(10L, TimeUnit.SECONDS));
        waitForFinish.await();

        MicroService.uninstall(InstrumentationRegistry.getContext(),serverURI);
        MicroService.uninstall(InstrumentationRegistry.getContext(),clientURI);
    }

    private Date start, end;
    private volatile int result;

    @Test
    public void Issue36Test() throws Exception {
        final CountDownLatch waitForService = new CountDownLatch(1);

        ClassLoader loader = getClass().getClassLoader();
        assertNotNull(loader);
        String serverURIString = loader.getResource("promise.js").toString();
        serverURIString = serverURIString.replace("jar:file:", "jarfile:");
        final URI serverURI = URI.create(serverURIString);

        final MicroService promise = new MicroService(InstrumentationRegistry.getContext(), serverURI,
                new MicroService.ServiceStartListener() {
                    @Override
                    public void onStart(MicroService service) {
                        start = new Date();
                        service.addEventListener("promise", new MicroService.EventListener() {
                            @Override
                            public void onEvent(MicroService service, String event, JSONObject payload) {
                                try {
                                    result = payload.getInt("_");
                                    end = new Date();
                                    service.getProcess().exit(1);
                                } catch (JSONException e) {
                                    e.printStackTrace();
                                }
                            }
                        });
                    }
                },
                new MicroService.ServiceErrorListener() {
                    @Override
                    public void onError(MicroService service, Exception e) {
                        android.util.Log.e("ServiceError", e.toString());
                        waitForService.countDown();
                    }
                },
                new MicroService.ServiceExitListener() {
                    @Override
                    public void onExit(MicroService service, Integer exitCode) {
                        waitForService.countDown();
                    }
                });
        promise.start();
        waitForService.await(10L, TimeUnit.SECONDS);
        assertEquals(123L, result);
        android.util.Log.d("Issue36Test", "Time to promise resolution is "
                + (end.getTime() - start.getTime()) + " ms");
        assertTrue(end.getTime() - start.getTime() < 4000);
    }

    @Test
    public void testAndroidAssetPromise() throws Exception {
        final CountDownLatch waitForService = new CountDownLatch(1);

        final URI serverURI = URI.create("file:///android_asset/promise.js");

        final MicroService promise = new MicroService(InstrumentationRegistry.getContext(), serverURI,
                new MicroService.ServiceStartListener() {
                    @Override
                    public void onStart(MicroService service) {
                        start = new Date();
                        service.addEventListener("promise", new MicroService.EventListener() {
                            @Override
                            public void onEvent(MicroService service, String event, JSONObject payload) {
                                try {
                                    result = payload.getInt("_");
                                    end = new Date();
                                    service.getProcess().exit(1);
                                } catch (JSONException e) {
                                    e.printStackTrace();
                                }
                            }
                        });
                    }
                },
                new MicroService.ServiceErrorListener() {
                    @Override
                    public void onError(MicroService service, Exception e) {
                        android.util.Log.e("ServiceError", e.toString());
                        waitForService.countDown();
                    }
                },
                new MicroService.ServiceExitListener() {
                    @Override
                    public void onExit(MicroService service, Integer exitCode) {
                        waitForService.countDown();
                    }
                });
        promise.start();
        waitForService.await(10L, TimeUnit.SECONDS);
        assertEquals(123L, result);
        android.util.Log.d("testAndroidAssetPromise", "Time to promise resolution is "
                + (end.getTime() - start.getTime()) + " ms");
        assertTrue(end.getTime() - start.getTime() < 4000);
    }

    @Test
    public void testDevServer() {
        URI devServer = MicroService.DevServer();
        assertEquals("http://10.0.2.2:8082/liquid.bundle?platform=android&dev=true", devServer.toString());

        devServer = MicroService.DevServer("foo", 8888);
        assertEquals("http://10.0.2.2:8888/foo.bundle?platform=android&dev=true", devServer.toString());

        devServer = MicroService.DevServer("foo.js", null);
        assertEquals("http://10.0.2.2:8082/foo.bundle?platform=android&dev=true", devServer.toString());

        devServer = MicroService.DevServer(null, 8888);
        assertEquals("http://10.0.2.2:8888/liquid.bundle?platform=android&dev=true", devServer.toString());

        devServer = MicroService.DevServer(null, null);
        assertEquals("http://10.0.2.2:8082/liquid.bundle?platform=android&dev=true", devServer.toString());
    }
}