// This will keep the process alive until an explicit exit() call
setInterval(function(){},1000)

// This will attempt to attach to a ConsoleSurface
LiquidCore.attach('org.liquidplayer.surfaces.console.ConsoleSurface', (error) => {
    console.log("Hello, World!")
    if (error) {
        console.error(error)
    }
})
