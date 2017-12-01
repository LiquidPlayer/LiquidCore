package org.liquidplayer.service;

import android.support.test.InstrumentationRegistry;

import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;

import java.net.URI;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class Sqlite3Test {
    private class Consts {
        int count;
    }

    private void testDatabase(final String fname) throws Exception {
        String testURIString = getClass().getClassLoader().getResource("sqlite3test.js").toString();
        testURIString = testURIString.replace("jar:file:", "jarfile:");
        final URI testURI = URI.create(testURIString);
        final CountDownLatch waitToEnd = new CountDownLatch(1);
        final Consts consts = new Consts();
        final MicroService test = new MicroService(InstrumentationRegistry.getContext(), testURI,
                new MicroService.ServiceStartListener() {
                    @Override
                    public void onStart(MicroService service, Synchronizer synchronizer) {
                        service.addEventListener("count", new MicroService.EventListener() {
                            @Override
                            public void onEvent(MicroService service, String event, JSONObject row){
                                try {
                                    consts.count += row.getInt("id");
                                    assertEquals("Ipsum " + (row.getInt("id")-1),row.getString("info"));
                                } catch (JSONException e) {
                                    assertTrue(false);
                                }

                            }
                        });
                        service.addEventListener("ready", new MicroService.EventListener() {
                            @Override
                            public void onEvent(MicroService service, String event, JSONObject o) {
                                try {
                                    JSONObject p = new JSONObject();
                                    p.put("fname", fname);
                                    service.emit("test", p);
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
                    public void onExit(MicroService service, Integer exitCode) {
                        assertEquals(1+2+3+4+5+6+7+8+9+10,consts.count);
                        waitToEnd.countDown();
                    }
                });
        test.start();
        assertTrue(waitToEnd.await(10L, TimeUnit.SECONDS));
    }

    @Test
    public void testMemoryDb() throws Exception {
        testDatabase(":memory:");
    }

    @Test
    public void testAnonymousDb() throws Exception {
        testDatabase("");
    }

    @Test
    public void testLocalDb() throws Exception {
        String testURIString = getClass().getClassLoader().getResource("sqlite3test.js").toString();
        testURIString = testURIString.replace("jar:file:", "jarfile:");
        final URI testURI = URI.create(testURIString);
        Exception throwme = null;
        MicroService.uninstall(InstrumentationRegistry.getContext(), testURI);
        try {
            testDatabase("/home/local/test.sqlite");
        } catch (Exception e) {
            throwme = e;
        } finally {
            MicroService.uninstall(InstrumentationRegistry.getContext(), testURI);
        }

        if (throwme != null)
            throw throwme;
    }

}
