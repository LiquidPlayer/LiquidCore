/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
@SuppressWarnings("WeakerAccess,SameParameterValue")
class FileSystem extends JSObject {

    private class JSBuilder {
        private StringBuilder js = new StringBuilder();

        private void realDir(final String android) {
            js.append("(function(){try {return fs.realpathSync('");
            js.append(android);
            js.append("');}catch(e){}})()");
        }

        private void alias(final String alias, final String android, final int mask) {
            js.append("fs_.aliases_['");
            js.append(alias);
            js.append("']=");
            realDir(android);
            js.append(";fs_.access_['");
            js.append(alias);
            js.append("']=");
            js.append(mask);
            js.append(";");
        }

        private void mkdir(final String alias, final String androidp, final int mask) {
            if (new File(androidp).mkdirs()) {
                android.util.Log.i("mkdir", "Created directory " + androidp);
            }
            alias(alias, androidp, mask);
        }

        private void symlink(final String alias, final String target, final String linkpath, final int mask) {
            js.append("(function(){fs.symlinkSync('");
            js.append(target);
            js.append("','");
            js.append(linkpath);
            js.append("');})();");
            alias(alias, target, mask);
        }

        private void mkdirAndSymlink(final String alias, final String target, final String linkpath, final int mask) {
            if (new File(target).mkdirs()) {
                android.util.Log.i("mkdir", "Created directory " + target);
            }
            symlink(alias, target, linkpath, mask);
        }

        private void linkMedia(final String type, final String alias, final String linkpath, int mask) {
            File external = Environment.getExternalStoragePublicDirectory(type);
            if (external.mkdirs()) {
                android.util.Log.i("linkMedia", "Created external directory " + external);
            }
            js.append("{var m=");
            realDir(external.getAbsolutePath());
            js.append(";");
            js.append("if(m){");
            js.append("(function(){fs.symlinkSync(m,'");
            js.append(linkpath);
            js.append("/public/media/");
            js.append(alias);
            js.append("');})();");
            js.append("fs_.aliases_['");
            js.append("/home/public/media/");
            js.append(alias);
            js.append("']=m;fs_.access_['");
            js.append("/home/public/media/");
            js.append(alias);
            js.append("']=");
            js.append(mask);
            js.append(";}}");
        }

        private void append(final String s) {
            js.append(s);
        }

        private String build() {
            return js.toString();
        }
    }

    private final Context androidCtx;
    private final String uniqueID;
    private final String sessionID;

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

    private void setUp(JSBuilder js, int mediaPermissionsMask) {
        final String suffix = "/__org.liquidplayer.node__/_" + uniqueID;
        final String sessionSuffix = "/__org.liquidplayer.node__/sessions/" + sessionID;
        final String sessionPath = androidCtx.getCacheDir().getAbsolutePath() + sessionSuffix;
        final String path = androidCtx.getCacheDir().getAbsolutePath() + suffix;
        final String localPath = androidCtx.getFilesDir().getAbsolutePath() + suffix;
        final String node_modules = androidCtx.getFilesDir().getAbsolutePath() +
                "/__org.liquidplayer.node__/node_modules";

        // Set up /home (read-only)
        js.mkdir("/home", sessionPath + "/home", Process.kMediaAccessPermissionsRead);
        // Set up /home/module (read-only)
        if (new File(localPath + "/module").mkdirs()) {
            android.util.Log.i("FileSystem", "Created directory " + path + "/module" );
        }
        js.symlink("/home/module", localPath + "/module",
                sessionPath + "/home/module", Process.kMediaAccessPermissionsRead);
        // Set up /home/temp (read/write)
        js.mkdirAndSymlink("/home/temp", sessionPath + "/temp",
                sessionPath + "/home/temp", Process.kMediaAccessPermissionsRW);
        // Set up /home/cache (read/write)
        js.mkdirAndSymlink("/home/cache", path + "/cache",
                sessionPath + "/home/cache", Process.kMediaAccessPermissionsRW);
        // Set up /home/local (read/write)
        js.mkdirAndSymlink("/home/local", localPath + "/local",
                sessionPath + "/home/local", Process.kMediaAccessPermissionsRW);
        // Permit access to node_modules
        js.symlink("/home/node_modules", node_modules,
                sessionPath + "/home/node_modules", Process.kMediaAccessPermissionsRead);

        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state) &&
                !Environment.MEDIA_MOUNTED_READ_ONLY.equals(state) &&
                !Environment.MEDIA_SHARED.equals(state)){
            android.util.Log.w("FileSystem", "Warning: external storage is unavailable");
        } else {
            if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state) &&
                    (mediaPermissionsMask & Process.kMediaAccessPermissionsWrite) != 0) {
                android.util.Log.w("FileSystem", "Warning: external storage is read only.");
                mediaPermissionsMask &= ~Process.kMediaAccessPermissionsWrite;
            }

            // Set up /home/public
            if (!new File(sessionPath + "/home/public").mkdirs()) {
                android.util.Log.e("FileSystem", "Error: Failed to set up /home/public");
            }

            // Set up /home/public/data
            File external = androidCtx.getExternalFilesDir(null);
            if (external != null) {
                final String externalPersistent = external.getAbsolutePath() + "/LiquidPlayer/" + uniqueID;
                if (new File(externalPersistent).mkdirs()) {
                    android.util.Log.i("FileSystem", "Created external directory " + externalPersistent);
                }
                js.symlink("/home/public/data", externalPersistent,
                        sessionPath + "/home/public/data", Process.kMediaAccessPermissionsRW);
            }

            // Set up /home/public/media
            if (!new File(sessionPath + "/home/public/media").mkdirs()) {
                android.util.Log.e("FileSystem", "Error: Failed to set up /home/public/media");
            }

            if ((mediaPermissionsMask & Process.kMediaAccessPermissionsWrite) != 0 &&
                    ContextCompat.checkSelfPermission(androidCtx,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {

                mediaPermissionsMask &= ~Process.kMediaAccessPermissionsWrite;
            }

            js.linkMedia(Environment.DIRECTORY_MOVIES,   "Movies",   sessionPath+"/home", mediaPermissionsMask);
            js.linkMedia(Environment.DIRECTORY_PICTURES, "Pictures", sessionPath+"/home", mediaPermissionsMask);
            js.linkMedia(Environment.DIRECTORY_DCIM,     "DCIM",     sessionPath+"/home", mediaPermissionsMask);
            js.linkMedia(Environment.DIRECTORY_ALARMS,   "Alarms",   sessionPath+"/home", mediaPermissionsMask);
            if (Build.VERSION.SDK_INT >= 19) {
                js.linkMedia(Environment.DIRECTORY_DOCUMENTS, "Documents", sessionPath+"/home", mediaPermissionsMask);
            }
            js.linkMedia(Environment.DIRECTORY_DOWNLOADS,"Downloads",sessionPath+"/home", mediaPermissionsMask);
            js.linkMedia(Environment.DIRECTORY_MUSIC,    "Music"    ,sessionPath+"/home", mediaPermissionsMask);
            js.linkMedia(Environment.DIRECTORY_NOTIFICATIONS, "Notifications", sessionPath+"/home",
                    mediaPermissionsMask);
            js.linkMedia(Environment.DIRECTORY_PODCASTS, "Podcasts", sessionPath+"/home", mediaPermissionsMask);
            js.linkMedia(Environment.DIRECTORY_RINGTONES,"Ringtones",sessionPath+"/home", mediaPermissionsMask);
        }
        js.append("fs_.cwd='/home';");
    }

    private static final Object sessionMutex = new Object();
    private static final ArrayList<String> activeSessions = new ArrayList<>();

    private static final Object lock = new Object();

    private static void uninstallSession(Context ctx, String sessionID) {
        synchronized (lock) {
            String sessionSuffix = "/__org.liquidplayer.node__/sessions/" + sessionID;

            File session = new File(ctx.getCacheDir().getAbsolutePath() + sessionSuffix);
            android.util.Log.i("sessionWatchdog", "deleting session " + session);

            deleteRecursive(session);
        }
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

        JSBuilder js = new JSBuilder();
        js.append("fs_.aliases_={};fs_.access_={};fs_.require=global.require;");

        setUp(js, mediaPermissionsMask);
        js.append("fs_.fs=function(file){");
        js.append(
                "if (!file.startsWith('/')) { file = ''+this.cwd+'/'+file; }" +
                "try { file = fs_.require('path').resolve(file); } catch (e) {console.log(e);}"+
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
                "return [access,newfile];");
        js.append("};");
        js.append("fs_.alias=function(file){");
        js.append(
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
                "return file;");
        js.append("};");

        new JSFunction(ctx, "setup",
                new String[] {"fs_"},
                js.build(),
                null,
                0
        ).call(null, this);
    }

    private static long lastSessionBark = 0L;
    // Don't clean constantly; wait 5 minutes at least between cleanings
    private static final long SESSION_WATCHDOG_TIMER = 5 * 60 * 1000;
    private static void sessionWatchdog(final Context ctx) {
        final String sessionsSuffix = "/__org.liquidplayer.node__/sessions";

        if (new Date().getTime() - lastSessionBark > SESSION_WATCHDOG_TIMER) {
            new Thread() {
                @Override public void run() {
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
            }.start();
        }
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
