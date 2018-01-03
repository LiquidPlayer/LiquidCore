package org.liquidplayer.javascript;

@SuppressWarnings("JniMissingFunction")
public class JNIJSValue extends JNIObject {
    JNIJSValue(long ref) {
        super(ref);
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        Finalize(reference);
    }

    @Override
    public int hashCode() {
        return (int) reference;
    }

    static native JNIJSValue makeUndefined(JNIJSContext ctx);
    static native JNIJSValue makeNull(JNIJSContext ctx);
    static native JNIJSValue makeBoolean(JNIJSContext ctx, boolean bool);
    static native JNIJSValue makeNumber(JNIJSContext ctx, double number);
    static native JNIJSValue makeString(JNIJSContext ctx, String string);
    static native JNIJSValue makeFromJSONString(JNIJSContext ctx, String string);

    native boolean isUndefined();
    native boolean isNull();
    native boolean isBoolean();
    native boolean isNumber();
    native boolean isString();
    native boolean isObject();
    native boolean isArray();
    native boolean isDate();
    native JNIReturnObject isEqual(JNIJSValue b);
    native boolean isStrictEqual(JNIJSValue b);

    native JNIReturnObject createJSONString(int indent);
    native boolean toBoolean();
    native JNIReturnObject toNumber();
    native JNIReturnObject toStringCopy();
    native JNIReturnObject toObject();

    native void Finalize(long reference);
}
