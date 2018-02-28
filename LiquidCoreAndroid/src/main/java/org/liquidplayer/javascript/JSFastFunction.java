package org.liquidplayer.javascript;

public class JSFastFunction extends JSFunction {
    JSFastFunction(JSContext ctx, final String name) {
        context = ctx;
        context.sync(new Runnable() {
            @Override
            public void run() {
                valueRef = context.ctxRef().makeFunctionWithCallback(JSFastFunction.this, name);
            }
        });

        context.persistObject(this);
        context.zombies.add(this);
    }

    protected JSValue function(JSObject thiz, JSValue[] args, final JSObject invokeObject) {
        return callback(thiz, args);
    }

    public JSValue callback(JSObject thiz, JSValue[] args) {
        return new JSValue(context);
    }
}