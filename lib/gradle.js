/*
 * Copyright © 2019 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */

const fs = require('fs')
const path = require('path')
const util = require('./util')

const usage = (() => {
/*
  Usage:
    npm run gradle-config [-- <options>]

  Examples:
    npm run gradle-config
    npm run gradle-config -- --dev --module=serverApp

  Options:
    --dev             Configures for building addons locally out of node_modules
    --module=<module> The name of the Android module to configure (default: app)
*/
}).toString().split(/\n/).slice(2, -2).join('\n')

const BUILD_GRADLE=path.resolve('.', '.liquidcore', 'liquidcore.build.gradle')
const SETTINGS_GRADLE=path.resolve('.', '.liquidcore', 'liquidcore.settings.gradle')
const BUILD_APPLY="\napply from: new File(rootProject.projectDir, '.liquidcore/liquidcore.build.gradle')"
const SETTINGS_APPLY="\napply from: '.liquidcore/liquidcore.settings.gradle'"

const tasks = (() => {
/*
task bundleLiquidCoreCode(type: Exec) {
  workingDir rootProject.projectDir
  commandLine 'node', 'node_modules/liquidcore/lib/cli.js', 'bundle', '--platform=android'
}
project.afterEvaluate {
    preBuild.dependsOn bundleLiquidCoreCode
}
*/
}).toString().split(/\n/).slice(2, -2).join('\n')

const gradle = async (override) => {
  let args = { module : 'app'}
  Object.assign(args, override)
  let app = args.module

  if (!fs.existsSync(path.resolve('.', app, 'build.gradle'))) {
    console.error("Module '" + app + '" not found."')
  }
  if (!fs.existsSync(path.resolve('.', 'settings.gradle'))) {
    console.error("./settings.gradle not found.")
  }

  if (!fs.existsSync(path.resolve('.', '.liquidcore'))) {
    fs.mkdirSync(path.resolve('.', '.liquidcore'))
  }

  if (args.help) {
    console.log(usage)
    process.exit(0)
  }

  let build_gradle =
    tasks + "\n" +
    "dependencies {\n"
  let settings_gradle = ''
  let includes = ''

  let modules = []
  util.recurse_packages('.', 'node_modules', (package, resolved) => {
    let addon = package['liquidcore']
    let android = addon && addon.android
    if (android) {
      if (!Array.isArray(android)) android = [android]
      android.forEach(android=>{
        let module = {
          name: android.name ? android.name : package.name,
          module: path.basename(resolved) + '-' + (android.name ? android.name : package.name),
          version: package.version,
          maven: android.maven,
          forceDev: !!android.forceDev
        }
        let aar = android.aar && path.resolve(resolved, android.aar + '.aar')
        let pth = android.dev && path.resolve(resolved, android.dev)
        module.inc = android.include && path.relative(path.resolve('.'),
          path.resolve(resolved, android.include))
        // on Windows `path.resolve` returns a path with non-escaped backslashes which does not
        // represent a valid string in Gradle; so, make the path processable by all OSes using
        // forward slashes instead
        module.inc = module.inc.split("\\").join("/")
        if (aar) {
          try {
            fs.realpathSync(aar)
            module.aar = path.relative(path.resolve('.'), aar)
          } catch (e) {
            console.warn("WARN: '%s' does not exist, skipping", aar)
          }
        }
        if (pth) {
          try {
            fs.realpathSync(pth)
            module.dev = path.relative(path.resolve('.'), pth)
          } catch (e) {
            console.warn("WARN: Project directory '%s' does not exist, skipping", pth)
          }
        }
        if (module.aar || module.dev || module.maven || module.inc) {
          modules.push(module)
        }
      })
    }
  })

  // Clean up and de-dupe modules
  // Precedence rules:
  // 1. dev > release
  // 2. relative path closest to root wins
  clean_modules = {}
  modules.forEach(p => {
    let current = clean_modules[p.module]
    if (current !== undefined) {
      if (!p.dev && current.dev) return // Rule 1
      ;if (current.dev && current.dev.length < p.dev.length) return // Rule 2
      ;if (current.aar && !p.aar && (!p.aar || current.aar.length < p.aar.length)) return
    }
    clean_modules[p.module] = p
  })

  Object.keys(clean_modules).forEach(k => {
    let aar = clean_modules[k].aar
    let dev = clean_modules[k].dev
    let forceDev = clean_modules[k].forceDev
    let mvn = clean_modules[k].maven
    let inc = clean_modules[k].inc
    let version = clean_modules[k].version
    let m = clean_modules[k].module
    if (aar || dev || mvn || inc) {
      if (inc) {
        let newinc = "apply from: new File(rootProject.projectDir, " +
        "'"+inc+"')\n"
        if (!includes.includes(newinc))
          includes += newinc
      } else {
        build_gradle +=
        "    if (findProject(':"+m+"') != null) {\n" +
        "        implementation project(':"+m+"')\n"
        "    }\n\n"
      }
      if (mvn) {
        build_gradle +=
          "    } else {\n" +
          "        implementation '"+mvn+":"+version+"'\n"
          "    }\n\n"
      } else if (aar) {
        build_gradle +=
          "    } else {\n" +
          "        implementation fileTree(include:['"+path.basename(android)+"'], " +
          "dir:new File(rootProject.projectDir, '"+path.dirname(android)+"'))\n"
          "    }\n\n"
      }
      if ((!aar && !mvn && !inc) || (inc && forceDev) || (args.dev && dev)) {
        settings_gradle +=
        "include ':"+m+"'\n" +
        "project(':"+m+"').projectDir = new File(\n" +
        "        rootProject.projectDir, '"+dev+"')\n\n"
      }
    }
  })

  build_gradle += '}\n'
  build_gradle += includes

  if (!fs.existsSync(BUILD_GRADLE) || String(fs.readFileSync(BUILD_GRADLE)) != build_gradle) {
    fs.writeFileSync(BUILD_GRADLE, build_gradle)
    console.log('Updated ' + BUILD_GRADLE)
  }

  let build = fs.readFileSync(path.resolve('.', app, "build.gradle"))
  if (!build.includes(BUILD_APPLY)) {
    build += BUILD_APPLY
    fs.writeFileSync(path.resolve('.', app, "build.gradle"), build)
    console.log('Updated ' + path.resolve('.', app, "build.gradle"))
  }

  let settings = String(fs.readFileSync(path.resolve('.', "settings.gradle")))
  if (settings_gradle != '') {
    if (!fs.existsSync(SETTINGS_GRADLE) || String(fs.readFileSync(SETTINGS_GRADLE)) != settings_gradle) {
      fs.writeFileSync(SETTINGS_GRADLE, settings_gradle)
      console.log('Updated ' + SETTINGS_GRADLE)
    }
    if (!settings.includes(SETTINGS_APPLY)) {
      settings += SETTINGS_APPLY
      fs.writeFileSync(path.resolve('.', "settings.gradle"), settings)
      console.log('Updated ' + path.resolve('.', "settings.gradle"))
    }
  } else {
    try { fs.unlinkSync(SETTINGS_GRADLE) } catch(e) {}
    if (settings.includes(SETTINGS_APPLY)) {
      settings = settings.replace(SETTINGS_APPLY,'')
      fs.writeFileSync(path.resolve('.', "settings.gradle"), settings)
      console.log('Updated ' + path.resolve('.', "settings.gradle"))
    }
  }

  console.log("Gradle config complete")

  delete args._
  delete args['--']
  delete args['']
  let update = !args.__noupdate
  delete args.__noupdate
  if (update) {
    let pkgjson = JSON.parse(fs.readFileSync(path.resolve('.','package.json')))
    pkgjson.liquidcore = pkgjson.liquidcore || {}
    pkgjson.liquidcore.gradle_options = args
    pkgjson.liquidcore.bundler_output = pkgjson.liquidcore.bundler_output || {}
    pkgjson.liquidcore.bundler_output.android = args.module + "/src/main/res/raw"
    fs.writeFileSync(path.resolve('.', 'package.json'), JSON.stringify(pkgjson,null,2))
    console.log('Saved gradle_options in package.json')
  }
}

module.exports = gradle
