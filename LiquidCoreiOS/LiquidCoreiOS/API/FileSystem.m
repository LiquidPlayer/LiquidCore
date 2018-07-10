//
//  FileSystem.m
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/10/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <JavaScriptCore/JavaScriptCore.h>
#import "FileSystem.h"

@protocol FileSystemExports <JSExport>
@property (nonatomic, readwrite) NSString* cwd;
@property (nonatomic, readonly) JSValue *fs;
@property (nonatomic, readonly) JSValue *alias;
@property (nonatomic, readwrite) NSMutableArray *aliases_;
@property (nonatomic, readwrite) NSMutableArray *access_;
@end

@interface FileSystemImpl : FileSystem<FileSystemExports>
@property (nonatomic, readwrite) NSString* cwd;
@property (nonatomic, readwrite) JSValue *fs;
@property (nonatomic, readwrite) JSValue *alias;
@property (nonatomic, readwrite) NSMutableArray *aliases_;
@property (nonatomic, readwrite) NSMutableArray *access_;

@property (nonatomic, copy) NSString *uniqueID;
@property (nonatomic, copy) NSString *sessionID;

@property (class, atomic, readonly) NSMutableArray* activeSessions;

- (id) initWithContext:(JSContext*)context
              uniqueID:(NSString*)uniqueID
       mediaAccessMask:(MediaAccessMask)mask;
- (void) setUp:(JSContext*)context mediaAccessMask:(MediaAccessMask)mask;
+ (void) sessionWatchdog;
@end

@implementation FileSystem
+ (id) createInContext:(JSContext *)context
              uniqueID:(NSString*)uniqueID
       mediaAccessMask:(MediaAccessMask)mask;
{
    id fs = [[FileSystemImpl alloc] initWithContext:context uniqueID:uniqueID mediaAccessMask:mask];
    return fs;
}

- (void)uninstallLocal:(NSString *)uniqueID {}
- (void)cleanUp {}
@end

@interface JSBuilder : NSObject
@property (nonatomic, copy) NSMutableString *js;
- init;
- (void) realDir:(NSString*) ios;
- (void) alias:(NSString*)alias ios:(NSString*)ios mask:(MediaAccessMask)mask;
- (void) mkdir:(NSString*)alias ios:(NSString*)ios mask:(MediaAccessMask)mask;
- (void) symlink:(NSString*)alias target:(NSString*)target linkpath:(NSString*)linkpath mask:(MediaAccessMask)mask;
- (void) symlinkRealTarget:(NSString*)alias target:(NSString*)target linkpath:(NSString*)linkpath mask:(MediaAccessMask)mask;
@end

@implementation JSBuilder
- init
{
    self = [super init];
    if (self) {
        _js = [[NSMutableString alloc] init];
    }
    return self;
}

- (void) append:(NSString *)str
{
    [_js appendString:str];
}

- (void) realDir:(NSString*) ios
{
    [self append:@"(function(){try {return fs.realpathSync('"];
    [self append:ios];
    [self append:@"');}catch(e){}})()"];
}

- (void) alias:(NSString*)alias ios:(NSString*)ios mask:(MediaAccessMask)mask
{
    [self append:@"fs_.aliases_['"];
    [self append:alias];
    [self append:@"']="];
    [self realDir:ios];
    [self append:@";fs_.access_['"];
    [self append:alias];
    [self append:@"']="];
    [self append:[NSString stringWithFormat:@"%d", mask]];
    [self append:@";"];
}

- (void) mkdir:(NSString*)alias ios:(NSString*)ios mask:(MediaAccessMask)mask
{
    NSError *error;
    
    if (![[NSFileManager defaultManager] createDirectoryAtPath:ios
                                   withIntermediateDirectories:YES
                                                    attributes:nil
                                                         error:&error])
    {
        NSLog(@"Create directory error: %@", error);
    }
    else
    {
        NSLog(@"mkdir: Created directory %@", ios);
    }

    [self alias:alias ios:ios mask:mask];
}

- (void) symlink:(NSString*)alias target:(NSString*)target linkpath:(NSString*)linkpath mask:(MediaAccessMask)mask
{
    [self append:@"(function(){fs.symlinkSync('"];
    [self append:target];
    [self append:@"','"];
    [self append:linkpath];
    [self append:@"');})();"];

    [self alias:alias ios:target mask:mask];
}

- (void) symlinkRealTarget:(NSString*)alias target:(NSString*)target linkpath:(NSString*)linkpath mask:(MediaAccessMask)mask
{
    [self append:@"(function(){fs.symlinkSync("];
    [self realDir:target];
    [self append:@",'"];
    [self append:linkpath];
    [self append:@"');})();"];

    [self alias:alias ios:target mask:mask];
}
@end

@implementation FileSystemImpl
static NSMutableArray* _activeSessions = nil;
static NSDate* lastBark;

static NSString* fs_code =
@"new Function('file',\""
@"if (!file.startsWith('/')) { file = ''+this.cwd+'/'+file; }"
@"try { file = require('path').resolve(file); } catch (e) {console.log(e);}"
@"var access = 0;"
@"var keys = Object.keys(this.aliases_).sort().reverse();"
@"for (var p=0; p<keys.length; p++) {"
@"    if (file.startsWith(this.aliases_[keys[p]] + '/')) {"
@"        file = keys[p] + '/' + file.substring(this.aliases_[keys[p]].length + 1);"
@"        break;"
@"    } else if (file == this.aliases_[keys[p]]) {"
@"        file = keys[p];"
@"        break;"
@"    }"
@"}"
@"var acckeys = Object.keys(this.access_).sort().reverse();"
@"for (var p=0; p<acckeys.length; p++) {"
@"    if (file.startsWith(acckeys[p] + '/') || acckeys[p]==file) {"
@"        access = this.access_[acckeys[p]];"
@"        break;"
@"    }"
@"}"
@"var newfile = file;"
@"for (var p=0; p<keys.length; p++) {"
@"    if (file.startsWith(keys[p] + '/')) {"
@"        newfile = this.aliases_[keys[p]] +'/'+file.substring(keys[p].length + 1);"
@"        break;"
@"    } else if (file == keys[p]) {"
@"        newfile = this.aliases_[keys[p]];"
@"        break;"
@"    }"
@"}"
@"return [access,newfile];"
@"\");";

static NSString* alias_code =
@"new Function('file',\""
@"var keys = Object.keys(this.aliases_).sort().reverse();"
@"for (var p=0; p<keys.length; p++) {"
@"   if (file.startsWith(this.aliases_[keys[p]] + '/')) {"
@"       file = keys[p] + '/' + file.substring(this.aliases_[keys[p]].length + 1);"
@"       break;"
@"   } else if (file == this.aliases_[keys[p]]) {"
@"       file = keys[p];"
@"       break;"
@"   }"
@"}"
@"return file;"
@"\");";

- initWithContext:(JSContext*)context
         uniqueID:(NSString*)uniqueID
  mediaAccessMask:(MediaAccessMask)mask;
{
    self = [super init];
    if (self) {
        
        // clear any dead sessions
        [FileSystemImpl sessionWatchdog];
        
        _uniqueID = uniqueID;
        _sessionID = [[NSUUID UUID] UUIDString];
        if (_activeSessions == nil) {
            _activeSessions = [[NSMutableArray alloc] init];
            lastBark = [NSDate dateWithTimeIntervalSince1970:0];
        }
        [FileSystemImpl.activeSessions addObject:_sessionID];
        
        _aliases_ = [[NSMutableArray alloc] init];
        _access_ = [[NSMutableArray alloc] init];
        
        [self setUp:context mediaAccessMask:mask];
        
        _fs = [context evaluateScript:fs_code];
        _alias = [context evaluateScript:alias_code];
    }
    return self;
}

- (void) setUp:(JSContext*)context mediaAccessMask:(MediaAccessMask)mask
{
    NSString* homedir = NSHomeDirectory();
    NSString* suffix = [NSString stringWithFormat:@"/__org.liquidplayer.node__/_%@", self.uniqueID];
    NSString* sessionSuffix = [NSString stringWithFormat:@"/__org.liquidplayer.node__/sessions/%@", self.sessionID];
    NSString* sessionPath = [NSString stringWithFormat:@"%@/tmp%@", homedir, sessionSuffix];
    NSString* path = [NSString stringWithFormat:@"%@/Library/Caches/%@", homedir, suffix];
    NSString* localPath = [NSString stringWithFormat:@"%@/Library/%@", homedir, suffix];
    NSString* node_modules = [NSString stringWithFormat:@"%@Library/__org.liquidplayer.node__/node_modules", homedir];
 
    JSBuilder* js = [[JSBuilder alloc] init];
    
    // Set up /home (read-only)
    [js mkdir:@"/home" ios:[NSString stringWithFormat:@"%@/home", sessionPath] mask:PermissionsRead];

    // Set up /home/module (read-only)
    NSError *error;
    NSString *module = [NSString stringWithFormat:@"%@/module", localPath];
    if (![[NSFileManager defaultManager] createDirectoryAtPath:module
                                   withIntermediateDirectories:YES
                                                    attributes:nil
                                                         error:&error])
    {
        NSLog(@"Create directory error: %@", error);
    }
    else
    {
        NSLog(@"mkdir: Created directory %@", module);
    }
    [js symlinkRealTarget:@"/home/module"
                   target:module
                 linkpath:[NSString stringWithFormat:@"%@/home/module", sessionPath]
                     mask:PermissionsRead];
    
    // Set up /home/temp (read/write)
    [js mkdir:@"/home/temp" ios:[NSString stringWithFormat:@"%@/temp", sessionPath] mask:PermissionsRW];
    // Set up /home/cache (read/write)
    [js mkdir:@"/home/cache" ios:[NSString stringWithFormat:@"%@/cache", path] mask:PermissionsRW];
    // Set up /home/local (read/write)
    [js mkdir:@"/home/local" ios:[NSString stringWithFormat:@"%@/local", localPath] mask:PermissionsRW];
    // Permit access to node_modules
    [js symlink:@"/home/node_modules"
         target:node_modules
       linkpath:[NSString stringWithFormat:@"%@/home/node_modules", sessionPath]
           mask:PermissionsRead];
    
    [js append:@"fs_.cwd='/home';"];
    
    context[@"fs_"] = self;
    [context evaluateScript:js.js];
    context[@"fs_"] = [JSValue valueWithUndefinedInContext:context];
}

+ (NSMutableArray *)activeSessions
{
    return _activeSessions;
}

+ (void)uninstallSession:(NSString *)sessionId
{
    NSError *error = nil;
    NSString* homedir = NSHomeDirectory();
    NSString* session = [NSString stringWithFormat:@"%@/__org.liquidplayer.node__/sessions/%@", homedir, sessionId];
    NSLog(@"sessionWatchdog: deleting session %@", sessionId);

    [[NSFileManager defaultManager] removeItemAtPath:session error:&error];
    if (error) {
        NSLog(@"Failed to delete session: %@", error);
    }
}

+ (void)sessionWatchdog
{
    if ([lastBark timeIntervalSinceNow] < -5 * 60) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            NSString* homedir = NSHomeDirectory();
            NSString* sessions = [NSString stringWithFormat:@"%@/__org.liquidplayer.node__/sessions", homedir];

            NSFileManager *fileManager = [NSFileManager defaultManager];
            
            NSDirectoryEnumerator * enumerator = [fileManager enumeratorAtPath:sessions];
            [enumerator skipDescendants];
            NSString* path;
            while (path = [enumerator nextObject])
            {
                if (![FileSystemImpl.activeSessions containsObject:path]) {
                    [FileSystemImpl uninstallSession:path];
                }
            }
            lastBark = [NSDate dateWithTimeIntervalSinceNow:0];
        });
    }
}

+ (void)uninstallLocal:(NSString *)uniqueID
{
    NSError *error = nil;
    NSString* homedir = NSHomeDirectory();
    NSString* suffix = [NSString stringWithFormat:@"/__org.liquidplayer.node__/_%@", uniqueID];
    NSString* path = [NSString stringWithFormat:@"%@/Library/Caches/%@", homedir, suffix];
    NSString* localPath = [NSString stringWithFormat:@"%@/Library/%@", homedir, suffix];
    [[NSFileManager defaultManager] removeItemAtPath:path error:&error];
    if (!error) {
        [[NSFileManager defaultManager] removeItemAtPath:localPath error:&error];
    }
    if (error) {
        NSLog(@"Delete directory error: %@", error);
    }
}

- (void)cleanUp
{
    bool needClean = [FileSystemImpl.activeSessions containsObject:self.sessionID];
    if (needClean) {
        [FileSystemImpl.activeSessions removeObject:self.sessionID];
        [FileSystemImpl uninstallSession:self.sessionID];
    }
}

@end
