/*
 * Copyright © 2018 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */
'use strict'

const path = require('path')
const fs = require('fs')
const resolve = require('metro-resolver').resolve
const here = fs.realpathSync.native(__dirname)
const blacklist = require('metro-config/src/defaults/blacklist')
const local_path = path.resolve('.') + '/node_modules/';

// There is no good way to save the system `require()` before metro overwrites
// it.  Plus metro does not handle cyclic dependencies well at all.  We fix these by
// using our own implementation of metroRequire.
try {
  fs.writeFileSync(path.resolve(__dirname,'../node_modules/metro-config/src/defaults/defaults.js'),
    fs.readFileSync(path.resolve(__dirname, 'metro-polyfill/defaults.js'))
  )
} catch (e) {
  fs.writeFileSync(path.resolve(local_path,'metro-config/src/defaults/defaults.js'),
    fs.readFileSync(path.resolve(__dirname, 'metro-polyfill/defaults.js'))
  )
}

// Patch for: https://github.com/facebook/metro/issues/330
// And disable throwOnModuleCollision -- this causes all kinds of problems and solves none.
try {
  fs.writeFileSync(path.resolve(__dirname,'../node_modules/metro/src/node-haste/DependencyGraph.js'),
    fs.readFileSync(path.resolve(__dirname, 'metro-polyfill/DependencyGraph.js'))
  )
} catch (e) {
  fs.writeFileSync(path.resolve(local_path,'metro/src/node-haste/DependencyGraph.js'),
    fs.readFileSync(path.resolve(__dirname, 'metro-polyfill/DependencyGraph.js'))
  )
}

const configure = (config) => {
  // Do not add all the react-native junk by default.  This needs to be done
  // explicitly by clients as the React Native environment won't be set up
  // until after the surface is bound; not at initial execution
  config.serializer = config.serializer || {}
  config.serializer.getModulesRunBeforeMainModule = () => [];

  // Node built-in modules need to bypass `metroRequire` and use the good
  // old fashioned node `require`
  config.resolver = config.resolver || {}
  let extraNodeModules = config.resolver.extraNodeModules || {}
  let watchFolders = config.watchFolders || []
  const nativenode = fs.readdirSync(path.resolve(here, 'node-native')).map((t) => t.split('.')[0])
  nativenode.forEach(dep => {
    extraNodeModules[dep] = path.resolve(path.resolve(here), "node-native", dep)
  })
  config.resolver.extraNodeModules = extraNodeModules
  config.watchFolders = watchFolders.concat([path.resolve(here, "node-native")])

  // Metro assumes that we are bundling for the browser, but in our case, we are not.
  config.resolver.resolverMainFields = (config.resolver.resolverMainFields || []).filter(e => e != 'browser')

  // Ignore evertyhing in the deps/ directory.  Some of the package.json files in the tests
  // cause problems
  config.resolver.blacklistRE = blacklist([/LiquidCore\/deps\/.*/])

  // If there is a missing module, don't throw an exception during the build process.  Some
  // modules are written to test for the presence of a module and handle the error case.
  // Instead, we cause an exception to be thrown at runtime and warn about it now.
  config.resolver.resolveRequest = (context, moduleName, platform) => {
    try {
      context.resolveRequest = null;
      return resolve(context, moduleName, platform);
    } catch (e) {
      console.warn("WARN: Cannot resolve module '" + moduleName + "'.  This will throw an Error at runtime.")
      return {
        type: "sourceFile",
        filePath: path.resolve(__dirname,"node-native","__unresolved.js")
      }
    }
  }

  return config
}

module.exports = configure
