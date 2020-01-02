const fs = require('fs')
const path = require('path')

let resolved_paths = []

function process(p, file, found) {
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
        return fs.readdirSync(resolved).forEach(m => process(resolved, m, found))
      } else {
        try {
          let package = JSON.parse(fs.readFileSync(path.resolve(resolved, 'package.json')))
          let addon = package && package['liquidcore']
          if (addon) {
            found(package, resolved)
          }
          return fs.readdirSync(resolved).forEach(m => process(resolved, m, found))
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
