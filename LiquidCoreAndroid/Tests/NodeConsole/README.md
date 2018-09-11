# NodeConsole
A Node.js console app for LiquidCore Android

This simple app exposes a [`ConsoleSurface`](https://github.com/LiquidPlayer/LiquidCore/tree/master/LiquidCoreAndroid/src/main/java/org/liquidplayer/surface/console) which provides an interactive
ANSI console to Node.js.  Note that it does not contain any Kotlin/Java code outside of
what is generated for a single-view app from Android Studio.  All of the magic happens
in the layout file, `activity_main.xml`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<android.support.constraint.ConstraintLayout xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:app="http://schemas.android.com/apk/res-auto"
    xmlns:tools="http://schemas.android.com/tools"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    tools:context=".MainActivity">

    <org.liquidplayer.service.LiquidView
        android:layout_width="match_parent"
        android:layout_height="match_parent"
        android:id="@+id/liquidview"
        app:liquidcore.URI="android.resource://org.liquidplayer.NodeConsole/raw/console"
        />

```

It exposes a `LiquidView` which points to a local resource file using the `app:liquidcore.URI`
attribute.  That file, `console.js` contains the following:

```javascript
const CONSOLE = 'org.liquidplayer.surface.console.ConsoleSurface';

/* Request a console surface to be attached to our Node.js process.  Once this is
 * done, process.stdout and process.stderr will be streamed to the LiquidView.
 */
LiquidCore.attach(CONSOLE, main);

/* The 'main' function will be called once the console surface is attached
 * to the process.
 */
function main(error)
{
    if (error) {
        /* ruh roh, something went wrong */
        LiquidCore.detach();
        return;
    }

    console.error("Welcome to LiquidCore!");
    console.log("Enter javascript code to run it in the console.")
    console.log("");
}
```

The `LiquidView` starts a LiquidCore micro service and executes the code in this file.  The
javascript code simply requests to be attached to `ConsoleSurface` UI, which is an ANSI
console view included in LiquidCore.  It prints a welcome message and then acts as a
(mostly) full-featured node console.

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

