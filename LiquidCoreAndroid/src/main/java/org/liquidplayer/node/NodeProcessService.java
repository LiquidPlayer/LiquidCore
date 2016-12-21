//
// NodeProcessService.java
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
package org.liquidplayer.node;

import android.app.IntentService;
import android.content.Intent;
import android.support.v4.content.LocalBroadcastManager;

import org.liquidplayer.javascript.JSContext;

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
            android.util.Log.d("NodeProcessService", "Creating new Process UUID " + uuid);
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
        for (Map.Entry<String,Process> entry : processMap.entrySet()) {
            if (entry.getValue() == process) {
                processMap.remove(entry.getKey());
                process.removeEventListener(this);
                break;
            }
        }
    }

    @Override
    public void onProcessExit(Process process, int exitCode) {
        for (Map.Entry<String,Process> entry : processMap.entrySet()) {
            if (entry.getValue() == process) {
                processMap.remove(entry.getKey());
                process.removeEventListener(this);
                break;
            }
        }
    }

    @Override
    public void onProcessFailed(Process process, Exception error) {

    }
}
