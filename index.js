const events = require('events')

let lc = global && global.LiquidCore

/* If we are not running on LiquidCore, fake the event emitter for node-based dev
 */
if (!lc) {
  class LiquidCore extends events {}
  lc = new LiquidCore();
  if (global) {
    global.LiquidCore = lc
  }
}

module.exports = lc
