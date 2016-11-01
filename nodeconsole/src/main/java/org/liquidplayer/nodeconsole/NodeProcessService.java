package org.liquidplayer.nodeconsole;

import android.app.IntentService;
import android.content.Intent;
import android.support.v4.content.LocalBroadcastManager;

import org.liquidplayer.node.Process;
import org.liquidplayer.v8.JSContext;

import java.util.HashMap;
import java.util.Map;

public class NodeProcessService extends IntentService implements Process.EventListener {
    public NodeProcessService() {
        super("org.liquidplayer.node.NodeProcessService");
    }

    static private Map<String,Process> processMap = new HashMap<>();

    @Override
    protected void onHandleIntent(Intent workIntent) {
        String uuid = workIntent.getExtras().getString("org.liquidplayer.node.Process");

        if (processMap.get(uuid) == null) {
            processMap.put(uuid,
                    new Process(this, "node_console", Process.kMediaAccessPermissionsRW, this));
        }

        Intent intent = new Intent(uuid);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    public static Process getProcess(String id) {
        return processMap.get(id);
    }

    @Override
    public void onProcessStart(Process process, JSContext context) {
    }

    @Override
    public void onProcessAboutToExit(Process process, int exitCode) {
    }

    @Override
    public void onProcessExit(Process process, int exitCode) {
        for (Map.Entry<String,Process> entry : processMap.entrySet()) {
            if (entry.getValue() == process) {
                processMap.remove(entry.getKey());
                break;
            }
        }
    }

    @Override
    public void onProcessFailed(Process process, Exception error) {

    }
}
