var http = require('http')
var url = require('url')
var fs = require('fs')
var path = require('path')
var browserify = require('browserify')
var insertGlobals = require('insert-module-globals')
var zlib = require('zlib')

function server() {
    function read_manifest(path) {
        if (path.endsWith('.js')) path = path.substring(0,path.length-3)
        path += '.manifest'
        var json = String(fs.readFileSync(path))
        return JSON.parse(json)
    }

    function parse_versionstring(string) {
        return (string || "0").split('.')
            .map((value, index) => (index > 2) ?0 :parseInt(value,10) * Math.pow(100,2-index))
            .reduce((sum, value) => sum + value)
    }

    // The user agent is assumed to be in the following format:
    // LiquidCore/[version] ([os/platform info]) Surface ([surface/[version]]) Binding ([modules])
    // Example:
    // LiquidCore/0.2.0 (Android; API 25; en-us) Surface (.Console/0.2.0) Binding (sqlite3)
    //
    // LiquidCore/[version] - where [version] identifies the release of LiquidCore
    // [os/platform info] - identifies OS specifics
    // Surface ([surface/[version]]) - Available surfaces.  Assumes 'org.liquidplayer.surfaces'
    //     prefix if none provided
    // Binding ([modules]) specifies list of modules which LiquidCore binds natively, and
    //     thus should be excluded from browserification
    function parse_useragent(agent) {
        var platform = agent.match(/LiquidCore\/(\s*[0-9\.]*)\s*\((.*?)\)/i)
        var surfaces = agent.match(/Surface\s+\((.*?)\)/i)
        var bindings = agent.match(/Binding\s+\((.*?)\)/i)
    
        return {
            version : parse_versionstring((platform && platform.length > 1) ? platform[1]:"0"),

            info : (platform && platform.length > 2) ? 
                platform[2].split(';').map(string => string.trim()) : [],

            surfaces : (surfaces && surfaces.length > 1) ?
                surfaces[1].split(';').map(string => string.trim())
                .map(string => {
                    var surface = string.split('/').map(string => string.trim())
                    if (surface[0].startsWith('.')) surface[0] =
                        'org.liquidplayer.surfaces' + surface[0]
                    surface[1] = parse_versionstring(surface[1] || "")
                    return { surface: surface[0], version: surface[1] }
                }) : [],
            
            bindings : (bindings && bindings.length > 1) ?
                bindings[1].split(';').map(string => string.trim()) : []
        }
    }

    function match_version(cmp, version) {
        var v
        if (typeof version === 'string' || typeof version == 'number') {
            v = {
                min : parse_versionstring(String(version)),
                max : Number.MAX_SAFE_INTEGER
            }
        } else if (version && version === Object(version)) {
            v = {
                min : parse_versionstring(version.min),
                max : parse_versionstring(version.max)
            }
        } else {
            v = {
                min : 0,
                max : Number.MAX_SAFE_INTEGER
            }
        }
        var foo = cmp >= v.min && cmp <= v.max
        return foo
    }

    function match_surface(cmp, surface) {
        var s
        if (typeof surface === 'string') {
            s = {
                surface : surface,
                version_match : true
            }
        } else if (surface && surface === Object(surface)) {
            s = {
                surface : surface.surface,
                version_match : match_version(cmp.version, surface.version)
            }
        } else {
            return false;
        }
        s.surface = s.surface && s.surface.startsWith('.') ?
            'org.liquidplayer.surfaces' + s.surface : s.surface
        return s.version_match && s.surface == cmp.surface
    }

    function match_surfaces(cmp, surfaces) {
        if (!surfaces || !surfaces.length) return true
        if (!cmp || !cmp.length) return false
        for (var i=0; i<cmp.length; i++) {
            if (surfaces.map( surface => match_surface(cmp[i], surface) )
                .reduce( (truth, match) => truth || match ))
                return true;
        }
        return false;
    }

    function match_info(cmp, info) {
        if (!info || !info.length) return true
        if (!cmp || !cmp.length) return false
        info = info && Object(info) === info ? info : [ info ]
        var upper = cmp.map( string => String(string).toUpperCase() )
        for (var i=0; i<info.length; i++) {
            if (!upper.includes(String(info[i]).toUpperCase())) return false
        }
        return true
    }

    function match_api(cmp, api) {
        var capi = cmp.find( string => String(string).startsWith('API ') )
        capi = capi ? parse_versionstring(capi.substr(4)) : Number.MAX_SAFE_INTEGER
        return match_version(capi, api)
    }

    function parse_manifest(agent, manifest) {
        if (manifest && Object(manifest) === manifest) {
            for (var i=0; manifest.configs && i<manifest.configs.length; i++) {
                if (match_version(agent.version, manifest.configs[i].version) &&
                    match_info(agent.info, manifest.configs[i].info) &&
                    match_api(agent.info, manifest.configs[i].api) &&
                    match_info(agent.bindings, manifest.configs[i].bindings) &&
                    match_surfaces(agent.surfaces, manifest.configs[i].surfaces)) {
                    
                    return {
                        file : manifest.configs[i].file,
                        bindings : manifest.configs[i].bindings,
                        transforms : manifest.configs[i].transforms,
                    }
                }
            }
        }
        return {}
    }

    function compile(fileObj, lastUpdate) {
        lastUpdate = lastUpdate ? Date.parse(lastUpdate) : 0
        if (!fileObj || !fileObj.file || typeof fileObj.file !== 'string') return
    
        try {
            var stat = fs.lstatSync(fileObj.file)
            var outfile = fileObj.file.lastIndexOf('/') === -1 ? './.lib/' + fileObj.file : 
                fileObj.file.substring(0, fileObj.file.lastIndexOf('/')) + '/.lib/' +
                fileObj.file.substring(fileObj.file.lastIndexOf('/'))
            var outdir = outfile.substring(0, outfile.lastIndexOf('/'))
            try {
                fs.mkdirSync(outdir)
            } catch (e) {}
            try {
                var ostat = fs.lstatSync(outfile)
                if (ostat.isFile() && Date.parse(ostat.mtime) > Date.parse(stat.mtime)) {
                    if (lastUpdate > Date.parse(ostat.mtime)) {
                        console.log(outfile + ' is up-to-date')
                        return 304
                    }
                    console.log('using cached ' + outfile)
                    return fs.createReadStream(outfile)
                }
            } catch (e) {
                // file not found; that's ok
            }
            console.log('Browserifying to ' + outfile)
            var stream = fs.createWriteStream(outfile)

            var insertGlobalVars = {};
            var wantedGlobalVars = ['__filename', '__dirname']
            Object.keys(insertGlobals.vars).forEach(function (x) {
                if (wantedGlobalVars.indexOf(x) === -1) {
                    insertGlobalVars[x] = undefined
                }
            })

            var noParse = ['process', 'assert', 'buffer', 'child_process', 'constants',
                'crypto', 'events', 'fs', 'os', 'path', 'process', 'readline', 'stream',
                'url', 'util', 'vm']
            if (fileObj.bindings) noParse += fileObj.bindings
            var b = browserify({
                commondir : false,
                builtins : false,
                detectGlobals : true,
                bundleExternal : true,
                dedupe : true,
                insertGlobalVars: insertGlobalVars,
                ignoreMissing : true,
                noParse: noParse,
                extensions: [ '.js' ],
                entries: fileObj.file,
                browserField: false
            })
        
            for (var i=0; fileObj.transforms && i < fileObj.transforms.length; i++) {
                b.transform({
                    global: true
                }, fileObj.transforms[i])            
            }
            var s = b.bundle((err,buf) => err ? console.error(err) : {})
            s.pipe(stream)
            return s
        } catch (e) {
            console.error(e)
        }
    }

    return http.createServer(function (request, response) {
        try {
            var requestUrl = url.parse(request.url)

            var fsPath = './'+path.normalize(requestUrl.pathname)
            var mPath = fsPath.endsWith('.js') ?
               fsPath.substring(0, fsPath.length-3) + '.manifest' :
               null

            response.writeHead(200)
            var fileStream
            if (mPath) {
                var manifest = JSON.parse(String(fs.readFileSync(mPath)))
                var agent = parse_useragent(request.headers['user-agent'])
                console.log(manifest)
                console.log(request.headers['user-agent'])
                console.log(agent)
                fileStream = compile(parse_manifest(agent,manifest),
                    request.headers['if-modified-since'])
                if (fileStream === 304) {
                    response.writeHead(304)
                    response.end()
                    console.log("Sending 304")
                    return
                }
            } else {
                fileStream = fs.createReadStream(fsPath)
            }
            if (!fileStream) {
                throw new Error("File does not exist or user agent doesn't match manifest")
            }
            var acceptEncoding = request.headers['accept-encoding']
            if (!acceptEncoding) {
                acceptEncoding = ''
            }
            console.log('accept-encoding: ' + acceptEncoding)

            // Note: this is not a conformant accept-encoding parser.
            // See http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.3
            if (acceptEncoding.match(/\bdeflate\b/)) {
                response.writeHead(200, { 'Content-Encoding': 'deflate' })
                fileStream.pipe(zlib.createDeflate()).pipe(response)
            } else if (acceptEncoding.match(/\bgzip\b/)) {
                response.writeHead(200, { 'Content-Encoding': 'gzip' })
                fileStream.pipe(zlib.createGzip()).pipe(response)
            } else {
                response.writeHead(200, {})
                fileStream.pipe(response);
            }
            fileStream.on('error',function(e) {
               response.writeHead(404)
               response.end()
            })
        } catch(e) {
            response.writeHead(500)
            response.end()
            console.log(e.stack)
        }
    })
}

module.exports = {
    createServer : function() { return server() }
}