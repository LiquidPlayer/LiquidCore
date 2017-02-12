#!/usr/bin/env node
var LiquidServer = require('./index.js')
var server = LiquidServer.createServer()

var port = parseInt(process.argv[process.argv.length-1])
port = isNaN(port) ? undefined : port 

server.on('listening',function() {
    console.log("Listening on port "+server.address().port)
})
server.listen(port)