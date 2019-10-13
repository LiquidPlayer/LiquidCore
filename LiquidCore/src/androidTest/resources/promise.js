setInterval(function(){}, 5000)

var promise = new Promise(function(resolve, reject) {
    resolve(123)
})

promise.then(function(result) {
    console.log('emitting ' + result)
    LiquidCore.emit( 'promise', result )
})