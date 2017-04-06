//
// FileSystem.java
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

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;
import android.support.v4.content.ContextCompat;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import java.util.UUID;

/**
 * This creates a JavaScript object that is used by nodedroid_file.cc in the native code
 * to control access to the file system.  We will create an alias filesystem with the
 * following structure.
 *
 * home
 *  |
 *  +--- node_modules
 *  |
 *  +--- module
 *  |
 *  +--- temp
 *  |
 *  +--- cache
 *  |
 *  +--- local
 *  |
 *  +--- public
 *         |
 *         +--- data
 *         |
 *         +--- media
 *                |
 *                +--- Pictures
 *                |
 *                +--- Movies
 *                |
 *                +--- Ringtones
 *                |
 *                +--- ...
 *
 * /home
 *
 * Read-only directory which acts only as a holding bin for other directories
 *
 * /home/node_modules
 *
 * Read-only directory containing included node modules.  These modules can be loaded using
 * 'require()'.  These modules are in addition to the standard built-in modules.  Currently, only
 * sqlite3 is included.
 *
 * /home/module
 *
 * Read-only directory.  Contains downloaded javascript code for the MicroService.  As
 * these files change on the server, they will be updated and cached here.
 *
 * /home/temp
 *
 * Read/write directory.  This directory is private to a single instance of the MicroService.
 * The contents of this directory live only as long as the underlying Node.js process is active.
 * This directory is appropriate only for very short-lived temporary files.
 *
 * /home/cache
 *
 * Read/write directory.  This directory is available to all instances of the same MicroService
 * running on the same local host.  The host, in this case, is the host app which uses an instance
 * of the LiquidCore library.  Files in this directory will be deleted on an as-needed basis
 * as they age and/or a memory space quota is breaching.
 *
 * /home/local
 *
 * Read/write directory.  This directory is the persistent storage for a MicroService and is shared
 * amongst all instances of the same MicroService running on the same local host.  All content in
 * this directory will persist so long as the MicroService is present on the host.  This directory
 * will only be cleared when the MicroService is "uninstalled".  Uninstallation happens for
 * MicroServices that have not been used in a long time and when space is required for installing
 * new MicroServices.
 *
 * /home/public
 *
 * Read-only directory which acts as a holding bin for other public directories.  This directory
 * may not exist if no external (sdcard) storage is available.
 *
 * /home/public/data
 *
 * Read/write directory for MicroService-specific data.  This directory is shared between all
 * instances of a MicroService on all hosts (local or not).  Its contents are publicly available for
 * any app to access, though its true location on external media is a bit obscured.  This directory
 * persists so long as a MicroService exists on any host.  If a MicroService is uninstalled from
 * every host, this directory will also be cleared.
 *
 * /home/public/media
 *
 * Read-only holding directory for public media-specific directories.
 *
 * /home/public/media/[MediaType]
 *
 * Read or read-write directory (depending on permissions given by the host) for known media types.
 * These types include Pictures, Movies, Ringtones, Music, Downloads, etc. as exposed by Android and
 * typically reside at a true location like /sdcard/Pictures, for example.  Files in these
 * directories are never cleared by LiquidCore, but can be managed by any other app or service.
 *
 * Everything else will result in a ENOACES (access denied) error
 */
class FileSystem extends JSObject {

    @jsexport(attributes = JSPropertyAttributeReadOnly) @SuppressWarnings("unused")
    private Property<JSObject> access_;

    @jsexport(attributes = JSPropertyAttributeReadOnly) @SuppressWarnings("unused")
    private Property<JSObject> aliases_;

    @jsexport(attributes = JSPropertyAttributeReadOnly) @SuppressWarnings("unused")
    private Property<JSFunction> fs;

    @jsexport(attributes = JSPropertyAttributeReadOnly) @SuppressWarnings("unused")
    private Property<JSFunction> alias;

    @jsexport  @SuppressWarnings("unused")
    private Property<String> cwd;

    private final Context androidCtx;
    private final String uniqueID;
    private final String sessionID;

    private String realDir(String dir) {
        JSValue v = getContext().evaluateScript(
                "(function(){try {return require('fs').realpathSync('" + dir + "');}catch(e){}})()"
        );
        if (v.isUndefined() || v.isNull()) return null;
        return v.toString();
    }
    private String mkdir(String dir) {
        if (new File(dir).mkdirs()) {
            android.util.Log.i("mkdir", "Created directory " + dir);
        }
        return realDir(dir);
    }
    private void symlink(String target, String linkpath) {
        getContext().evaluateScript(
                "(function(){require('fs').symlinkSync('" + target + "','" + linkpath +"');})()"
        );
    }
    private static boolean isSymlink(File file) throws IOException {
        if (file == null)
            throw new NullPointerException("File must not be null");
        File canon;
        if (file.getParent() == null) {
            canon = file;
        } else {
            File canonDir = file.getParentFile().getCanonicalFile();
            canon = new File(canonDir, file.getName());
        }
        return !canon.getCanonicalFile().equals(canon.getAbsoluteFile());
    }
    static void deleteRecursive(File fileOrDirectory) {
        try {
            if (fileOrDirectory.isDirectory() && !isSymlink(fileOrDirectory) &&
                    fileOrDirectory.listFiles() != null) {
                for (File child : fileOrDirectory.listFiles()) {
                    deleteRecursive(child);
                }
            }
        } catch (IOException e) {
            android.util.Log.e("deleteRecursive", e.getMessage());
        }

        if (!fileOrDirectory.delete()) {
            android.util.Log.e("deleteRecursive",
                    "Failed to delete " + fileOrDirectory.getAbsolutePath());
        }
    }
    private void linkMedia(String type, String dir, String home, int mediaPermissionsMask) {
        File external = Environment.getExternalStoragePublicDirectory(type);
        if (external.mkdirs()) {
            android.util.Log.i("linkMedia", "Created external directory " + external);
        }
        String media = realDir(external.getAbsolutePath());
        if (media != null) {
            symlink(media, home + "/public/media/" + dir);
            aliases_.get().property("/home/public/media/" + dir, media);
            access_.get().property("/home/public/media/" + dir, mediaPermissionsMask);
        }
    }

    private void setUp(int mediaPermissionsMask) {
        final String suffix = "/__org.liquidplayer.node__/_" + uniqueID;
        String sessionSuffix = "/__org.liquidplayer.node__/sessions/" + sessionID;

        // Set up /home (read-only)
        String home  = mkdir(androidCtx.getCacheDir().getAbsolutePath() +
                sessionSuffix + "/home");
        aliases_.get().property("/home", home);
        access_ .get().property("/home", Process.kMediaAccessPermissionsRead);

        // Set up /home/module (read-only)
        String module = mkdir(androidCtx.getFilesDir().getAbsolutePath() +
                suffix + "/module");
        symlink(module, home + "/module");
        aliases_.get().property("/home/module", module);
        access_ .get().property("/home/module", Process.kMediaAccessPermissionsRead);

        // Set up /home/temp (read/write)
        String temp = mkdir(androidCtx.getCacheDir().getAbsolutePath() +
                sessionSuffix + "/temp");
        symlink(temp, home + "/temp");
        aliases_.get().property("/home/temp", temp);
        access_ .get().property("/home/temp", Process.kMediaAccessPermissionsRW);

        // Set up /home/cache (read/write)
        String cache = mkdir(androidCtx.getCacheDir().getAbsolutePath() +
                suffix + "/cache");
        symlink(cache, home + "/cache");
        aliases_.get().property("/home/cache", cache);
        access_ .get().property("/home/cache", Process.kMediaAccessPermissionsRW);

        // Set up /home/local (read/write)
        String local = mkdir(androidCtx.getFilesDir().getAbsolutePath() +
                suffix + "/local");
        symlink(local, home + "/local");
        aliases_.get().property("/home/local", local);
        access_ .get().property("/home/local", Process.kMediaAccessPermissionsRW);

        // Permit access to node_modules
        String node_modules = androidCtx.getFilesDir().getAbsolutePath() +
                "/__org.liquidplayer.node__/node_modules";
        symlink(node_modules, home + "/node_modules");
        aliases_.get().property("/home/node_modules", node_modules);
        access_ .get().property("/home/node_modules", Process.kMediaAccessPermissionsRead);

        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)){
            android.util.Log.w("FileSystem", "Warning: external storage is unavailable");
        } else {
            if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state) &&
                    (mediaPermissionsMask & Process.kMediaAccessPermissionsWrite) != 0) {
                android.util.Log.w("FileSystem", "Warning: external storage is read only.");
                mediaPermissionsMask &= ~Process.kMediaAccessPermissionsWrite;
            }

            // Set up /home/public
            if (!new File(home + "/public").mkdirs()) {
                android.util.Log.e("FileSystem", "Error: Failed to set up /home/public");
            }

            // Set up /home/public/data
            File external = androidCtx.getExternalFilesDir(null);
            if (external != null) {
                String externalPersistent = mkdir(external.getAbsolutePath() +
                        "/LiquidPlayer/" + uniqueID);
                symlink(externalPersistent, home + "/public/data");
                aliases_.get().property("/home/public/data", externalPersistent);
                access_.get().property("/home/public/data", Process.kMediaAccessPermissionsRW);
            }

            // Set up /home/public/media
            if (!new File(home + "/public/media").mkdirs()) {
                android.util.Log.e("FileSystem", "Error: Failed to set up /home/public/media");
            }

            if ((mediaPermissionsMask & Process.kMediaAccessPermissionsWrite) != 0 &&
                    ContextCompat.checkSelfPermission(androidCtx,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {

                mediaPermissionsMask &= ~Process.kMediaAccessPermissionsWrite;
            }

            linkMedia(Environment.DIRECTORY_MOVIES,   "Movies",   home, mediaPermissionsMask);
            linkMedia(Environment.DIRECTORY_PICTURES, "Pictures", home, mediaPermissionsMask);
            linkMedia(Environment.DIRECTORY_DCIM,     "DCIM",     home, mediaPermissionsMask);
            linkMedia(Environment.DIRECTORY_ALARMS,   "Alarms",   home, mediaPermissionsMask);
            if (Build.VERSION.SDK_INT >= 19) {
                linkMedia(Environment.DIRECTORY_DOCUMENTS, "Documents", home, mediaPermissionsMask);
            }
            linkMedia(Environment.DIRECTORY_DOWNLOADS,"Downloads",home, mediaPermissionsMask);
            linkMedia(Environment.DIRECTORY_MUSIC,    "Music"    ,home, mediaPermissionsMask);
            linkMedia(Environment.DIRECTORY_NOTIFICATIONS, "Notifications", home,
                    mediaPermissionsMask);
            linkMedia(Environment.DIRECTORY_PODCASTS, "Podcasts", home, mediaPermissionsMask);
            linkMedia(Environment.DIRECTORY_RINGTONES,"Ringtones",home, mediaPermissionsMask);
        }

        cwd.set("/home");
    }

    private static final Object sessionMutex = new Object();
    private static final ArrayList<String> activeSessions = new ArrayList<>();

    private static void uninstallSession(Context ctx, String sessionID) {
        String sessionSuffix = "/__org.liquidplayer.node__/sessions/" + sessionID;

        File session = new File(ctx.getCacheDir().getAbsolutePath() + sessionSuffix);
        android.util.Log.i("sessionWatchdog", "deleting session " + session);

        deleteRecursive(session);
    }

    static void uninstallLocal(Context ctx, String serviceID) {
        final String suffix = "/__org.liquidplayer.node__/_" + serviceID;

        deleteRecursive(new File(ctx.getCacheDir().getAbsolutePath() + suffix));
        deleteRecursive(new File(ctx.getFilesDir().getAbsolutePath() + suffix));
    }

    static void uninstallGlobal(Context ctx, String serviceID) {
        File external = ctx.getExternalFilesDir(null);
        if (external != null) {
            String externalPersistent = external.getAbsolutePath() + "/LiquidPlayer/" + serviceID;
            deleteRecursive(new File(externalPersistent));
        }
    }

    FileSystem(JSContext ctx, Context androidCtx,
               String uniqueID, int mediaPermissionsMask) {
        super(ctx);
        this.androidCtx = androidCtx;

        // Clean any dead sessions
        sessionWatchdog(androidCtx);

        this.uniqueID = uniqueID;
        sessionID = UUID.randomUUID().toString();
        synchronized (sessionMutex) {
            activeSessions.add(sessionID);
        }

        aliases_.set(new JSObject(ctx));
        access_ .set(new JSObject(ctx));

        setUp(mediaPermissionsMask);

        fs.set(new JSFunction(ctx, "fs", ""+
                "if (!file.startsWith('/')) { file = this.cwd+'/'+file; }" +
                "try { file = require('path').resolve(file); } catch (e) {console.log(e);}"+
                "var access = 0;"+
                "var keys = Object.keys(this.aliases_).sort().reverse();"+
                "for (var p=0; p<keys.length; p++) {"+
                "    if (file.startsWith(this.aliases_[keys[p]] + '/')) {"+
                "        file = keys[p] + '/' + file.substring(this.aliases_[keys[p]].length + 1);"+
                "        break;"+
                "    } else if (file == this.aliases_[keys[p]]) {"+
                "        file = keys[p];"+
                "        break;"+
                "    }"+
                "}"+
                "var acckeys = Object.keys(this.access_).sort().reverse();"+
                "for (var p=0; p<acckeys.length; p++) {"+
                "    if (file.startsWith(acckeys[p] + '/') || acckeys[p]==file) {"+
                "        access = this.access_[acckeys[p]];"+
                "        break;"+
                "    }"+
                "}"+
                "var newfile = file;"+
                "for (var p=0; p<keys.length; p++) {"+
                "    if (file.startsWith(keys[p] + '/')) {"+
                "        newfile = this.aliases_[keys[p]] +'/'+file.substring(keys[p].length + 1);"+
                "        break;"+
                "    } else if (file == keys[p]) {"+
                "        newfile = this.aliases_[keys[p]];"+
                "        break;"+
                "    }"+
                "}"+
                "return [access,newfile];",
                "file"));

        alias.set(new JSFunction(ctx, "alias", ""+
                "var keys = Object.keys(this.aliases_).sort().reverse();"+
                "for (var p=0; p<keys.length; p++) {"+
                "   if (file.startsWith(this.aliases_[keys[p]] + '/')) {"+
                "       file = keys[p] + '/' + file.substring(this.aliases_[keys[p]].length + 1);"+
                "       break;"+
                "   } else if (file == this.aliases_[keys[p]]) {"+
                "       file = keys[p];"+
                "       break;"+
                "   }"+
                "}"+
                "return file;",
                "file"));

    }

    private static long lastSessionBark = 0L;
    // Don't clean constantly; wait 5 minutes at least between cleanings
    private static final long SESSION_WATCHDOG_TIMER = 5 * 60 * 1000;
    private static void sessionWatchdog(Context ctx) {
        final String sessionsSuffix = "/__org.liquidplayer.node__/sessions";

        if (new Date().getTime() - lastSessionBark > SESSION_WATCHDOG_TIMER) {
            File sessions = new File(ctx.getCacheDir().getAbsolutePath() + sessionsSuffix);
            File[] files = sessions.listFiles();
            if (files != null) {
                for (File session : files) {
                    boolean isActive;
                    synchronized (sessionMutex) {
                        isActive = activeSessions.contains(session.getName());
                    }
                    if (!isActive) {
                        uninstallSession(ctx, session.getName());
                    }
                }
            }
            lastSessionBark = new Date().getTime();
        }
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        cleanUp();
    }

    void cleanUp() {
        boolean needClean;
        synchronized (sessionMutex) {
            needClean = activeSessions.contains(sessionID);
            activeSessions.remove(sessionID);
        }
        if (needClean) {
            uninstallSession(androidCtx, sessionID);
        }
    }
}
