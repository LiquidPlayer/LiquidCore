//
//  FileSystem.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/10/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef FileSystem_h
#define FileSystem_h

#import "Process.h"

@interface FileSystem : NSObject
+ (id) createInContext:(JSContext *)context
              uniqueID:(NSString*)uniqueID
       mediaAccessMask:(MediaAccessMask)mask;

+ (void) uninstallLocal:(NSString*)uniqueID;
- (void) cleanUp;
@end

#endif /* FileSystem_h */
