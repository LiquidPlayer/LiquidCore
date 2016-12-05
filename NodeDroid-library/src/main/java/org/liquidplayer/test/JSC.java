package org.liquidplayer.test;

import org.liquidplayer.javascript.JSContext;

import java.util.Scanner;

@SuppressWarnings("JniMissingFunction")
public class JSC {

    static {
        JSContext.dummy();
    }

    public int test() throws Exception {
        String script = new Scanner(getClass().getClassLoader()
                .getResourceAsStream("testapi.js"), "UTF-8").useDelimiter("\\A").next();
        return main(script);
    }

    private native int main(String script);
}
