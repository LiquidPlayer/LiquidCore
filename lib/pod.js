/*
 * Copyright © 2019 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */

const fs = require('fs')
const path = require('path')
const util = require('./util')

const default_ios_target_version = '11.0'
const Specs = 'https://github.com/LiquidPlayer/Specs.git'

;(() => {
  const pod = async (override) => {
    var args = {
      iosVersion : default_ios_target_version,
      version : await util.get_latest_version(),
      specs : Specs
    }
    Object.assign(args, override)

    if (args.help) {
      console.log('')
      console.log('Generates a Podfile which contains the cocoapods required to configure')
      console.log('LiquidCore and any required addons.  The output will go to stdout.  If')
      console.log('you already have a Podfile, copy the source and pod inclusion lines into')
      console.log('it.  Otherwise, you can just redirect output to Podfile (e.g. ')
      console.log('`liquidcore pod MyTarget > Podfile`).  Be sure to run `pod install` afterwards.')
      console.log('')
      console.log('Usage:')
      console.log('liquidcore pod <target> [<options>]')
      console.log('')
      console.log('Where:')
      console.log('  <target>                  Build target project')
      console.log('  <options>                 One or more of the following:')
      console.log('    --dev                   Configures for building addons locally out of node_modules')
      console.log('    --project=<project>     Specifies a project file (ex. mydir/myproject.xcodeproj) if in different directory')
      console.log('    --version=<version>     Use specific version of LiquidCore (default: ' + liquidcore_version +')')
      console.log('    --liquidcore=<path>     Use local build of LiquidCore (mutually exclusive with --version)')
      console.log('    --ios-version=<version> Target minimum iOS version (default: ' + default_ios_target_version +')')
      console.log('    --specs=<url>           Link to specs repo for LiquidCore (default: ' + Specs + ')')
      console.log('')
      console.log('If you need to install liquidcore addons, specify them in a local package.json file')
      console.log('and run `npm install` before running this command.')
      process.exit(0)
      console.log('')
    }

    if (args._.length < 1) {
      console.error('Target must be specified.')
      process.exit(-1)
    }

    if (args.liquidcore && args.liquidcore[0] === '~') {
      args.liquidcore = path.join(process.env.HOME, args.liquidcore.slice(1));
    }

    if (args.liquidcore && fs.existsSync(path.resolve(args.liquidcore))) {
      if (fs.statSync(path.resolve(args.liquidcore)).isDirectory())
        args.liquidcore = args.liquidcore + '/LiquidCore.podspec'
    }
    if (args.liquidcore && !fs.existsSync(path.resolve(args.liquidcore))) {
      console.error('LiquidCore project cannot be found at ' + args.liquidcore);
      process.exit(-2)
    }

    let podfile =
      "platform :ios, '"+args.iosVersion+"'\n" +
      "use_frameworks!\n\n"
    let specs = ['https://github.com/CocoaPods/Specs.git']
    let pods = []

    if (args.liquidcore) {
      pods.push({name : "LiquidCore", path : args.liquidcore })
    } else {
      pods.push({name : "LiquidCore", version: args.version, specs: args.specs})
    }

    util.recurse_packages('.', 'node_modules', (package, resolved) => {
      let addon = package['liquidcore-addon']
      let ios = addon && addon.ios
      let ios_dev = addon && addon['ios-dev']
      if (ios || ios_dev) {
        let i = (!ios || (ios_dev && args.dev)) ? ios_dev : ios
        if (i.version === undefined && i.path === undefined && i.podspec === undefined) i.version = package.version
        if (!Array.isArray(i)) i = [i]
        i.forEach((dev) => {
          if (dev.version) {
            return pods.push(dev)
          }
          let prop = (dev.path && 'path') || 'podspec'
          let ipath = path.resolve(resolved, dev[prop])
          try {
            // Cocoapods cannot handle symlinks and most annoyingly just kind of silently
            // fails, so unwind symlinks
            dev[prop] = path.relative(path.resolve('.'), fs.realpathSync(ipath))
            pods.push(dev)
          } catch (e) {
            console.warn("WARN: '%s' does not exist, skipping", ipath)
          }
        })
      }
    })

    // Clean up and de-dupe pods
    // Precedence rules:
    // 1. Dev pods > Release pods
    // 2. Dev pods: relative path closest to root wins
    // 3. Release pods: Highest version wins (FIXME: Need to deal with compatibility rules)
    clean_pods = {}
    pods.forEach(p => {
      let current = clean_pods[p.name]
      if (current !== undefined) {
        if (current.path && !p.path) return // Rule 1
        ;if (current.path && current.path.length < p.path.length) return // Rule 2
        ;if (current.version && p.version && util.version(current.version) < util.version(p.version)) return // Rule 3
        ;if (!p.version) return
      }
      clean_pods[p.name] = p
    })

    // Include 'source' lines
    Object.keys(clean_pods).forEach(k =>
      clean_pods[k].specs &&
      !specs.includes(clean_pods[k].specs) &&
      specs.unshift(clean_pods[k].specs))
    if (specs.length > 1) {
      podfile += "source '"
      podfile += specs.join("'\nsource '")
      podfile += "'\n\n"
    }

    if (args.project) {
      podfile += "project '"+args.project+"'\n"
    }
    podfile += "target '"+args._[0]+"' do\n"

    // Include 'pod' lines
    Object.keys(clean_pods).forEach(k => {
      let p = clean_pods[k]
      podfile += "  pod '" + p.name + "'"
      if (p.path) {
        podfile += ", :path => '" + p.path + "'"
      } else if (p.podspec) {
        podfile += ", :podspec => '" + p.podspec + "'"
      } else if (p.version) {
        podfile += ", '" + p.version + "'"
      }
      if (p.subspecs) {
        podfile += ", :subspecs => ["
        p.subspecs.forEach((subspec) => podfile += "'" + subspec + "',")
        podfile += "]"
      }
      podfile += '\n'
    })

    podfile += "end\n"

    console.log(podfile)
  }

  module.exports = pod
})()
