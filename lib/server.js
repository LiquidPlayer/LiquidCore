/*
 * Copyright © 2019-20 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */
const http = require('http')
const configure = require('./Config')
const Metro = require('metro')
const { mergeConfig } = require("metro-config")

const usage = (() => {
  /*
  Usage:
    npx liquidcore server [options]

  Examples:
    npx liquidcore server
    npx liquidcore server --port=8888
    npx liquidcore server --reset-cache

  Options:
    --project-root=<dir>    The root folder of your project
    --reset-cache           Whether we should reset the cache when starting the build
    --max-workers=<num>     The number of workers we should parallelize the transformer on
    --port=<num>            Which port to listen on

  For more options, you can create a 'metro.config.js' file to specify more advanced
  capabilities.  See https://facebook.github.io/metro/docs/en/configuration for more
  information.
  */
}).toString().split(/\n/).slice(2, -2).join('\n')

async function server(override) {
  var config = configure(await Metro.loadConfig())

  var args = {
    assetExts: [],
    platforms: ['ios','android'],
    port: 8082,
    resetCache: false,
    verbose: false,
  }

  config = mergeConfig(config, args, override)

  if (config.help) {
    console.log(usage)
    process.exit(0)
  }

  const metroBundlerServer = await Metro.runMetro(config)

  const httpServer = http.createServer((req, res) => {
    metroBundlerServer.processRequest(req, res, () => {
      res.statusCode = 500
      res.end('The server only responds to requests for .bundle files')
    })
  })

  httpServer.listen(config.port)
}

module.exports = server