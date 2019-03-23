# The LiquidCore Project

LiquidCore enables [Node.js] virtual machines to run inside Android and iOS apps.  It provides a complete runtime environment, including a virtual file system.

LiquidCore also provides a convenient way for Android developers to [execute raw JavaScript](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-as-a-Native-Javascript-Engine) inside of their apps, as iOS developers can already do natively with JavaScriptCore.

Version
-------
[![Release](https://jitpack.io/v/LiquidPlayer/LiquidCore.svg)](https://jitpack.io/#LiquidPlayer/LiquidCore)
![Downloads](https://jitpack.io/v/LiquidPlayer/LiquidCore/week.svg)

[0.6.0](https://github.com/LiquidPlayer/LiquidCore/releases/tag/0.6.0) - Get it through [JitPack](https://jitpack.io/#LiquidPlayer/LiquidCore/0.6.0), **or**:

In your Android project root directory:
```
% npm i -g liquidcore-cli
% liquidcore gradle
```
And follow the directions in the output.

Javadocs
--------
[Version 0.6.0](https://liquidplayer.github.io/LiquidCoreAndroid/0.6.0/index.html)

# Table of Contents

1. [Architecture](#architecture)
2. [Java / JavaScript API](#java--javascript-api) - Access JavaScript directly from Java
3. [Node `Process`](#node-process) - Run raw Node.js instances on Android
4. [The `MicroService`](#the-microservice) - Run enhanced Node.js instances on Android
5. ["Hallo, die Weld!" Micro Service Tutorial](#hallo-die-weld-micro-service-tutorial)
6. [Native add-ons](#native-add-ons) 
7. [Building the LiquidCore Android library](#building-the-liquidcore-android-library)
8. [License](#license)

# Architecture

![Android Architecture Diagram](https://github.com/LiquidPlayer/LiquidCore/raw/master/doc/ArchitectureAndroid.png)

LiquidCore for Android includes the Node.js runtime and V8 backend.  In addition, it provides three APIs for apps to interact with:

* **[Java / JavaScript JNI](#java--javascript-api) API**, which provides a convenient way to run raw JavaScript code from within Java, without the need for a clunky `WebView` or to write any native code.
* **[Node `Process`](#node-process) API**, which allows developers to launch fast isolated instances of the Node.js runtime.
* **[`MicroService`](#the-microservice) API**, which is an abstraction of a Node.js process and supports dynamic code fetching and native add-ons.

Native add-ons enable extending the basic runtime environment with additional native functionality.  Add-ons have access to all the above APIs, plus the ability to use [WebKit's JavaScriptCore API](https://developer.apple.com/documentation/javascriptcore?language=objc) running on top of V8.  This allows projects that depend on JavaScriptCore, like [React Native](https://facebook.github.io/react-native/), to use LiquidCore directly.

# Java / JavaScript API

[JavaDocs v0.6.0](https://liquidplayer.github.io/LiquidCoreAndroid/0.6.0/index.html?org/liquidplayer/javascript/package-summary.html)

You can use LiquidCore as a raw Native Javascript Engine (i.e. as a replacement for [`AndroidJSCore`](https://github.com/ericwlange/AndroidJSCore)).  That topic is discussed [here](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-as-a-Native-Javascript-Engine).

For example:

```java
JSContext context = new JSContext();
JSFunction javaFactorial = new JSFunction(context,"factorial") {
    public Integer factorial(Integer x) {
        int factorial = 1;
        for (; x > 1; x--) {
            factorial *= x;
        }
        return factorial;
    }
};
context.property("factorial", javaFactorial);
JSValue result = context.evaluateScript("(() => { let x = 10; return factorial(x); })()");
android.util.Log.i("LiquidCoreExample", "The factorial of 10 is " + result.toString());
```

# Node `Process`

[JavaDocs v0.6.0](https://liquidplayer.github.io/LiquidCoreAndroid/0.6.0/index.html?org/liquidplayer/node/Process.html)

LiquidCore allows creation of raw Node.js instances.  Each instance runs in its own thread and is isolated from all other instances.  Instances can share a [virtual file system](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-File-System), by using a common unique identifier.

It is not recommended to use the `Process` API directly for most use cases. The [`MicroService`](#the-microservice) API is more robust and has additional functionality, like support for native modules and downloadable JavaScript bundles.

# The `MicroService`

[JavaDocs v0.6.0](https://liquidplayer.github.io/LiquidCoreAndroid/0.6.0/index.html?org/liquidplayer/service/MicroService.html)

A micro service is nothing more than an independent Node.js instance whose startup code is referenced by a URI.  For example:

```java
MicroService service = new MicroService(androidContext,
    new URI("http://my.server.com/path/to/code.js"));
service.start();
```

The service URI can either refer to a server URL or a local Android resource. LiquidCore is designed to primarily use remote URLs, as dynamic updates are an important value proposition, but local resources are also supported.  For example, `android.resource://com.example.myapp/raw/some_js_file`, where `some_js_file.js` resides in `res/raw/some_js_file.js` (note that the `.js` is omitted from the URI when using an Android resource).

A micro service can communicate with the host app once the Node.js environment is set up.  This can be determined by adding a `ServiceStartListener` in the `MicroService` constructor:

```java
MicroService service = new MicroService(
    androidContext,
    new URI("http://my.server.com/path/to/code.js"),
    new MicroService.StartServiceListener() {
        @Override
        public void onStart(MicroService service) {
            // .. The environment is live, but the startup JS code (from the URI)
            // has not been executed yet.
        }
    }
);
service.start();
```

A micro service communicates with the host through a simple [`EventEmitter`](https://nodejs.org/api/events.html#events_class_eventemitter) interface, eponymously called `LiquidCore`.  For example, in your JavaScript startup code (code.js in this example):

```javascript
LiquidCore.emit('my_event', {foo: "hello, world", bar: 5, l337 : ['a', 'b'] })
```

On the Java side, the host app can listen for events:

```java
// ... in the StartServiceListener.onStart() method:

service.on("my_event", new MicroService.EventListener() {
    @Override
    public void onEvent(MicroService service, String event, JSONObject payload) {
        try {
            android.util.Log.i("Event:" + event, payload.getString("foo"));
            // logs: I/Event:my_event: hello, world
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }
});
```

Similarly, the micro service can listen for events from the host:

```java
JSONObject payload = new JSONObject();
payload.put("hallo", "die Weld");
service.emit("host_event", payload);
```

Then, in Javascript:

```javascript
LiquidCore.on('host_event', function(msg) {
   console.log('Hallo, ' + msg.hallo)
})
```

LiquidCore creates a convenient virtual file system so that instances of micro services do not unintentionally or maliciously interfere with each other or the rest of the Android filesystem.  The file system is described in detail [here](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-File-System).

# "Hallo, die Weld!" Micro Service Tutorial

#### Prerequisites

* A recent version of [Node.js] -- 8.9.3 or newer
* [Android Studio]

(You can find all the code below in a complete example project [here](https://github.com/LiquidPlayer/Examples/tree/master/HelloWorld) if you get stuck).

To use a micro service, you need two things: the micro service code, and a host app.

We will start by creating a very simple micro service, which does nothing more than send a welcome message to the host.  This will be served from a machine on our network.  Start by installing the command-line interface:

```
$ npm install -g liquidcore-cli
```

Next, generate a project called `HelloWorld` using the tool:

```
$ liquidcore init HelloWorld
$ cd HelloWorld && npm install
```

This will generate a small Hello World project for you.  We are going to change it a bit, but the important thing is that this sets everything up correctly and provides you with some nice features like a development server and production bundler.

Once installation has completed, edit the file `index.js` in your `HelloWorld` directory and replace its contents with the following:

```javascript
/* Hello, World! Micro Service */

// A micro service will exit when it has nothing left to do.  So to
// avoid a premature exit, let's set an indefinite timer.  When we
// exit() later, the timer will get invalidated.
setInterval(function() {}, 1000)

// Listen for a request from the host for the 'ping' event
LiquidCore.on( 'ping', function() {
    // When we get the ping from the host, respond with "Hallo, die Weld!"
    // and then exit.
    LiquidCore.emit( 'pong', { message: 'Hallo, die Weld!' } )
    process.exit(0)
})

// Ok, we are all set up.  Let the host know we are ready to talk
LiquidCore.emit( 'ready' )
```

Finally, you can now run your development server.

```
$ npm run server
```

This will fire off a server built on the [metro bundler](https://facebook.github.io/metro/en/).  Metro does everything we need and more, so if you've used the old `liquidserver` in the past, this replaces that.  Anyway, congratulations, you just created a micro service.  You can test that it is working correctly by navigating to
`http://localhost:8082/liquid.bundle?platform=android` in your browser.  You should be able to find the contents of `index.js` that you just created with some additional wrapper code.  The wrapper is simply
to allow multiple Node.js modules to be packed into a single file.  If you were to
`require()` some other module, that module and its dependencies would get wrapped into this
single file.

You can leave that running or restart it later.  Now we need to create a host app.

1. In Android Studio, create a new project by selecting `File -> New Project ...`
2. Fill out the basics and press `Next` (Application Name: `HelloWorld`, Company Domain: `liquidplayer.org`, Package name: `org.liquidplayer.examples.helloworld`)
3. Fill in the Target Devices information.  The defaults are fine.  Click `Next`
4. Select `Empty Activity` and then `Next`
5. The default options are ok.  Click `Finish`

You now have a basic app that does very little.  Go ahead and run it in your emulator.
If you hadn't figured out why we are creating a "Hallo, die Weld!" app, you probably can
see why by now.  You already get "Hello, World!" from Android Studio.  We're going to
make it speak German with our micro service.

Next, open the `res/layout/activity_main.xml` file.  We need to make a couple of modifications.  Replace the contents with the following:

```xml
<?xml version="1.0" encoding="utf-8"?>
<RelativeLayout xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:tools="http://schemas.android.com/tools"
    android:id="@+id/activity_main"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    android:paddingBottom="@dimen/activity_vertical_margin"
    android:paddingLeft="@dimen/activity_horizontal_margin"
    android:paddingRight="@dimen/activity_horizontal_margin"
    android:paddingTop="@dimen/activity_vertical_margin"
    tools:context="org.liquidplayer.examples.helloworld.MainActivity">

    <TextView
        android:id="@+id/text"
        android:layout_width="wrap_content"
        android:layout_height="wrap_content"
        android:text="Hello World!" />

    <Button
        android:id="@+id/button"
        android:layout_width="wrap_content"
        android:layout_height="wrap_content"
        android:layout_centerHorizontal="true"
        android:layout_centerVertical="true"
        android:text="Sprechen Sie Deutsch!"
        />
</RelativeLayout>
```

All we've changed is that we have given our `TextView` a name: `text`, and added a button.  Go ahead and run it again.  You should now see a big button in the middle.

Now it is time to connect LiquidCore.  First, you must add the library.  Go to your **root-level `build.grade`**
file and add the `jitpack` dependency:

```
...

allprojects {
    repositories {
        jcenter()
        maven { url 'https://jitpack.io' }
    }
}

...
```

Then, add the LiquidCore library to your **app's `build.gradle`**:

```
dependencies {
    ...
    implementation 'com.github.LiquidPlayer:LiquidCore:0.6.0'
}

```
Go ahead and sync to make sure the library downloads and links properly.  Run the app again to ensure that all is good.

Now, let's connect our button to the micro service.  Edit `MainActivity.java` in our app, and replace the contents with the following:

```java
package org.liquidplayer.examples.helloworld;

import android.os.Handler;
import android.os.Looper;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;
import org.liquidplayer.service.MicroService;
import org.liquidplayer.service.MicroService.ServiceStartListener;
import org.liquidplayer.service.MicroService.EventListener;

import java.net.URI;
import java.net.URISyntaxException;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final TextView textView = (TextView) findViewById(R.id.text);
        final Button button = (Button) findViewById(R.id.button);

        // Our 'ready' listener will wait for a ready event from the micro service.  Once
        // the micro service is ready, we'll ping it by emitting a "ping" event to the
        // service.
        final EventListener readyListener = new EventListener() {
            @Override
            public void onEvent(MicroService service, String event, JSONObject payload) {
                service.emit("ping");
            }
        };

        // Our micro service will respond to us with a "pong" event.  Embedded in that
        // event is our message.  We'll update the textView with the message from the
        // micro service.
        final EventListener pongListener = new EventListener() {
            @Override
            public void onEvent(MicroService service, String event, final JSONObject payload) {
                // NOTE: This event is typically called inside of the micro service's thread, not
                // the main UI thread.  To update the UI, run this on the main thread.
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            textView.setText(payload.getString("message"));
                        } catch (JSONException e) {
                            e.printStackTrace();
                        }
                    }
                });
            }
        };

        // Our start listener will set up our event listeners once the micro service Node.js
        // environment is set up
        final ServiceStartListener startListener = new ServiceStartListener() {
            @Override
            public void onStart(MicroService service) {
                service.addEventListener("ready", readyListener);
                service.addEventListener("pong", pongListener);
            }
        };

        // When our button is clicked, we will launch a new instance of our micro service.
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                URI uri = MicroService.DevServer();
                MicroService service = new MicroService(MainActivity.this, uri, startListener);
                service.start();
            }
        });
    }
}
```

Now, restart the app and then click the button.  The "Hello World" message should change to German.  You have successfully connected a micro service to a host app!

To demonstrate the instant update feature, leave the app and server running.  Now, edit `index.js` on your server machine to respond with a different message and then save:

```javascript
...
    LiquidCore.emit( 'pong', { message: 'Das ist super!' } )
...
```

Go back to the app and press the button again.  Your message should update.

That's it.  That's all there is to it.  Of course, this is an overly simplified example.  You have all of the capabilities of Node.js at your disposal.

A quick note about the `MicroService.DevServer()`: this generates convenience URL which points to the loopback address (`10.0.2.2`) on Android, which is used to serve the emulator from the host machine.  This won't work on actual hardware.  You would need to replace this with an actual URL.  `MicroService.DevServer()` assumes the entry file is named `liquid.js` and the server is running on port 8082.  Both of these assumptions can be changed by providing arguments, e.g. `MicroService.DevServer("another_file.bundle", 8888)` would generate a URL to fetch a bundle with an entry point of `another_file.js` on port 8888. 

# Native Add-ons

Introduced in version 0.6.0 is experimental support for native node modules (add-ons).  In Node, native add-ons are compiled (or os/architecture-specific prebuilts are downloaded) during `npm install`.  For example, `npm install sqlite3` installs the JavaScript interface to SQLite3 to `node_modules/sqlite3`, but it also compiles a native module `node_sqlite3.node` which is a dynamic library that contains the C-language SQLite3 library and native V8 interface code.  The code is built for the specific machine it is running on using `node-gyp`.

Unfortunately, there are several issues with this on mobile devices.  Primarily, although dynamic loading of libraries is supported, for security reasons, those libraries must be embedded in the APK to be used.  So it is not possible to download a native library at runtime and link to it (unlike with pure JavaScript modules, where this is perfectly ok).  Secondly, `node-gyp` is not really a cross-compiler (although some have hacked it for this purpose).  That is, it is optimized to build for the machine on which it is being run (e.g. your Mac or Linux machine), not for some remote device like a mobile phone, and not for multiple architectures at once (i.e. ARM, ARM64, X86, and X86_64).

To support these requirements, native modules have to be modified to work with LiquidCore.  Documentation, frameworks and tools for how to build a native module for LiquidCore are forthcoming, but for now, here is an example of how to use an existing add-on.  [This fork of node-sqlite3](https://github.com/LiquidPlayer/node-sqlite3) has been modified for use with LiquidCore.  We can install it using `npm`.

In your Android project's root directory, create an empty `package.json` file.  This will allow us to install node modules locally.

```
% echo "{}" > package.json
```

Then, install `@liquidcore/sqlite3`:

```
% npm i @liquidcore/sqlite3
```

Now, generate the gradle file(s) to include the library into your app using the `liquidcore` utility (make sure you have installed at least version 0.4.4 of `liquidcore-cli`):

```
% liquidcore gradle
```

This will generate a file called `liquidcore.build.gradle`.  Simply include this file in your own app's `build.gradle` by adding the following line near the top:

```gradle
apply from: new File(rootProject.projectDir, 'liquidcore.build.gradle')
```

That's it.  The SQLite3 add-on will now be available for your JavaScript code to use.

To use it, install `@liquidcore/sqlite3` instead of `sqlite3` with `npm` in your JavaScript project and use the module exactly as you would otherwise.

If there are specific native modules that you would like to use with LiquidCore, please file an issue to request it.  In the near term, I will build a few in order to document and simplify the process (it is a bit arduous at the moment) or I can help you to build it.  Once the documentation stabilizes, I will stop.

# Building the LiquidCore Android library

If you are interested in building the library directly and possibly contributing, you must
clone the repository:

    % git clone https://github.com/liquidplayer/LiquidCore.git

You can then use the library locally, by specifying the `--liquidcore` option when creating your gradle files:

    % liquidcore gradle --liquidcore=/path/to/local/LiquidCore

This will generate two files, `liquidcore.build.gradle` and `liquidcore.settings.gradle`.  Include them into your project's gradle files as described in the output of the `liquidcore gradle` command.
    
##### Note

The Node.js library (`libnode.so`) is pre-compiled and included in binary form in
`deps/node-8.9.3/prebuilt`.  All of the modifications required to produce the library are included in `deps/node-8.9.3`.  To build each library (if you so choose), see the instructions [here](https://github.com/LiquidPlayer/LiquidCore/wiki/How-to-build-libnode.so).

# License

Copyright (c) 2014 - 2019 LiquidPlayer

Distributed under the MIT License.  See [LICENSE.md](https://github.com/LiquidPlayer/LiquidCore/blob/master/LICENSE.md) for terms and conditions.

[Node.js]:https://nodejs.org/
[Android Studio]:https://developer.android.com/studio/index.html
[BigNumber]:https://github.com/MikeMcl/bignumber.js/
