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
        final Semaphore wait = new Semaphore(-1);

        // First, start a MicroService from a file.  This service creates a small HTTP file server
        final MicroService server = new MicroService(InstrumentationRegistry.getContext(),
                URI.create("android.resource://"+
                        InstrumentationRegistry.getContext().getPackageName()+ "/raw/server"));

        // Next, start a MicroService that is served from the server
        MicroService client = new MicroService(InstrumentationRegistry.getContext(),
            URI.create("http://localhost:9615/hello.js"),
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
                    assertTrue(false);
                }
            },
            new MicroService.ServiceExitListener() {
                @Override
                public void onExit(MicroService service) {
                    wait.release();
                }
            }
        );

        server.start();

        server.getProcess().addEventListener(new Process.EventListener() {
            public void onProcessStart(Process process, JSContext context) {}
            public void onProcessAboutToExit(Process process, int exitCode) {}
            public void onProcessExit(Process process, int exitCode) {
                wait.release();
            }
            public void onProcessFailed(Process process, Exception error) {}
        });

        client.start();

        wait.acquire();
    }
}