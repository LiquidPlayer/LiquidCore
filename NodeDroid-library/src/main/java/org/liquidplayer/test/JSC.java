package org.liquidplayer.test;

import org.liquidplayer.javascript.JSContext;
import org.liquidplayer.javascript.JSContextGroup;

import java.util.Scanner;

@SuppressWarnings("JniMissingFunction")
public class JSC {

    static {
        JSContext.dummy();
    }

    JSContextGroup group = null;

    JSC(JSContextGroup group) {
        this.group = group;
    }

    int testAPI() throws Exception {
        String script = new Scanner(getClass().getClassLoader()
                .getResourceAsStream("testapi.js"), "UTF-8").useDelimiter("\\A").next();
        return main(script, group==null?0L:group.groupRef());
    }

    int testMinidom() throws Exception {
        String script = new Scanner(getClass().getClassLoader()
                .getResourceAsStream("minidom.js"), "UTF-8").useDelimiter("\\A").next();
        return minidom(script, group==null?0L:group.groupRef());
    }

    private native int main(String script, long contextGroup);
    private native int minidom(String script, long contextGroup);
}
