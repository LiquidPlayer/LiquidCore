The LiquidCore Project
----------------------

LiquidCore provides an environment for developers to create native mobile micro apps in Javascript that can in turn be embedded into _other_ apps.  Think: native `<iframe>` for mobile apps.  A LiquidCore micro app is simply a [Node.js] module that can be served from the cloud, and therefore, like in a webpage, it can be modified server-side and instantly updated on all mobile devices.

LiquidCore also provides a convenient way for Android developers to [execute raw JavaScript](https://github.com/LiquidPlayer/LiquidCore/wiki/LiquidCore-as-a-Native-Javascript-Engine) inside of their apps, as iOS developers can already do natively with JavaScriptCore.

For a description of how to use LiquidCore, please see the appropriate README under [LiquidCoreAndroid](https://github.com/LiquidPlayer/LiquidCore/tree/master/LiquidCoreAndroid) or [LiquidCoreiOS](https://github.com/LiquidPlayer/LiquidCore/tree/master/LiquidCoreiOS).

Version
-------
[0.5.0](https://github.com/LiquidPlayer/LiquidCore/releases/tag/0.5.0) [![Release](https://jitpack.io/v/LiquidPlayer/LiquidCore.svg)](https://jitpack.io/#LiquidPlayer/LiquidCore)

### Android
Get it through [JitPack](https://jitpack.io/#LiquidPlayer/LiquidCore/0.5.0)

**Step 1.** Add it in your root `build.gradle` at the end of repositories:

	allprojects {
		repositories {
			...
			maven { url 'https://jitpack.io' }
		}
	}
**Step 2.** Add the dependency

	dependencies {
	        implementation 'com.github.LiquidPlayer:LiquidCore:0.5.0'
	}

### iOS
**Note:** Version 0.5.0 is the very first iOS release and required a significant amount of effort to simulate the
V8 API on top of JavaScriptCore.  As such, it is unstable and buggy.  Use it at your own risk and ideally if you are willing
to help me debug it.

The framework is distributed through [Carthage](https://github.com/Carthage/Carthage#adding-frameworks-to-an-application).

1. Install Carthage as described [here](https://github.com/Carthage/Carthage/blob/master/README.md#installing-carthage).
1. Create a [Cartfile](https://github.com/Carthage/Carthage/blob/master/Documentation/Artifacts.md#cartfile) that includes the `LiquidCore` framework: ```git "git@github.com:LiquidPlayer/LiquidCore.git" ~> 0.5.0```
1. Run `carthage update`. This will fetch dependencies into a [Carthage/Checkouts](https://github.com/Carthage/Carthage/blob/master/Documentation/Artifacts.md#carthagecheckouts) folder, then build each one or download a pre-compiled framework.
1. On your application targets’ _General_ settings tab, in the “Linked Frameworks and Libraries” section, drag and drop `LiquidCore.framework` from the [Carthage/Build](https://github.com/Carthage/Carthage/blob/master/Documentation/Artifacts.md#carthagebuild) folder on disk.
1. On your application targets’ _Build Phases_ settings tab, click the _+_ icon and choose _New Run Script Phase_. Create a Run Script in which you specify your shell (ex: `/bin/sh`), add the following contents to the script area below the shell:

    ```sh
    /usr/local/bin/carthage copy-frameworks
    ```

1. Add the paths to the framework under “Input Files":

    ```
    $(SRCROOT)/Carthage/Build/iOS/LiquidCore.framework
    ```

1. Add the paths to the copied framework to the “Output Files”:

    ```
    $(BUILT_PRODUCTS_DIR)/$(FRAMEWORKS_FOLDER_PATH)/LiquidCore.framework
    ```


API Documentation
-----------------
**Android Javadocs**: [Version 0.5.0](https://liquidplayer.github.io/LiquidCoreAndroid/0.5.0/index.html)

**iOS Objective-C/Swift**: [Version 0.5.0](https://liquidplayer.github.io/LiquidCoreiOS/0.5.0/index.html)

License
-------

 Copyright (c) 2014-2018 Eric Lange. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

[Node.js]:https://nodejs.org/
[Android Studio]:https://developer.android.com/studio/index.html
[BigNumber]:https://github.com/MikeMcl/bignumber.js/
