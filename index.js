const events = require('events')
const fs = require('fs')
const path = require('path')
const join = path.join

let lc = global && global.LiquidCore

/* If we are not running on LiquidCore, fake the event emitter for node-based dev
 */
if (!lc) {
  class LiquidCore extends events {}
  lc = new LiquidCore();

  /* If we are running outside of LiquidCore (i.e. testing in node on desktop), override the
   * `require()` function so that we can search ./node_modules/ for native `.node` addons.
   * Emit a warning so that developers know that they need to provide a LiquidCore-specific
   * addon to work inside the LiquidCore environment or replace with a js-only implementation.
   *
   * Algorithm borrowed from the `bindings` project: https://github.com/TooTallNate/node-bindings
   * Copyright (c) 2012 Nathan Rajlich <nathan@tootallnate.net>
   */
  const native_require = global.require
  const defaults = {
      arrow: process.env.NODE_BINDINGS_ARROW || ' → '
      , compiled: process.env.NODE_BINDINGS_COMPILED_DIR || 'compiled'
      , platform: process.platform
      , arch: process.arch
      , version: process.versions.node
      , bindings: 'bindings.node'
      , bindingsjs: 'bindings.node.js'
      , try: [
        // node-gyp's linked version in the "build" dir
        [ 'module_root', 'build', 'bindings' ]
        // node-waf and gyp_addon (a.k.a node-gyp)
        , [ 'module_root', 'build', 'Debug', 'bindings' ]
        , [ 'module_root', 'build', 'Release', 'bindings' ]
        // Debug files, for development (legacy behavior, remove for node v0.9)
        , [ 'module_root', 'out', 'Debug', 'bindings' ]
        , [ 'module_root', 'Debug', 'bindings' ]
        // Release files, but manually compiled (legacy behavior, remove for node v0.9)
        , [ 'module_root', 'out', 'Release', 'bindings' ]
        , [ 'module_root', 'Release', 'bindings' ]
        // Legacy from node-waf, node <= 0.4.x
        , [ 'module_root', 'build', 'default', 'bindings' ]
        // Production "Release" buildtype binary (meh...)
        , [ 'module_root', 'compiled', 'version', 'platform', 'arch', 'bindings' ]
        // LiquidCore mock implementation
        , [ 'module_root', 'mocks', 'bindingsjs']
        ]
      }

  function bindings (opts) {
    // Argument surgery
    if (typeof opts == 'string') {
      opts = { bindings: opts }
    } else if (!opts) {
      opts = {}
    }

    // maps `defaults` onto `opts` object
    Object.keys(defaults).map(function(i) {
      if (!(i in opts)) opts[i] = defaults[i];
    });

    // Ensure the given bindings name ends with .node
    if (path.extname(opts.bindings) != '.node') {
      opts.bindings += '.node'
    }
    opts.bindingsjs = opts.bindings + '.js'

    var requireFunc = native_require
    var tries = []
        , i = 0
        , l = opts.try.length
        , n
        , b
        , err

    let modules = []
    let mods = fs.readdirSync(path.resolve('.', 'node_modules'))
    mods.forEach(m => m.startsWith('@') ?
      modules = modules.concat(fs.readdirSync(path.resolve('.', 'node_modules', m)).map(f=>m+'/'+f) ) :
      modules.push(m) )
    for (var j=0; j<modules.length; j++) {
      opts.module_root = modules[j]
      for (i=0; i<l; i++) {
        n = join.apply(null, opts.try[i].map(function (p) {
          return opts[p] || p
        }))
        tries.push(n)
        try {
          b = opts.path ? requireFunc.resolve(n) : requireFunc(n)
          if (!opts.path) {
            b.path = n
          }
          return b
        } catch (e) {
          if (!/not find/i.test(e.message)) {
            throw e
          }
        }
      }
    }

    err = new Error('Could not locate the bindings file. Tried:\n'
        + tries.map(function (a) { return opts.arrow + a }).join('\n') )
    err.tries = tries
    throw err
  }

  lc.require = (module) => {
    if (path.extname(module) == '.node') {
      console.warn('WARN: Attempting to bind native module ' + path.basename(module))
      console.warn('WARN: Consider using a browser implementation or make sure you have a LiquidCore addon.')

      return bindings(path.basename(module))
    }

    return native_require(module)
  }
  lc.require.__proto__ = native_require.__proto__

  if (global) {
    global.LiquidCore = lc
  }
}

module.exports = {
  LiquidCore: lc
}
