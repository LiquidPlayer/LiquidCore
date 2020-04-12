/*
 * Copyright © 2018 LiquidPlayer
 *
 * Released under the MIT license.  See LICENSE.md for terms.
 */
const fs = require('fs')

const usage = (() => {
/*
Description:
  Initlializes a node module's package.json file for use with LiquidCore.  Also
  creates an 'example.js' service.

Usage:
  npx liquidcore init [<options>]

Options:
  --help     Print this message
*/
}).toString().split(/\n/).slice(2, -2).join('\n')

const pkg = {
  scripts: {
    server: "liquidcore server",
    bundler: "liquidcore bundle",
    init: "liquidcore init",
    "gradle-config": "liquidcore gradle",
    "pod-config": "liquidcore pod",
    postinstall: "liquidcore postinstall"
  },
  liquidcore: {
    __nooverwrite: true,
    entry: ["example.js"]
  }
}

const merge = (config) => {
  return new Promise((resolve,reject) => {
    const write_package_json = (data) => {
      fs.writeFile(config.dir + '/package.json', JSON.stringify(data,null,2), (err) => {
        if (err) return reject(err)
        console.log('package.json was updated.')
        resolve(config)
      })
    }

    fs.readFile(config.dir + '/package.json', (err,old)=>{
      if (err) {
        return reject(config.dir + '/package.json cannot be read.  Make sure you run "npm init" first.')
      }
      var data =JSON.parse(old)
      for (prop in pkg) {
        if (typeof pkg[prop] === 'object') {
          if (!data[prop] || typeof data.prop !== 'object') data[prop] = {}
          for (p in pkg[prop]) {
            if (p != '__nooverwrite' && (!pkg[prop].__nooverwrite || data[prop][p] === undefined))
              data[prop][p] = pkg[prop][p]
          }
        } else {
          data[prop] = pkg[prop]
        }
      }

      write_package_json(data)
    })
  })
}

const cp_file = (config, file, name) => {
  return new Promise((resolve,reject) => {
    fs.stat(config.dir + '/' + name,(err) => {
      if (!err) return resolve(config)
      fs.readFile(__dirname + file, (err,data) => {
        if (err) return reject(err)
        fs.writeFile(config.dir + '/' + name, data, (err) => {
          if (err) return reject(err)
          console.log('Created file ' + name)
          resolve(config)
        })
      })
    })
  })
}

const create_examplejs = (config) => cp_file(config, '/templates/helloworld.js','example.js')

const success = (config) => {
  console.log('Successfully updated package.json for use with LiquidCore.')
  return Promise.resolve(config)
};

async function prepare(override) {
  var args = {}
  Object.assign(args, override)

  if (args.help) {
    console.log(usage)
    process.exit(0)
  }

  let config = {
    dir: args.dir || '.'
  }

  fs.stat(config.dir,async (err,stats) => {
    if (!err && !stats.isDirectory()) {
      console.error('<project-dir> must be a directory containing package.json.')
      process.exit(-3)
    }
    try {
      await create_examplejs(config)
      await merge(config)
      await success(config)
    } catch(err) {
      console.error(err)
      process.exit(-4)
    }
  })
}

module.exports = prepare
