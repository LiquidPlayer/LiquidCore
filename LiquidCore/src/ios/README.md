# The LiquidCore Project for iOS

[![Download](https://img.shields.io/npm/dt/liquidcore.svg)](https://www.npmjs.com/package/liquidcore)

[![NPM](https://nodei.co/npm/liquidcore.png)](https://nodei.co/npm/liquidcore/)

LiquidCore enables [Node.js](https://nodejs.org) virtual machines to run inside iOS apps.  It provides a complete runtime environment, including a virtual file system.

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
$ npx liquidcore init
```

The `init` step will add some utility scripts and the `liquidcore` object to your `package.json` file.  It will also create an example service called `example.js`, which will get packaged into your app.  You can change / add files to be packaged by editing the `liquidcore.entry` property in your `package.json`.

#### Step 3: Configure your mobile app project

```bash
$ npx liquidcore pod-config --target=<target> --podfile=<podfile>
$ npx liquidcore bundler --platform=ios
$ pod install
```

where `<target>` is your XCode project target, and `<podfile>` is the path of your application's `Podfile`

### Notes

LiquidCore requires the use of [Cocoapods](https://cocoapods.org/), so make sure you've set up your project to use a [`Podfile`](https://guides.cocoapods.org/using/the-podfile.html) first.  **Important**: Your `Podfile` must contain the `use_frameworks!` directive.  If you are not yet using Cocoapods, you can create a simple one like this:

```ruby
platform :ios, '11.0'
use_frameworks!

target '<TARGET>' do
end
```

where `<TARGET>` is the name of your `xcodeproj` (without the `.xcodeproj` extension).

Also, any time you add a new `liquidcore.entry` point in `package.json`, you must first run the bundler and then run `pod install` again.  This is a quirk of how Cocoapods works with finding files.  Those files must exist at installation time, or they will not be available in the pod, even if they are created later.  So after adding a new `entry`, just do this part again:

```bash
$ npx liquidcore bundler --platform=ios
$ pod install
```

## Automatic Bundling

One of the newest features in 0.7.0+ is the ability to automatically bundle JavaScript files in the application build process.  This is configured in the `pod-config` step above.  The bundling options are stored in the local `package.json` file in the `liquidcore` property.  A typical file `liquidcore` object may look something like this:

```json
  "liquidcore": {
    "entry": [
      "example.js",
      "index.js"
    ],
    "bundler_output": {
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

If you have a non-standard configuration for your app, you may have to change some of these values.  

Bundling is a convenient way to test and package your JavaScript projects.  The bundler uses [Metro](https://facebook.github.io/metro/) to package up all of your required node modules into a single file that can be packaged as a resource in your app.  If you are running on the Simulator, you can run a local server on your development machine and hot-edit your JavaScript code by `npm run server` in your project root.  If you are using the `Bundle` API (described below), and your app is built in debug mode, it will first attempt to get the bundles from the server.  If the server is not available, it will use the automated bundle packaged at build time.  In release mode, it will always use the packaged bundle.

## Usage

### The `MicroService` API

A micro service is nothing more than an independent Node.js instance whose startup code is referenced by a URI.  For example:

```swift
import LiquidCore
...
let url = LCMicroService.bundle("example")
let service = LCMicroService(url: url)
service?.start()
```

The service URL can either refer to a server URL or a local resource.  For services that are automatically bundled with your app by LiquidCore, you can use the `LCMicroService.bundle()` method to generate the correct URL.  Any javascript entry files referenced in your `package.json` in `liquidcore.entry` will get bundled automatically with each build.  By default, the initialization script creates and packages `example.js`, but you can easily change this.

A micro service can communicate with the host app once the Node.js environment is set up.  This can be determined by adding a start listener in the constructor:

Your class must conform to `LCMicroServiceDelegate`.

```swift
let service = LCMicroService(url:url,
                        delegate:self)
service?.start()
...
func onStart(_ service: LCMicroService) {
    // .. The environment is live, but the
    // startup JS code (from the URI) has
    // not been executed yet.
}
```

A micro service communicates with the host through a simple [`EventEmitter`](https://nodejs.org/docs/latest-v10.x/api/events.html) interface, eponymously called `LiquidCore`.  For example, in your JavaScript startup code:

```javascript
LiquidCore.emit('my_event', {foo: "hello, world", bar: 5, l337 : ['a', 'b'] })
```

On the app side, the host app can listen for events:

Your class must conform to `LCMicroServiceEventListener`.

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

Similarly, the micro service can listen for events from the host:

```swift
var payload = ["hallo" : "die Weld"]
service.emitObject("host_event", object:payload)
```

Then, in Javascript:

```javascript
LiquidCore.on('host_event', function(msg) {
   console.log('Hallo, ' + msg.hallo)
})
```

LiquidCore creates a convenient virtual file system so that instances of micro services do not unintentionally or maliciously interfere with each other or the rest of the Android/iOS filesystem.  The file system is described in detail [here](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-File-System).


## Playing with `example.js`

When you follow the directions above, LiquidCore will automatically bundle a file called
`example.js`, which looks like this:

```javascript
const {LiquidCore} = require('liquidcore')

// A micro service will exit when it has nothing left to do.  So to
// avoid a premature exit, set an indefinite timer.  When we
// exit() later, the timer will get invalidated.
setInterval(()=>{}, 1000)

console.log('Hello, World!')

// Listen for a request from the host for the 'ping' event
LiquidCore.on( 'ping', () => {
    // When we get the ping from the host, respond with "Hello, World!"
    // and then exit.
    LiquidCore.emit( 'pong', { message: 'Hello, World from LiquidCore!' } )
    process.exit(0)
})

// Ok, we are all set up.  Let the host know we are ready to talk
LiquidCore.emit( 'ready' )
```

Below is an example of how to interact with this JavaScript code from the app.  Note that `textBox` on iOS is a UI element whose setup is not shown here.

```swift
import UIKit
import LiquidCore

class ViewController: UIViewController,
  LCMicroServiceDelegate,
  LCMicroServiceEventListener {
    
  @IBOutlet weak var textBox: UITextField!
    
  override func viewDidLoad() {
    super.viewDidLoad()
        
    let url = LCMicroService.bundle("example")
    let service = LCMicroService(url: url, 
                                delegate: self)
    service?.start()
  }

  func onStart(_ service: LCMicroService) {
    service.addEventListener("ready",listener: self)
    service.addEventListener("pong", listener: self)
  }
    
  func onEvent(_ service: LCMicroService, 
                   event: String,
                 payload: Any?) {
    if event == "ready" {
      service.emit("ping")
    } else if event == "pong" {
      let p = (payload as! Dictionary<String,AnyObject>)
      let message = p["message"] as! String
      DispatchQueue.main.async {
        self.textBox.text = message
      }
    }
  }
}
```

You can use this as a guide for how to create your own services.  You can use `npm install` to install most JS-only (non-native) modules and `require` them as normal.  The bundler will package all of the code into a single file.


## API Documentation

[**iOS Objective-C/Swift**](https://liquidplayer.github.io/LiquidCoreiOS/index.html)


## License

Copyright (c) 2014 - 2020 LiquidPlayer

Distributed under the MIT License.  See [LICENSE.md](https://github.com/LiquidPlayer/LiquidCore/blob/master/LICENSE.md) for terms and conditions.
