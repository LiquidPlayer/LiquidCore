package org.liquidplayer.node;

import android.content.Context;
import android.os.Environment;
import android.text.Html;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;

import java.io.File;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/**
 * This creates a JavaScript object that is used by nodedroid_file.cc in the native code
 * to control access to the file system.  We will create an alias filesystem with the
 * following structure
 *
 * /
 * /home
 * /home/persistent
 * /home/cache
 * /home/external
 * /home/external/persistent
 * /home/external/cache
 * /home/media
 * /home/media/Pictures
 * /home/media/Movies
 * /home/media/Ringtones
 * /home/media/Downloads
 * /home/media/...
 *
 * Everything else will result in a ENOACES (access denied) error
 */
class FileSystem extends JSObject {

    @jsexport(attributes = JSPropertyAttributeReadOnly)
    private Property<JSObject> access_;

    @jsexport(attributes = JSPropertyAttributeReadOnly)
    private Property<JSObject> aliases_;

    @jsexport(attributes = JSPropertyAttributeReadOnly)
    private Property<JSFunction> fs;

    @jsexport(attributes = JSPropertyAttributeReadOnly)
    private Property<JSFunction> alias;

    @jsexport
    private Property<String> cwd;

    private final Context androidCtx;
    private String uniqueID;

    private String realDir(String dir) {
        return getContext().evaluateScript(
                "(function(){return require('fs').realpathSync('" + dir + "');})()"
        ).toString();
    }
    private String mkdir(String dir) {
        new File(dir).mkdirs();
        return realDir(dir);
    }
    private List<String> toclean = new ArrayList<>();
    private void symlink(String target, String linkpath) {
        getContext().evaluateScript(
                "(function(){require('fs').symlinkSync('" + target + "','" + linkpath +"');})()"
        );
    }
    private boolean isSymlink(File file) {
        try {
            return file.getCanonicalFile().equals(file.getAbsoluteFile());
        } catch (IOException e) {
            return true;
        }
    }
    private void deleteRecursive(File fileOrDirectory) {
        if (fileOrDirectory.isDirectory() && !isSymlink(fileOrDirectory))
            for (File child : fileOrDirectory.listFiles())
                deleteRecursive(child);

        if (!fileOrDirectory.delete()) {
            android.util.Log.d("nodedroid", "Failed to delete directory");
        }
    }

    private void setUp() {
        final String suffix = "/__org.liquidplayer.node__/_" + uniqueID;
        String random = "" + UUID.randomUUID();
        String sessionSuffix = suffix + "/" + random;

        String home  = mkdir(androidCtx.getFilesDir().getAbsolutePath() +
                sessionSuffix + "/home");
        toclean.add(home);
        aliases_.get().property("/home", home);
        access_ .get().property("/home", Process.kMediaAccessPermissionsRW);

        String cache = mkdir(androidCtx.getCacheDir().getAbsolutePath() +
                sessionSuffix + "/cache");
        toclean.add(cache);
        symlink(cache, home + "/cache");
        aliases_.get().property("/home/cache", cache);

        String persistent = mkdir(androidCtx.getFilesDir().getAbsolutePath() +
                suffix + "/persistent");
        symlink(persistent, home + "/persistent");
        aliases_.get().property("/home/persistent", persistent);

        new File(home + "/external").mkdirs();
        if (androidCtx.getExternalCacheDir()!=null) {
            String externalCache = mkdir(androidCtx.getExternalCacheDir().getAbsolutePath() +
                    sessionSuffix + "/cache");
            symlink(externalCache, home + "/external/cache");
            toclean.add(externalCache);
            aliases_.get().property("/home/external/cache", externalCache);
        } else {
            android.util.Log.d("setUp", "No external cache dir");
        }

        File external = androidCtx.getExternalFilesDir(null);
        if (external!=null) {
            String externalPersistent = mkdir(external.getAbsolutePath() +
                    "/LiquidPlayer/" + uniqueID);
            symlink(externalPersistent, home + "/external/persistent");
            aliases_.get().property("/home/external/persistent", externalPersistent);
        }

        external = androidCtx.getExternalFilesDir(Environment.DIRECTORY_PICTURES);
        androidCtx.getExternalFilesDir(Environment.DIRECTORY_DCIM);
        androidCtx.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        androidCtx.getExternalFilesDir(Environment.DIRECTORY_MOVIES);
        androidCtx.getExternalFilesDir(Environment.DIRECTORY_MUSIC);
        androidCtx.getExternalFilesDir(Environment.DIRECTORY_NOTIFICATIONS);
        androidCtx.getExternalFilesDir(Environment.DIRECTORY_PODCASTS);
        androidCtx.getExternalFilesDir(Environment.DIRECTORY_RINGTONES);
        if (external!=null) {
            String media = realDir(external.getAbsolutePath() + "/..");
            symlink(media, home + "/media");
            aliases_.get().property("/home/media", media);
        }

        cwd.set("/home");
    }

    private void tearDown() {
        for (String dir : toclean) {
            deleteRecursive(new File(dir));
        }
    }

    FileSystem(JSContext ctx, Context androidCtx,
               String uniqueID, int mediaPermissionsMask) {
        super(ctx);
        this.androidCtx = androidCtx;
        try {
            this.uniqueID = URLEncoder.encode(uniqueID, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            android.util.Log.e("FileSystem", e.toString());
        }

        aliases_.set(new JSObject(ctx));
        access_ .set(new JSObject(ctx));

        setUp();


        /*
        String state = Environment.getExternalStorageState();
        int access = Environment.MEDIA_MOUNTED_READ_ONLY.equals(state) ?
                kMediaAccessPermissionsRead :
                Environment.MEDIA_MOUNTED.equals(state) ? kMediaAccessPermissionsRW : 0;
        access &= mediaPermissionsMask;

        if (access > 0) {
            for (int i = 0; i < media.length; i++) {
                String localDir = ctx.evaluateScript(
                    "(function(){return require('fs').realpathSync('" + publics[i] + "');})()"
                ).toString();

                aliases_.get().property(media[i], localDir);
                access_.get().property(media[i], access);
            }
        }
        */

        fs.set(new JSFunction(ctx, "fs", ""+
                "if (!file.startsWith('/')) { file = this.cwd+'/'+file; }" +
                "try { file = require('path').resolve(file); } catch (e) {}"+
                "var access = 0;"+
                "for (var p in this.aliases_) {"+
                "    if (file.startsWith(this.aliases_[p] + '/')) {"+
                "        file = p + '/' + file.substring(this.aliases_[p].length + 1);"+
                "        break;"+
                "    } else if (file == this.aliases_[p]) {"+
                "        file = p;"+
                "        break;"+
                "    }"+
                "}"+
                "for (var p in this.access_) {"+
                "    if (file.startsWith(p + '/') || p==file) {"+
                "        access = this.access_[p];"+
                "        break;"+
                "    }"+
                "}"+
                "var newfile = file;"+
                "for (var p in this.aliases_) {"+
                "    if (file.startsWith(p + '/')) {"+
                "        newfile = this.aliases_[p] + '/' + file.substring(p.length + 1);"+
                "        break;"+
                "    } else if (file == p) {"+
                "        newfile = this.aliases_[p];"+
                "        break;"+
                "    }"+
                "}"+
                "return [access,newfile];",
                "file"));

        alias.set(new JSFunction(ctx, "alias", ""+
                "for (var p in this.aliases_) {"+
                "   if (file.startsWith(this.aliases_[p] + '/')) {"+
                "       file = p + '/' + file.substring(this.aliases_[p].length + 1);"+
                "       break;"+
                "   } else if (file == this.aliases_[p]) {"+
                "       file = p;"+
                "       break;"+
                "   }"+
                "}"+
                "return file;",
                "file"));

    }

    void cleanUp() {
        tearDown();
    }
}
