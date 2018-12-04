The LiquidCore Project
----------------------

LiquidCore enables Node.js virtual machines to run inside Android and iOS apps.  It provides a complete runtime environment, including a virtual file system and native SQLite3 support.

LiquidCore also provides a convenient way for Android developers to [execute raw JavaScript](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-as-a-Native-Javascript-Engine) inside of their apps, as iOS developers can already do natively with JavaScriptCore.

For a description of how to use LiquidCore, please see the appropriate README under [LiquidCoreAndroid](https://github.com/LiquidPlayer/LiquidCore/tree/master/LiquidCoreAndroid) or [LiquidCoreiOS](https://github.com/LiquidPlayer/LiquidCore/tree/master/LiquidCoreiOS).

Version
-------
[0.5.1](https://github.com/LiquidPlayer/LiquidCore/releases/tag/0.5.1)

[![Release](https://jitpack.io/v/LiquidPlayer/LiquidCore.svg)](https://jitpack.io/#LiquidPlayer/LiquidCore)
[![Downloads](https://jitpack.io/v/LiquidPlayer/LiquidCore/week.svg)](https://jitpack.io/#LiquidPlayer/LiquidCore)
[![Carthage compatible](https://img.shields.io/badge/Carthage-compatible-4BC51D.svg?style=flat)](https://github.com/Carthage/Carthage)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

API Documentation
-----------------
**Android Javadocs**: [Version 0.5.1](https://liquidplayer.github.io/LiquidCoreAndroid/0.5.1/index.html)

**iOS Objective-C/Swift**: [Version 0.5.1](https://liquidplayer.github.io/LiquidCoreiOS/0.5.1/index.html)

Architecture
------------

The diagrams below generally represent reality, although they are aligned with the vocabulary that will be used in version 0.6.0.  Plugins are currently referred to as "surfaces" in versions 0.5.x, but this will change shortly.

### Android

![Android architecture diagram](https://github.com/LiquidPlayer/LiquidCore/raw/master/doc/ArchitectureAndroid.png)

LiquidCore for Android includes the Node.js runtime and V8 backend.  In addition, it provides three APIs for apps to interact with:

* **[Java / JavaScript JNI](https://liquidplayer.github.io/LiquidCoreAndroid/0.5.1/org/liquidplayer/javascript/package-frame.html) API**, which provides a convenient way to run raw JavaScript code from within Java, without the need for a clunky `WebView`.
* **Node [`Process`](https://liquidplayer.github.io/LiquidCoreAndroid/0.5.1/org/liquidplayer/node/Process.html) API**, which allows developers to launch fast isolated instances of the Node.js runtime
* **[`MicroService`](https://liquidplayer.github.io/LiquidCoreAndroid/0.5.1/org/liquidplayer/service/MicroService.html) API**, which is an abstraction of a Node.js process and supports dynamic code fetching and native plugins 

Native plugins enable extending the basic runtime environment with additional native functionality.  Plugins have access to all the above APIs, plus the ability to use [WebKit's JavaScriptCore API](https://developer.apple.com/documentation/javascriptcore?language=objc) running on top of V8.  This allows projects that depend on JavaScriptCore, like [React Native](https://facebook.github.io/react-native/), to use LiquidCore directly.

### iOS

![iOS architecture diagram](https://github.com/LiquidPlayer/LiquidCore/raw/master/doc/ArchitectureiOS.png)

LiquidCore for iOS includes the Node.js runtime, but without the V8 backend.  Instead, it marshalls calls to V8 through an interpreter to Apple's JavaScriptCore engine.  It provides two APIs for apps to interact with:

* **Node `Process` API**, which allows developers to launch fast isolated instances of the Node.js runtime
* **[`LCMicroService`](https://liquidplayer.github.io/LiquidCoreiOS/0.5.1/Classes/LCMicroService.html) API**, which is an abstraction of a Node.js process and supports dynamic code fetching and native plugins 

Native plugins enable extending the basic runtime environment with additional native functionality.  Plugins have access to the above APIs, plus the ability to use the V8 API.  This allows projects that depend on V8, such native Node modules to use LiquidCore directly.

Note: The `Process` API is not exposed publicly in versions 0.5.x and below.  It will be in 0.6+.

License
-------

Copyright (c) 2014 - 2018 LiquidPlayer

Distributed under the MIT License.  See [LICENSE.md](LICENSE.md) for terms and conditions.

[Node.js]:https://nodejs.org/
[Android Studio]:https://developer.android.com/studio/index.html
[BigNumber]:https://github.com/MikeMcl/bignumber.js/
