const path = require('path')
const fs = require('fs')

const jazzy = (() => {
/*
clean: true
objc: true
umbrella_header: LiquidCore/LiquidCore.h
framework_root: .
sdk: iphonesimulator
module: LiquidCore
module_version: {{version}}
author: LiquidPlayer
author_url: https://github.com/LiquidPlayer
github_url: https://github.com/LiquidPlayer/LiquidCore
copyright: © 2018-2020 LiquidPlayer
hide_documentation_coverage: true
output: ../../../../LiquidPlayer.github.io/LiquidCoreiOS/{{version}}
root_url: https://liquidplayer.github.io/LiquidCoreiOS/{{version}}

*/
}).toString().split(/\n/).slice(2, -2).join('\n')

let inp = fs.readFileSync(path.resolve('../../..','package.json'))
let data = JSON.parse(inp)

let version = data.version

let yaml = jazzy.replace(/{{version}}/g, version)

fs.writeFileSync('.jazzy.yaml', yaml)
