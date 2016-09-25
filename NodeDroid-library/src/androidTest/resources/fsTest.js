(function() {
    var fs = require('fs');

    // Test 1: Try to read the /data directory
    global.a = null;
    try {
        global.a = fs.readdirSync("/data");
    } catch(e) {
        global.a = e;
    }

    // Test 2: Try to use relative pathing to get there
    global.b = null;
    try {
        global.b = fs.readdirSync("../data");
    } catch(e) {
        global.b = e;
    }

    // Test 3: Try to chmod there
    global.c = null;
    try {
        process.chdir('/data');
        global.c = fs.readdirSync(".");
    } catch(e) {
        global.c = e;
    }

    // Test 4: Try to chmod there with relative pathing
    global.d = null;
    try {
        process.chdir('../data');
        global.d = fs.readdirSync(".");
    } catch(e) {
        global.d = e;
    }

    // Test 5: Try to create a symlink to do the nasty
    global.e = null;
    try {
        fs.symlinkSync('/data', './naughty');
        global.e = fs.readdirSync("./naughty/");
    } catch (e) {
        global.e = e;
    }
})()
