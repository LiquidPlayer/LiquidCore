# NodeConsole
A Node.js console app for LiquidCore iOS

This simple app exposes an [`LCConsoleSurfaceView`](https://github.com/LiquidPlayer/LiquidCore/tree/master/LiquidCoreiOS/LiquidCore/ConsoleView) which provides an interactive
ANSI console to Node.js.  Note that it does not contain any code outside of
what is generated for an empty app from XCode.  All of the magic happens in the Interface Builder storyboard.

The `main.storyboard` has a single `UIView` placed on the view controller.  Its custom class (in the Identity inspector) is
set to `LCLiquidView` and the "Js Resource" field is set to `console` in the "Liquid View" section of the Attributes inspector.

That resource file, `console.js`, contains the following:

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

The `LCLiquidView` starts a LiquidCore micro service and executes the code in this file.  The
javascript code simply requests to be attached to `ConsoleSurface` UI, which is an ANSI
console view included in LiquidCore.  It prints a welcome message and then acts as a
(mostly) full-featured node console.

License
-------

Copyright (c) 2014 - 2018 LiquidPlayer

Distributed under the MIT License.  See [LICENSE.md](https://github.com/LiquidPlayer/LiquidCore/blob/master/LICENSE.md) for terms and conditions.
