setInterval(function(){}, 5000)

var promise = new Promise(function(resolve, reject) {
    setTimeout(resolve, 500, 123)
})

promise.then(function(result) {
    console.log('emitting ' + result)
    LiquidCore.emit( 'promise', result )
})