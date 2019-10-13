/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.JSCTest;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;

import java.lang.reflect.Method;
import java.util.Scanner;

public class JSC {

    /* Ensure the shared libraries get loaded first */
    static {
        try {
            Method init = JSContext.class.getDeclaredMethod("init");
            init.setAccessible(true);
            init.invoke(null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private JSContextGroup group;

    JSC(JSContextGroup group) {
        this.group = group;
    }

    int testAPI() {
        ClassLoader loader = getClass().getClassLoader();
        if (loader != null) {
            String script = new Scanner(loader
                    .getResourceAsStream("testapi.js"), "UTF-8").useDelimiter("\\A").next();
            return main(script, group == null ? 0 : group.groupHash());
        }
        return 1;
    }

    int testMinidom() {
        ClassLoader loader = getClass().getClassLoader();
        if (loader != null) {
            String script = new Scanner(loader
                    .getResourceAsStream("minidom.js"), "UTF-8").useDelimiter("\\A").next();
            return minidom(script, group == null ? 0 : group.groupHash());
        }
        return 1;
    }

    private native int main(String script, long contextGroup);
    private native int minidom(String script, long contextGroup);
}
