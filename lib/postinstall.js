/*
 * Copyright © 2020 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */
const fs = require('fs')
const path = require('path')
const gradle = require('./gradle')
const pod = require('./pod')

const usage = (() => {
/*
  Description:
    Run automatically after an 'npm install', this script ensures
    that any new native addons are added to the configuration files.

  Usage:
    npx liquidcore postinstall [<options>]

  Options:
    --help   Print this message
*/
}).toString().split(/\n/).slice(2, -2).join('\n')

async function postinstall(override) {
  var args = {}
  Object.assign(args, override)

  if (args.help) {
    console.log(usage)
    process.exit(0)
  }

  let inp = fs.readFileSync(path.resolve('.','package.json'))
  let data = JSON.parse(inp)
  if (data && data.liquidcore) {
    let android = data.liquidcore.gradle_options
    if (android && typeof android === 'object') {
      android.__noupdate = true
      await gradle(android)
    }
    let ios = data.liquidcore.pod_options
    if (ios && typeof ios === 'object') {
      ios.__noupdate = true
      ios.install = true
      await pod(ios)
    }
  }
}

module.exports = postinstall
