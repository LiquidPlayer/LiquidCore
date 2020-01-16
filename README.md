# The LiquidCore Project

# NOTE: Version 0.7.0 is not yet stable, please be patient ...

[![Download](https://img.shields.io/npm/dt/liquidcore.svg)](https://www.npmjs.com/package/liquidcore)

[![NPM](https://nodei.co/npm/liquidcore.png)](https://nodei.co/npm/liquidcore/)

LiquidCore enables [Node.js](https://nodejs.org) virtual machines to run inside Android and iOS apps.  It provides a complete runtime environment, including a virtual file system.

LiquidCore also provides a convenient way for Android developers to [execute raw JavaScript](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-as-a-Raw-JavaScript-engine-for-Android-(v.-0.7.0-)) inside of their apps, as iOS developers can already do natively with JavaScriptCore.

## Installation

#### Step 1: Make sure your project is configured for use with `npm`
In the root directory of your project, you must have a `package.json` file.  If you do not already
have one, you can create it by running:

```bash
$ npm init
```
and following the steps in the wizard.

#### Step 2: Install LiquidCore and configure `package.json`

```bash
$ npm i liquidcore
$ node node_modules/liquidcore/lib/cli.js init
```

The `init` step will add some utility scripts and the `liquidcore` object to your `package.json` file.  It will also create an example service called `example.js`, which will get packaged into your app.  You can change / add files to be packaged by editing the `liquidcore.entry` property in your `package.json`.

#### Step 3: Configure your mobile app project

<table ><tbody><tr><td>

#### Android

</td><td>

#### iOS

</td></tr><tr><td>

```bash
$ npm run gradle-config -- --module=<app>
```

where `<app>` is the name of your application module (the default in Android Studio is 'app').
</td><td>

```bash
$ npm run pod-config -- --target=<target> --podfile=<podfile>
```

where `<target>` is your XCode project target, and `<podfile>` is the path of your application's `Podfile`
</td></tr></tbody>
</table>

Note: On iOS, LiquidCore requires the use of [Cocoapods](https://cocoapods.org/), so make sure you've set up your project to use a [`Podfile`](https://guides.cocoapods.org/using/the-podfile.html) first.

## Automatic Bundling

One of the newest features in 0.7.0+ is the ability to automatically bundle JavaScript files in the application build process.  This is configured in the `gradle-config` and/or `pod-config` steps above.  The bundling options are stored in the local `package.json` file in the `liquidcore` property.  A typical file `liquidcore` object may look something like this:

```json
  "liquidcore": {
    "entry": [
      "example.js",
      "index.js"
    ],
    "gradle_options": {
      "module": "app"
    },
    "bundler_output": {
      "android": "app/src/main/res/raw",
      "ios": ".liquidcore/ios_bundle"
    },
    "bundler_options": {
      "minify": false
    },
    "pod_options": {
      "dev": true,
      "target": "TestApp"
    }
  }
```

To include a new bundle, simply put the entry point JavaScript file in the `entry` array property.  LiquidCore will generate one bundle for each `entry` during the build process.

If you have a non-standard configuration for your app, you may have to change some of these values.  For example, `bundler_output` for Android assumes that your resources directory is at `<app-module>/src/main/res`.  This is the Android Studio default.  If you have changed this to something else, you will need to update this property.

Bundling is a convenient way to test and package your JavaScript projects.  The bundler uses [Metro](https://facebook.github.io/metro/) to package up all of your required node modules into a single file that can be packaged as a resource in your app.  If you are running on the Android Emulator or iOS Simulator, you can run a local server on your development machine and hot-edit your JavaScript code by `npm run server` in your project root.  If you are using the `Bundle` API (described below), and your app is built in debug mode, it will first attempt to get the bundles from the server.  If the server is not available, it will use the automated bundle packaged at build time.  In release mode, it will always use the packaged bundle.

## Usage

### The `MicroService` API

A micro service is nothing more than an independent Node.js instance whose startup code is referenced by a URI.  For example:

<table ><tbody><tr><td>

#### Android Kotlin

</td><td>

#### iOS Swift

</td></tr><tr><td>

```kotlin
val uri = MicroService.Bundle(androidContext, "example")
val service = MicroService(androidContext, uri)
service.start()
```

</td><td>

```swift
import LiquidCore
...
let url = LCMicroService.bundle("example")
let service = LCMicroService(url: url!)
service?.start()
```

</td></tr></tbody>
</table>

The service URI can either refer to a server URL or a local resource.  For services that are automatically bundled with your app by LiquidCore, you can use the `MicroService.Bundle()` or `LCMicroService.bundle()` methods to generate the correct URI.  Any javascript entry files referenced in your `package.json` in `liquidcore.entry` will get bundled automatically with each build.  By default, the initialization script creates and packages `example.js`, but you can easily change this.

A micro service can communicate with the host app once the Node.js environment is set up.  This can be determined by adding a start listener in the constructor:

<table ><tbody><tr><td>

#### Android Kotlin
</td><td>

#### iOS Swift
</td></tr><tr><td>

```kotlin
val uri = MicroService.Bundle(androidContext, "example")
val startListener = MicroService.ServiceStartListener {
    // .. The environment is live, but the startup
    // JS code (from the URI) has not been executed yet.
}
val service = MicroService(androidContext, uri,
    startListener)
service.start()
```
</td><td>

```swift
let service = LCMicroService(url:url!,
                        delegate:self)
service?.start()
...
func onStart(_ service: LCMicroService) {
    // .. The environment is live, but the
    // startup JS code (from the URI) has
    // not been executed yet.
}
```
</td></tr></tbody>
</table>

A micro service communicates with the host through a simple [`EventEmitter`](https://nodejs.org/docs/latest-v10.x/api/events.html) interface, eponymously called `LiquidCore`.  For example, in your JavaScript startup code:

```javascript
LiquidCore.emit('my_event', {foo: "hello, world", bar: 5, l337 : ['a', 'b'] })
```

On the app side, the host app can listen for events:

<table ><tbody><tr><td>

#### Android Kotlin
</td><td>

#### iOS Swift
</td></tr><tr><td>

```kotlin
val listener = MicroService.EventListener { 
    service, event, payload ->
    android.util.Log.i("Event:" + event,
        payload.getString("foo"))
    // logs: I/Event:my_event: hello, world
}
service.addEventListener("my_event", listener)
```

</td><td>

```swift
service.addEventListener("my_event", listener:self)
...
func onEvent(_ service: LCMicroService, event: String, 
               payload: Any?) {
    var p = (payload as! Dictionary<String,AnyObject>)
    NSLog(format:"Event: %@: %@", args:event, p["foo"]);
    // logs: Event:my_event: hello, world
} 
```
</td></tr></tbody>
</table>

Similarly, the micro service can listen for events from the host:

<table ><tbody><tr><td>

#### Android Kotlin
</td><td>

#### iOS Swift
</td></tr><tr><td>

```kotlin
val payload = JSONObject()
payload.put("hallo", "die Weld")
service.emit("host_event", payload)
```

</td><td>

```swift
var payload = ["hallo" : "die Weld"]
service.emitObject("host_event", object:payload)
```

</td></tr></tbody>
</table>

Then, in Javascript:

```javascript
LiquidCore.on('host_event', function(msg) {
   console.log('Hallo, ' + msg.hallo)
})
```

LiquidCore creates a convenient virtual file system so that instances of micro services do not unintentionally or maliciously interfere with each other or the rest of the Android/iOS filesystem.  The file system is described in detail [here](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-File-System).

## API Documentation

[**Android Javadocs (liquidcore-Nodejs)**](https://liquidplayer.github.io/LiquidCoreAndroid/index.html)

[**Android Javadocs (liquidcore-V8)**](https://liquidplayer.github.io/LiquidV8/index.html)

[**iOS Objective-C/Swift**](https://liquidplayer.github.io/LiquidCoreiOS/index.html)


## License

Copyright (c) 2014 - 2020 LiquidPlayer

Distributed under the MIT License.  See [LICENSE.md](https://github.com/LiquidPlayer/LiquidCore/blob/master/LICENSE.md) for terms and conditions.
