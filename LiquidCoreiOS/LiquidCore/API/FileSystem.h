/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef FileSystem_h
#define FileSystem_h

#import "Process.h"

@interface FileSystem : NSObject
@property (nonatomic, copy) NSString* modulePath;
+ (id) createInContext:(JSContext *)context
              uniqueID:(NSString*)uniqueID
       mediaAccessMask:(MediaAccessMask)mask;

+ (void) uninstallLocal:(NSString*)uniqueID;
- (void) cleanUp;
@end

#endif /* FileSystem_h */
