const configure = require('./Config')
const Metro = require('metro')
const { mergeConfig } = require("metro-config")
const path = require('path')
const fs = require('fs')
const tmp = require('tmp')
const fc = require('filecompare')

const usage = (() => {
  /*
  Usage:
    npx liquidcore bundler [<entry-file>] <out> [<options>]

  Where:
    <entry-file>            The entry file to bundle
    <out>                   Output location
    If <entry-file> is not specified, the script will look for the 'liquidcore.entry'
    property in 'package.json' and process the file (if a string) or files (array)
    to the directory specified in <out>.
    If <out> is omitted, the script will use the 'liquidcore.bundler_output.<platform>'
    property in 'package.json' to determine the directory.

  Examples:
    npx liquidcore bundler index.js ~/index.bundle.js
    npx liquidcore bundler lib/foo.js ~/foo.bundle --reset-cache --dev --platform=ios
    npx liquidcore bundler ~/project_dir/res/raw --platform=android

  Options:
    --project-root=<dir>     The root folder of your project
    --reset-cache            Whether we should reset the cache when starting the build
    --max-workers=<num>      The number of workers we should parallelize the transformer on
    --dev=<bool>             Whether to enable developer mode or not
    --platform=[ios|android] The platform to bundle for

  For more options, you can create a 'metro.config.js' file to specify more advanced
  capabilities.  See https://facebook.github.io/metro/docs/en/configuration for more
  information.
  */
}).toString().split(/\n/).slice(2, -2).join('\n')

function mkdirp(dir) {
  if (fs.existsSync(dir)) { return true }
  const dirname = path.dirname(dir)
  mkdirp(dirname);
  fs.mkdirSync(dir);
}

async function bundler(override) {
  if (override.help || override._.length > 2) {
    console.log(usage)
    process.exit(0)
  }

  var config = configure(await Metro.loadConfig())

  var args = {
    assetExts: [],
    resetCache: false,
    verbose: false,
  }

  let files = []
  let bundler_options = {
    // Issue #155: disable by default
    "minify" : false
  }
  let from_pkgjson = false
  if (override._.length < 2) {
    let pkgjson = JSON.parse(fs.readFileSync(path.resolve('.', 'package.json')))
    if (!Array.isArray(pkgjson.liquidcore.entry)) {
      pkgjson.liquidcore.entry = [pkgjson.liquidcore.entry]
    }
    if (pkgjson.liquidcore && pkgjson.liquidcore.bundler_options &&
      typeof pkgjson.liquidcore.bundler_options === 'object') {
      bundler_options = pkgjson.liquidcore.bundler_options
    }

    let outdir
    if (override._.length == 0) {
      if (!override.platform) {
        console.error('You must either specify an <out> directory or --platform')
        process.exit(-1)
      }
      outdir = pkgjson.liquidcore.bundler_output[override.platform]
      from_pkgjson = true
    } else {
      outdir = override._[0]
    }
    mkdirp(path.resolve(outdir))
    files = pkgjson.liquidcore.entry.map(
      e=>({
        entry: path.resolve('.',e),
        out:   path.resolve(outdir,path.basename(e,'.js'))
      })
    )
  } else {
    files.push({
      entry: override._[0],
      out:   override._[1]
    })
  }

  config = mergeConfig(config, args, override)

  for (f in files) {
    let tmpfil
    let finalout
    if (from_pkgjson) {
      finalout = files[f].out + ".js"
      tmpfil = tmp.fileSync({  postfix: '.js' })
      files[f].out = tmpfil.name
    }
    Object.assign(files[f], bundler_options, override)
    try {
      const copy = (isEqual) => {
        if (!isEqual) {
          fs.copyFileSync(tmpfil.name,finalout)
          console.log('Wrote updated ' + finalout)
        } else {
          console.log(finalout + ' is unchanged')
        }
        tmpfil.removeCallback()
      }
      await Metro.runBuild(config, files[f])
      if (from_pkgjson) {
        if (fs.existsSync(finalout)) {
          await fc(finalout, tmpfil.name, (isEqual) => {
            if (!isEqual) {
              fs.copyFileSync(tmpfil.name,finalout)
              console.log('Wrote updated ' + finalout)
            } else {
              console.log(finalout + ' is unchanged')
            }
            tmpfil.removeCallback()
          })
        } else {
          fs.copyFileSync(tmpfil.name,finalout)
          console.log('Wrote ' + finalout)
        }
      }
    } catch (e) {
      console.error(e)
      if (from_pkgjson)
        tmpfil.removeCallback()
      process.exit(-2)
    }
  }
}

module.exports = bundler