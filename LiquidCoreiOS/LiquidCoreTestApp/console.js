const CONSOLE = 'org.liquidplayer.surface.console.ConsoleSurface';

/* Request a console surface to be attached to our Node.js process.  Once this is
 * done, process.stdout and process.stderr will be streamed to the LCLiquidView.
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
