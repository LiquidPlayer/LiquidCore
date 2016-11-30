package org.liquidplayer.test;

import org.liquidplayer.javascript.JSContext;

@SuppressWarnings("JniMissingFunction")
public class JSC {

    static {
        JSContext.dummy();
    }

    public int test() throws Exception {
        String script = getClass().getClassLoader().getResource("testapi.js").getContent().toString();
        return main(script);
    }

    private native int main(String script);
}
