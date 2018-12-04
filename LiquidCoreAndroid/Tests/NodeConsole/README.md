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

/* Request a console surface to be bound and attached to our Node.js process.  Once this is
 * done, process.stdout and process.stderr will be streamed to the LiquidView.
 */
LiquidCore.bind(CONSOLE)
    .then((console)=>console.attach())
    .then(main)
    .catch((error) => {
        /* ruh roh, something went wrong */
        console.error(error);
    });

/* The 'main' function will be called once the console surface is attached
 * to the process.
 */
function main()
{
    console.error("Welcome to LiquidCore!");
    console.log("Enter javascript code to run it in the console.")
    console.log("");

    // Run this session indefinitely -- otherwise node will quit when it has nothing left to do
    setInterval(()=>{}, 1000);

    // Returning a resolved promise here just to preserve the promise chaining flow above
    return Promise.resolve();
}
```

The `LiquidView` starts a LiquidCore micro service and executes the code in this file.  The
javascript code simply requests to be attached to `ConsoleSurface` UI, which is an ANSI
console view included in LiquidCore.  It prints a welcome message and then acts as a
(mostly) full-featured node console.

License
-------

Copyright (c) 2014 - 2018 LiquidPlayer

Distributed under the MIT License.  See [LICENSE.md](https://github.com/LiquidPlayer/LiquidCore/blob/master/LICENSE.md) for terms and conditions.
