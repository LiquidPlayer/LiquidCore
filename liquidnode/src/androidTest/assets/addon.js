setInterval(function(){}, 5000)

const addon = require('native-module-test.node')

function assertFunction(fn) {
    LiquidCore.emit('assert', {expected: 'function', value: typeof fn})
    if (typeof fn !== 'function') {
        process.exit(4)
    }
}

function assertEquals(expected,value) {
    LiquidCore.emit('assert', {expected: expected, value: value})
    if( expected !== value) {
        process.exit(3)
    }
}

assertFunction(addon.highLevelFunction)
assertFunction(addon.lowLevelFunction)
assertEquals(42, addon.highLevelFunction())
assertEquals(43, addon.lowLevelFunction())

process.exit(1)