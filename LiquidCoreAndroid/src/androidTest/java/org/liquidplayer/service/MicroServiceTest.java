package org.liquidplayer.service;

import android.support.test.InstrumentationRegistry;

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
        }
        final Consts consts = new Consts();
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
                                android.util.Log.d("testMicroService", payload.getString("msg"));
                                assertEquals("Hello, World!", payload.getString("msg"));
                                server.getProcess().exit(0);
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
                public void onExit(MicroService service) {
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