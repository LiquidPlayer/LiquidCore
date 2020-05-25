/*
 * Copyright © 2019-20 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */
const fs = require('fs')
const path = require('path')

function process(p, file, found, resolved_paths) {
  if (!resolved_paths) resolved_paths = []
  let resolved = path.resolve(p, file)
  let real
  try {
    real = fs.realpathSync(resolved)
  } catch (e) {
    return
  }
  if (resolved_paths.includes(real)) return; // Don't get stuck in an endless loop
  resolved_paths.push(real)

  try {
    const stat = fs.statSync(resolved)
    if (stat.isDirectory()) {
      if (file == 'node_modules' || file[0] == '@') {
        return fs.readdirSync(resolved).forEach(m => process(resolved, m, found, resolved_paths))
      } else {
        try {
          let package = JSON.parse(fs.readFileSync(path.resolve(resolved, 'package.json')))
          let addon = package && package['liquidcore']
          if (addon) {
            found(package, resolved)
          }
          return fs.readdirSync(resolved).forEach(m => process(resolved, m, found, resolved_paths))
        } catch (e) {}
      }
    }
  } catch (e) {}
}

const version = (s) => s.split('.').map((v,n) =>
  (s.length-n-1)*100*parseInt(v)).reduce((p,v)=>Number.isInteger(v)?p+v:v)

module.exports = {
  recurse_packages : process,
  version : version,
}
