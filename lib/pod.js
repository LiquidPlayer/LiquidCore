/*
 * Copyright © 2019 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */

const fs = require('fs')
const path = require('path')
const util = require('./util')
const {exec} = require('child_process')

const usage = (() => {
/*
  Usage:
    npm run pod-config <target> [<options>]

  Where:
    <target>          The target XCode project

  Examples:
    npx liquidcore pod-config myxcodeproj
    npx liquidcore pod-config myxcodeproj --dev

  Options:
    --dev             Configures for building addons locally out of node_modules
    --podfile <path>  Path to local Podfile (default '.')
    --target <target> The target XCode project (mutually exclusive with <target> parameter)
    --install         Run 'pod install' if any changes were made
*/
}).toString().split(/\n/).slice(2, -2).join('\n')

const LIQUIDCORE_RB=path.resolve('.', '.liquidcore', 'liquidcore.rb')
const LIQUIDCORE_PODS='liquidcore_pods'
const LIQUIDCORE_BUNDLE_PODSPEC=path.resolve('.', '.liquidcore', 'liquidcore_bundle.podspec')
const BUNDLE_PODSPEC=(()=>{
/*
Pod::Spec.new do |s|
  s.name = 'liquidcore_bundle'
  s.version = '1.0.0'
  s.summary = 'Bundled JS files for LiquidCore'
  s.description = 'A pod containing the files generated from the JS bundler'
  s.author = ''
  s.homepage = 'http'
  s.source = { :git => "" }
  s.resource_bundles = {
   'LiquidCore' => [
      'ios_bundle/*.js'
   ]
 }
 s.script_phase = { :name => 'Bundle JavaScript Files', :script => '(cd ..; node node_modules/liquidcore/lib/cli.js bundle --platform=ios)' }
end
*/
}).toString().split(/\n/).slice(2, -2).join('\n')

const pod = async (override) => {
  var args = {
  }
  Object.assign(args, override)

  if (!fs.existsSync(path.resolve('.', '.liquidcore'))) {
    fs.mkdirSync(path.resolve('.', '.liquidcore'))
  }

  if (args.help) {
    console.log(usage)
    process.exit(0)
  }

  if (args.target === undefined && args._.length < 1) {
    console.error('Target must be specified.')
    process.exit(-1)
  }

  let target = args.target || args._[0]
  args.target = target

  let podfile = ""
  let specs = ['https://github.com/CocoaPods/Specs.git']
  let pods = []

  util.recurse_packages('.', 'node_modules', (package, resolved) => {
    let addon = package['liquidcore']
    let ios = addon && addon.ios
    let ios_dev = addon && addon['ios-dev']
    if (ios || ios_dev) {
      let i = (!ios || (ios_dev && args.dev)) ? ios_dev : ios
      if (!Array.isArray(i)) i = [i]
      i.forEach((dev) => {
        if (dev.version === undefined &&
          dev.path === undefined &&
          dev.podspec === undefined)
          dev.version = package.version
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

  podfile += "def " + LIQUIDCORE_PODS + "\n"

  // Include 'source' lines
  Object.keys(clean_pods).forEach(k =>
    clean_pods[k].specs &&
    !specs.includes(clean_pods[k].specs) &&
    specs.unshift(clean_pods[k].specs))
  if (specs.length > 1) {
    podfile += "  source '"
    podfile += specs.join("'\nsource '")
    podfile += "'\n\n"
  }

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

  podfile += "  pod 'liquidcore_bundle', :path => '.liquidcore/liquidcore_bundle.podspec'\n"
  podfile += "end\n"

  let has_changed = false
  let liquidcore_rb_old = ''
  try {
    liquidcore_rb_old = String(fs.readFileSync(LIQUIDCORE_RB))
  } catch (e) {}

  if (liquidcore_rb_old != podfile) {
    has_changed = true
    fs.writeFileSync(LIQUIDCORE_RB, podfile)
    console.log('Updated ' + LIQUIDCORE_RB)
  }

  let Podfile_path = path.resolve(args.podfile || '.')
  if (path.basename(Podfile_path) != 'Podfile') {
    Podfile_path = path.resolve(Podfile_path, 'Podfile')
  }
  let relative_path = path.relative(path.dirname(Podfile_path),LIQUIDCORE_RB)
  let LOAD="load '" + relative_path + "'"

  let Podfile
  try {
    Podfile = String(fs.readFileSync(Podfile_path))
  } catch (e) {
    console.error('Cannot find Podfile at ' + Podfile_path)
    process.exit(-5)
  }

  let changed = String(Podfile)
  if (!changed.includes(LOAD)) {
    changed = LOAD + "\n" + Podfile
  }
  if (!changed.includes(LIQUIDCORE_PODS)) {
    const rx = new RegExp('target[ \\t]*[\\\'\\\"]' + target + '[ \\t]*[\\\'\\\"].+\\n')
    let loc = rx.exec(changed)
    if (!loc) {
      console.error('Cannot find target ' + target + ' in ' + Podfile_path)
      process.exit(-4)
    }
    changed = changed.substr(0,loc.index) + loc[0] + '  ' + LIQUIDCORE_PODS +
      '\n' + changed.substr(loc.index + loc[0].length)
  }
  if (Podfile != changed) {
    has_changed = true
    fs.writeFileSync(Podfile_path, changed)
    console.log('Updated ' + Podfile_path)
  }

  console.log("Pod config complete")

  let old = ''
  if (fs.existsSync(LIQUIDCORE_BUNDLE_PODSPEC)) {
    old = fs.readFileSync(LIQUIDCORE_BUNDLE_PODSPEC);
  }
  if (old != BUNDLE_PODSPEC) {
    has_changed = true
    fs.writeFileSync(LIQUIDCORE_BUNDLE_PODSPEC,BUNDLE_PODSPEC)
    console.log('Created ' + LIQUIDCORE_BUNDLE_PODSPEC)
  }

  delete args._
  delete args['--']
  delete args['']
  let update = !args.__noupdate
  let install = !!args.install
  delete args.__noupdate
  delete args.install
  if (update) {
    let pkgjson = JSON.parse(fs.readFileSync(path.resolve('.','package.json')))
    pkgjson.liquidcore = pkgjson.liquidcore || {}
    pkgjson.liquidcore.pod_options = args
    pkgjson.liquidcore.bundler_output = pkgjson.liquidcore.bundler_output || {}
    pkgjson.liquidcore.bundler_output.ios = ".liquidcore/ios_bundle"
    fs.writeFileSync(path.resolve('.', 'package.json'), JSON.stringify(pkgjson,null,2))
    console.log('Saved pod_options in package.json')
  }

  if (has_changed && install) {
    console.log("Changes were made, re-running 'pod install'")
    const done_handler = (err, stdout, stderr) => {
      console.log(stdout)
      console.error(stderr)
      if (!err) {
        console.log('Pods successfully installed')
      } else {
        console.error('Pods failed to install')
        process.exit(-6)
      }
    }
    exec('pod install', done_handler)
  }
}

module.exports = pod
