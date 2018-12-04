/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
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
