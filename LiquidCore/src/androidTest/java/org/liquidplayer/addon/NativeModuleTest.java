package org.liquidplayer.addon;

import android.content.Context;

import androidx.annotation.Keep;

import org.liquidplayer.javascript.JSFunction;
import org.liquidplayer.javascript.JSObject;
import org.liquidplayer.javascript.JSValue;
import org.liquidplayer.node.BuildConfig;
import org.liquidplayer.service.AddOn;
import org.liquidplayer.service.MicroService;

@Keep
@SuppressWarnings("unused")
public class NativeModuleTest implements AddOn {
    public NativeModuleTest(Context context) {}

    @Override
    public void register(String module) {
        if (BuildConfig.DEBUG && !module.equals("native-module-test")) { throw new AssertionError(); }

        register();
    }

    @Override
    public void require(JSValue binding, MicroService service) {
        if (BuildConfig.DEBUG && (binding == null || !binding.isObject())) {
            throw new AssertionError();
        }

        JSObject bindingObject = binding.toObject();
        bindingObject.property("highLevelFunction",
            new JSFunction(binding.getContext(), "highLevelFunction") {
                public Integer highLevelFunction() {
                    return 42;
                }
            });
    }


    native static void register();
}
