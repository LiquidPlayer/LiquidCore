/*
 * Copyright © 2018 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */
'use strict'

const minimist = require('minimist')

const modules =
[
    'server',
    'bundle',
    'init',
    'gradle',
    'pod',
    'postinstall'
]

var args = minimist(process.argv.slice(2), {'--': true});

const usage = (() => {
/*
Usage:
  npm run <command> [params] [-- <options>]

Examples:
  npm run init
  npm run gradle-config -- --dev
  npm run pod-config MyiOSProject

Commands:
  init                 Initialize a project for use with LiquidCore
  gradle-config        Generate gradle configuration for Android project
  pod-config           Generates a cocoapods configuration for iOS project
  server               Starts a development server
  bundler              Bundles javascript files into single file

For more information on each command, specify the --help option, e.g.
npm run pod-config -- --help
*/
}).toString().split(/\n/).slice(2, -2).join('\n')

let command = args._[0]
args._.shift()
if (!command || !modules.includes(command)) {
    usage()
    process.exit(-2)
}

for (var arg in args) {
    var newarg = ''
    var capNext = false
    for (var i=0; i<arg.length; i++) {
        if (arg[i] == '-') {
            capNext = true
        } else {
            newarg += capNext ? String(arg[i]).toUpperCase() : arg[i]
            capNext = false
        }
    }
    if (arg != newarg) args[newarg] = args[arg]
    if (args[arg] === 'true') args[arg] = true
    if (args[arg] === 'false') args[arg] = false
}

const mod = require('./' + command)
mod(args)