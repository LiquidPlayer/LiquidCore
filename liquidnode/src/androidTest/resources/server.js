var http = require('http')
var url = require('url')
var fs = require('fs')
var path = require('path')
var baseDirectory = '/home/local'

// Create a file to serve

/* The comments will be converted to a string */
var source = function () {
  /*
    setInterval(function(){},1000)

    LiquidCore.on('js_msg', (msg) => {
        if ('Hallo die Weld!' === msg.msg) {
            LiquidCore.emit('null')
        } else {
            LiquidCore.emit('null', 'fail')
        }
    })

    LiquidCore.on('js_null', (msg) => {
        if (msg === undefined) {
            LiquidCore.emit('number', 5.2)
        } else {
            LiquidCore.emit('number', 0)
        }
    })

    LiquidCore.on('js_number', (msg) => {
        if (msg === 2.5) {
            LiquidCore.emit('integer', 24)
        } else {
            LiquidCore.emit('integer', 'fail')
        }
    })

    LiquidCore.on('js_integer', (msg) => {
        if (msg === 42) {
            LiquidCore.emit('float', 16.3)
        } else {
            LiquidCore.emit('float', 'fail')
        }
    })

    LiquidCore.on('js_float', (msg) => {
        if (msg === 36.1) {
            LiquidCore.emit('long', 123456789)
        } else {
            LiquidCore.emit('long', 'fail')
        }
    })


    LiquidCore.on('js_long', (msg) => {
        if (msg === 987654321) {
            LiquidCore.emit('string','foo')
        } else {
            LiquidCore.emit('string', 'fail')
        }
    })

    LiquidCore.on('js_string', (msg) => {
        if (msg === 'bar') {
            LiquidCore.emit('boolean', true)
        } else {
            LiquidCore.emit('boolean', false)
        }
    })

    LiquidCore.on('js_boolean', (msg) => {
        if (msg === false) {
            LiquidCore.emit('array',[1,'two',true,{str:'bar'}])
        } else {
            LiquidCore.emit('array',[0])
        }
    })

    LiquidCore.on('js_array', (msg) => {
        if (msg[0] === 5) {
            process.exit(0)
        } else {
            process.exit(1)
        }
    })

    LiquidCore.emit('msg',{msg: 'Hello, World!'})
  */
  }.toString().split(/\n/).slice(2, -2).join('\n')

try {
    fs.writeFileSync('/home/local/hello.js', source, 'utf-8')
} catch (e) {
    console.error(e)
}

var server = http.createServer(function (request, response) {
   try {
     var requestUrl = url.parse(request.url)

     var fsPath = baseDirectory+path.normalize(requestUrl.pathname)

     response.writeHead(200)
     var fileStream = fs.createReadStream(fsPath)
     fileStream.pipe(response)
     fileStream.on('error',function(e) {
         console.error(e)
         response.writeHead(404)
         response.end()
     })
   } catch(e) {
     response.writeHead(500)
     response.end()
     console.log(e.stack)
   }
})

server.on('listening',function() {
    console.log("listening on port "+server.address().port)
    LiquidCore.emit('listening', {port: server.address().port})
})
server.listen()