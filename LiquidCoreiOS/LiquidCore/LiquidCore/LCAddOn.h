/*
 * Copyright (c) 2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <UIKit/UIKit.h>
#import <LiquidCore/LiquidCore.h>
#import <JavaScriptCore/JavaScriptCore.h>

/**
 Supported addons must expose an object that implements this protocol.
 */
@protocol LCAddOn <NSObject>

/**
 When `require('<MODULE_NAME>.node')` is called from JavaScript, this method will
 be triggered if this addon was registered to handle `<MODULE_NAME>`.  This method
 muar execute the `__register_<MODULE_NAME>()` static function created from
 `NODE_MODULE_*` macros in `node.h`.
 
 @param module The name of the module to register.
 */
- (void) register:(NSString*) module;

/**
 Each time `require()` is called for this module from JS, this method will be triggered
 before returning the bound object to the caller.  This is an opportunity to attach anything
 else to the object that may be required from Objective-C or Swift before returning it
 to the caller.
 
 @param binding The native binding object.
 @param service The `LCMicroService` of this process
 */
- (void) require:(JSValue*) binding service:(LCMicroService*)service;

@end

/**
 An `LCAddOnFactory` creates instances of `LCAddOn`
 */
@interface LCAddOnFactory : NSObject

/**
 Default initializer for the factory.
 */
- (instancetype) init;

/**
 Creates an instance of the add-on.
 @return a new instance of the add-on
 */
- (id<LCAddOn>) createInstance;

/**
 Registers an `LCAddOnFactory` factory with LiquidCore.
 @param module The native binding module (`<module>.node`) to be associated with this factory
 @param factory instance of `LCAddOnFactory` to handle binding module
 */
+ (void) registerAddOnFactory:(NSString*)module
                      factory:(LCAddOnFactory*)factory;

@end
