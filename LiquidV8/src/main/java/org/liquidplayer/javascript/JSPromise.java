package org.liquidplayer.javascript;

/**
 * Convenience class for managing Javascript Promises
 */
public class JSPromise extends JSObject {
    /**
     * Creates a new JavaScript Promise object.
     * @param ctx The JavaScript context
     */
    public JSPromise(JSContext ctx) {
        context = ctx;
        promiseObject = context.evaluateScript(
            "(()=>{" +
            "  var po = {}; var clock = true;" +
            "  var timer = setInterval(()=>{if(!clock) clearTimeout(timer);}, 100); "+
            "  po.promise = new Promise((resolve,reject)=>{po.resolve=resolve;po.reject=reject});" +
            "  po.promise.then(()=>{clock=false}).catch(()=>{clock=false});" +
            "  return po;" +
            "})();").toObject();
        JSObject promise = promiseObject.property("promise").toObject();
        valueRef = promise.valueRef();

        addJSExports();
        context.persistObject(this);
    }

    /**
     * Resolves the Promise
     * @param args optional arguments to pass to promise resolution function
     */
    public void resolve(Object ...args) {
        if (args == null) {
            args = new Object[0];
        }
        promiseObject.property("resolve").toFunction().apply(null, args);
    }

    /**
     * Rejects the Promise
     * @param args optional arguments to pass to the promise rejection function
     */
    public void reject(Object ...args) {
        if (args == null) {
            args = new Object[0];
        }
        promiseObject.property("reject").toFunction().apply(null, args);
    }

    final private JSObject promiseObject;
}
