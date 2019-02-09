# The LiquidCore Project

LiquidCore enables Node.js virtual machines to run inside Android and iOS apps.  It provides a complete runtime environment, including a virtual file system.

Version
-------
[0.6.0](https://github.com/LiquidPlayer/LiquidCore/releases/tag/0.6.0)

LiquidCore is distributed using [CocoaPods](https://cocoapods.org/).  In your project's `Podfile`, add:

```
pod 'LiquidCore'
```

API Documentation
-----------------
[Version 0.6.0](https://liquidplayer.github.io/LiquidCoreiOS/0.6.0/index.html)

# Table of Contents

1. [Architecture](#architecture)
2. [Node `LCProcess`](#node-lcprocess) - Run raw Node.js instances on iOS
3. [The `LCMicroService`](#the-lcmicroservice) - Run enhanced Node.js instances on iOS
3. ["Hallo, die Weld!" Micro Service Tutorial](#hallo-die-weld-micro-service-tutorial)
6. [Native add-ons](#native-add-ons) 
4. [Building the LiquidCore iOS framework](#building-the-liquidcore-ios-framework)
5. [License](#license)

# Architecture

![iOS Architecture Diagram](https://github.com/LiquidPlayer/LiquidCore/raw/master/doc/ArchitectureiOS.png)

LiquidCore for iOS includes the Node.js runtime, but without the V8 backend.  Instead, it marshalls calls to V8 through an interpreter to Apple's JavaScriptCore engine.  It provides two APIs for apps to interact with:

* **Node [`LCProcess`](#node-lcprocess) API**, which allows developers to launch fast isolated instances of the Node.js runtime.
* **[`LCMicroService`](#the-lcmicroservice) API**, which is an abstraction of a Node.js process and supports dynamic code fetching and native add-ons.

Native add-ons enable extending the basic runtime environment with additional native functionality.  Add-ons have access to the above APIs, plus the ability to use the V8 API.  This allows projects that depend on V8, such native Node modules to use LiquidCore directly.


# Node `LCProcess`

[Jazzy API Doc v0.6.0](https://liquidplayer.github.io/LiquidCoreiOS/0.6.0/Classes/LCProcess.html)

LiquidCore allows creation of raw Node.js instances.  Each instance runs in its own thread and is isolated from all other instances.  Instances can share a [virtual file system](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-File-System), by using a common unique identifier.

It is not recommended to use the `LCProcess` API directly for most use cases. The [`LCMicroService`](#the-lcmicroservice) API is more robust and has additional functionality, like support for native modules and downloadable JavaScript bundles.


# The `LCMicroService`

[Jazzy API Doc v0.6.0](https://liquidplayer.github.io/LiquidCoreiOS/0.6.0/Classes/LCMicroService.html)

A *micro app* is built on a *micro service*.  A micro service is nothing more than an independent Node.js instance whose startup code is referenced by a URI.  For example:

#### Swift
```swift
import LiquidCore
...

let url = URL(string: "http://my.server.com/path/to/code.js")
let service = LCMicroService(url: url!)
service?.start()
```

#### Objective-C
```objective-c
#import <LiquidCore/LiquidCore.h>
...

NSURL *url = [NSURL URLWithString:@"http://my.server.com/path/to/code.js"];
LCMicroService *service = [[LCMicroService alloc] initWithURL:url];
[service start];
```

The service URI can either refer to a server URL or a local resource (e.g. `NSURL *url = [[NSBundle mainBundle]
    URLForResource: @"somefile" withExtension:@"js"];`).  LiquidCore is designed to primarily use remote URLs, as dynamic updates are an important value proposition, but local resources are also supported.

A micro service can communicate with the host app once the Node.js environment is set up.  This can be determined by specifying an `LCMicroServiceDelegate` in the `LCMicroService` constructor:

#### Swift
```swift
let url = URL(string: "http://my.server.com/path/to/code.js")
let service = LCMicroService(url: url!, delegate:self)
service?.start()

...
func onStart(_ service: LCMicroService) {
    // .. The environment is live, but the startup JS code (from the URI)
    // has not been executed yet.
}
```

#### Objective-C
```objective-c
NSURL *url = [NSURL URLWithString:@"http://my.server.com/path/to/code.js"];
LCMicroService *service = [[LCMicroService alloc] initWithURL:url delegate:self];
[service start];

...

- (void) onStart:(LCMicroService*)service
{
    // .. The environment is live, but the startup JS code (from the URI)
    // has not been executed yet.
}
```

A micro service communicates with the host through a simple [`EventEmitter`](https://nodejs.org/api/events.html#events_class_eventemitter) interface, eponymously called `LiquidCore`.  For example, in your JavaScript startup code (code.js in this example):

```javascript
LiquidCore.emit('my_event', {foo: "hello, world", bar: 5, l337 : ['a', 'b'] })
```

On the iOS side, the host app can listen for events through the `LCMicroServiceEventListener` protocol:

#### Swift
```swift
// ... in the onStart:synchronizer: method:
service.addEventListener("my_event", listener: self)

...
func onEvent(_ service: LCMicroService, event: String, payload: Any?) {
    var p = (payload as! Dictionary<String,AnyObject>)
    NSLog(format:"Event: %@: %@", args:event, p["foo"]);
    // logs: Event:my_event: hello, world
} 
```

#### Objective-C
```objective-c
// ... in the onStart:synchronizer: method:
[service addEventListener:@"my_event" listener:self];

...

- (void) onEvent:(LCMicroService*)service event:(NSString*)event payload:(id _Nullable)payload
{
    NSLog(@"Event: %@: %@", event, payload[@"foo"]);
    // logs: Event:my_event: hello, world
}

```

Similarly, the micro service can listen for events from the host:

#### Swift
```swift
var payload = ["hallo" : "die Weld"]
service.emitObject("host_event", object:payload)
```

#### Objective-C
```objective-c
NSDictionary *payload = @{ @"hallo" : @"die Weld" };
[service emitObject:@"host_event" object:payload];
```

Then, in Javascript:

```javascript
LiquidCore.on('host_event', function(msg) {
   console.log('Hallo, ' + msg.hallo)
})
```

LiquidCore creates a convenient virtual file system so that instances of micro services do not unintentionally or maliciously interfere with each other or the rest of the iOS filesystem.  The file system is described in detail [here](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-File-System).


# "Hallo, die Weld!" Micro Service Tutorial

#### Prerequisites

* A recent version of [Node.js] -- 8.9.3 or newer
* [XCode]

(You can find all the code below in a complete example project [here](https://github.com/LiquidPlayer/Examples/tree/master/HelloWorld) if you get stuck).

To use a micro service, you need two things: the micro service code, and a host app.

We will start by creating a very simple micro service, which does nothing more than send a welcome message to the host.  This will be served from a machine on our network.  Start by installing the command-line interface:

```
$ npm install -g liquidcore-cli
```

Next, generate a project called `helloworld` using the tool:

```
$ liquidcore init helloworld
$ cd helloworld && npm install
```

This will generate a small Hello World project for you.  We are going to change it a bit, but the important thing is that this sets everything up correctly and provides you with some nice features like a development server and production bundler.

Once installation has completed, edit the file `index.js` in your `helloworld` directory and replace its contents with the following:

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
`http://localhost:8082/liquid.bundle?platform=ios` in your browser.  You should be able to find the contents of `index.js` that you just created with some additional wrapper code.  The wrapper is simply
to allow multiple Node.js modules to be packed into a single file.  If you were to
`require()` some other module, that module and its dependencies would get wrapped into this
single file.

You can leave that running or restart it later.  Now we need to create a host app.

1. In XCode, create a new project by selecting File->New->Project->Single View App
2. Fill out the basics and press `Next` (Product Name: `HelloWorld`, Organization Name: `LiquidPlayer`, Language: `Swift`).  Leave the rest as defaults.
3. Select a location for it and press `Create`
4. Open the `ViewController.swift` file and replace the code with the following contents:

```swift
import UIKit

class ViewController: UIViewController {

    var text: UILabel = UILabel()
    var button: UIButton = UIButton(type: .system)
    
    override func viewDidLoad() {
        super.viewDidLoad()

        text.textAlignment = .center
        text.text = "Hello World!"
        text.font = UIFont(name: "Menlo", size: 17)
        self.view.addSubview(text)
        
        button.setTitle("Sprechen Sie Deutsch!", for: .normal)
        button.titleLabel?.font = UIFont(name: "Menlo", size: 17)
        button.addTarget(self, action: #selector(onTouch), for: .touchUpInside)
        self.view.addSubview(button)
        
        self.text.translatesAutoresizingMaskIntoConstraints = false
        self.button.translatesAutoresizingMaskIntoConstraints = false
        
        let top = UILayoutGuide()
        let bottom = UILayoutGuide()
        self.view.addLayoutGuide(top)
        self.view.addLayoutGuide(bottom)
        
        let views = [ "text": text, "button": button, "top":top, "bottom":bottom ]
        let c1 = NSLayoutConstraint.constraints(withVisualFormat: "H:|-[text]-|", metrics: nil, views: views)
        let c2 = NSLayoutConstraint.constraints(withVisualFormat: "H:|-[button]-|", metrics: nil, views: views)
        let c3 = NSLayoutConstraint.constraints(withVisualFormat: "V:|[top]-[text]-[button]-[bottom(==top)]|",
                                                metrics: nil, views: views)
        self.view.addConstraints(c1 + c2 + c3)
    }
    
    @objc func onTouch(sender:UIButton!) {
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

}


```

You now have a basic app that does very little.  Go ahead and run it in your simulator.
You should see a white background with a message that says "Hello World!" and a button below it that says "Sprechen Sie Deutsch".  This is just a simple Hello World app.  We are going to teach it to speak German by using our LiquidCore micro service.

Now it is time to connect LiquidCore.  First, you must add the framework.  Follow the instructions at the top of the file for this.  The first time you run `carthage update`, it will take a long time as it needs to clone the entire repo.  But after the first time, it should be quicker.  Everything should continue working the same.  Once this is done, re-run your app.  It should continue working as before.

Now, let's connect our button to the micro service.  Edit `ViewController.swift` in our app, and replace the first couple of lines with the following:

```swift
import UIKit
import LiquidCore

class ViewController: UIViewController, LCMicroServiceDelegate, LCMicroServiceEventListener {
```

Now, replace the `onTouch()` function with the following:

```swift
    @objc func onTouch(sender:UIButton!) {
        let url = LCMicroService.devServer()
        let service = LCMicroService(url: url!, delegate: self)
        service?.start()
    }
    
    func onStart(_ service: LCMicroService) {
        service.addEventListener("ready", listener: self)
        service.addEventListener("pong", listener: self)
    }
    
    func onEvent(_ service: LCMicroService, event: String, payload: Any?) {
        if event == "ready" {
            service.emit("ping")
        } else if event == "pong" {
            DispatchQueue.main.async {
                var p = (payload as! Dictionary<String,AnyObject>)
                self.text.text = p["message"] as? String
            }
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

A quick note about the `LCMicroService.devServer()`: this generates convenience URL which points to the loopback address (`localhost`) on iOS, which is used to serve the simulator from the host machine.  This won't work on actual hardware.  You would need to replace this with an actual URL.  `LCMicroService.devServer()` assumes the entry file is named `liquid.js` and the server is running on port 8082.  Both of these assumptions can be changed by providing arguments, e.g. `LCMicroService.devServer(fileName:"another_file.bundle", port:8888)` would generate a URL to fetch a bundle with an entry point of `another_file.js` on port 8888. 


# Native Add-ons

Introduced in version 0.6.0 is experimental support for native node modules (add-ons).  In Node, native add-ons are compiled (or os/architecture-specific prebuilts are downloaded) during `npm install`.  For example, `npm install sqlite3` installs the JavaScript interface to SQLite3 to `node_modules/sqlite3`, but it also compiles a native module `node_sqlite3.node` which is a dynamic library that contains the C-language SQLite3 library and native V8 interface code.  The code is built for the specific machine it is running on using `node-gyp`.

Unfortunately, there are several issues with this on mobile devices.  Primarily, although dynamic frameworks are supported, for security reasons, those frameworks must be embedded in the app to be used.  So it is not possible to download a native framework at runtime and link to it (unlike with pure JavaScript modules, where this is perfectly ok).  Secondly, `node-gyp` is not really a cross-compiler (although some have hacked it for this purpose).  That is, it is optimized to build for the machine on which it is being run (e.g. your Mac machine), not for some remote device like a mobile phone, and not for multiple architectures at once (i.e. ARM64, X86_64).

To support these requirements, native modules have to be modified to work with LiquidCore.  Documentation, frameworks and tools for how to build a native module for LiquidCore are forthcoming, but for now, here is an example of how to use an existing add-on.  [This fork of node-sqlite3](https://github.com/LiquidPlayer/node-sqlite3) has been modified for use with LiquidCore.  We can install it using `npm`.

In your iOS project's root directory, create an empty `package.json` file.  This will allow us to install node modules locally.

```
% echo "{}" > package.json
```

Then, install `@liquidcore/sqlite3`:

```
% npm i @liquidcore/sqlite3
```

Now, generate the `Podfile` to include the framework into your app using the `liquidcore` utility (make sure you have installed at least version 0.4.4 of `liquidcore-cli`):

```
% liquidcore pod <project-name> > Podfile
```

This will generate a Podfile with all required pods.  If you are already using CocoaPods, you can just copy the `source` and `pod` lines from this output into your existing `Podfile`.

That's it.  The SQLite3 add-on will now be available for your JavaScript code to use.

To use it, install `@liquidcore/sqlite3` instead of `sqlite3` with `npm` in your JavaScript project and use the module exactly as you would otherwise.

If there are specific native modules that you would like to use with LiquidCore, please file an issue to request it.  In the near term, I will build a few in order to document and simplify the process (it is a bit arduous at the moment) or I can help you to build it.  Once the documentation stabilizes, I will stop.

# Building the LiquidCore iOS framework

If you are interested in building the library directly and possibly contributing, you must
do the following:

    % git clone https://github.com/liquidplayer/LiquidCore.git
    % cd LiquidCore
    % pod lib lint

You can use the pod locally, by specifying the `--liquidcore` option when creating your podfile:

```
% liquidcore pod <project-name> --liquidcore=/path/to/local/LiquidCore
```


# License

Copyright (c) 2014 - 2019 LiquidPlayer

Distributed under the MIT License.  See [LICENSE.md](https://github.com/LiquidPlayer/LiquidCore/blob/master/LICENSE.md) for terms and conditions.

[Node.js]:https://nodejs.org/
[XCode]:https://developer.apple.com/xcode/
[BigNumber]:https://github.com/MikeMcl/bignumber.js/
