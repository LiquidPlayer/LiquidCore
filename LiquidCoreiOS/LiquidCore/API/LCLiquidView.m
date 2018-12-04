/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#import <LiquidCore/LiquidCore.h>
#import "Process.h"

static NSMutableArray* registeredSurfaces;

@implementation LCSurfaceRegistration
+ (void) registerSurface:(Class<LCSurface>)surfaceClass
{
    if (registeredSurfaces == nil) {
        registeredSurfaces = [[NSMutableArray alloc] init];
    }
    if (![registeredSurfaces containsObject:surfaceClass]) {
        [registeredSurfaces addObject:surfaceClass];
    }
}
@end

@interface LCLiquidView() <LCMicroServiceDelegate>
@property (nonatomic, strong) UIView<LCSurface> *surfaceView;
@end

@implementation LCLiquidView {
    NSMutableArray *availableSurfaces_;
    BOOL customSurfaces_;
    LCMicroService *service_;
    NSMutableDictionary *boundSurfaces_;
    id<LoopPreserver> preserver_;
    NSArray *argv_;
}

- (id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        availableSurfaces_ = [NSMutableArray arrayWithArray:registeredSurfaces];
        customSurfaces_ = NO;
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        availableSurfaces_ = [NSMutableArray arrayWithArray:registeredSurfaces];
        customSurfaces_ = NO;
    }
    return self;
}

- (void) awakeFromNib
{
    [super awakeFromNib];
    NSMutableArray *array = [[NSMutableArray alloc] init];

    if (self.arguments != nil) {
        NSScanner *scanner = [NSScanner scannerWithString:self.arguments];
        NSString *substring;
        
        while (scanner.scanLocation < self.arguments.length) {
            unichar character = [self.arguments characterAtIndex:scanner.scanLocation];
            if (character == '"') {
                [scanner setScanLocation:(scanner.scanLocation + 1)];
                [scanner scanUpToString:@"\"" intoString:&substring];
                [scanner setScanLocation:(scanner.scanLocation + 1)];
            }
            else {
                [scanner scanUpToString:@" " intoString:&substring];
            }
            [array addObject:substring];
            
            if (scanner.scanLocation < self.arguments.length) [scanner setScanLocation:(scanner.scanLocation + 1)];
        }
    }
    
    if (self.jsResource != nil) {
        self.URL = [[[NSBundle mainBundle] URLForResource:_jsResource withExtension:@"js"] absoluteString];
    }
    
    if (self.availableSurfaces != nil) {
        NSScanner *scanner = [NSScanner scannerWithString:self.arguments];
        NSString *substring;
        
        while (scanner.scanLocation < self.arguments.length) {
            unichar character = [self.arguments characterAtIndex:scanner.scanLocation];
            [scanner scanUpToString:@" " intoString:&substring];
            [self enableSurface:substring];
            
            if (scanner.scanLocation < self.arguments.length) [scanner setScanLocation:(scanner.scanLocation + 1)];
        }
    }
    
    if (self.URL == nil) {
        self.URL = [[LCMicroService devServer] absoluteString];
    }

    if (array.count > 0) {
        [self start:[NSURL URLWithString:self.URL] argv:argv_];
    } else {
        NSLog(@"URL = %@", self.URL);
        [self start:[NSURL URLWithString:self.URL]];
    }
}

- (void) layoutSubviews
{
    [super layoutSubviews];
    
    if (self.surfaceView != nil) {
        [self.surfaceView setFrame:self.frame];
        [self.surfaceView setNeedsLayout];
    }
}

- (void) enableSurface:(NSString*)canonicalName, ...
{
    if (!customSurfaces_) {
        customSurfaces_ = YES;
        availableSurfaces_ = [[NSMutableArray alloc] init];
    }
    
    for (Class<LCSurface> surface in registeredSurfaces) {
        NSString *name = [[surface class] SURFACE_CANONICAL_NAME];
        if ([name isEqualToString:canonicalName]) {
            if (![availableSurfaces_ containsObject:surface]) {
                [availableSurfaces_ addObject:surface];
            }
        }
        
        va_list args;
        va_start(args,canonicalName);
        NSString *arg;
        while(( arg = va_arg(args, id))){
            if ([name isEqualToString:arg]) {
                if (![availableSurfaces_ containsObject:surface]) {
                    [availableSurfaces_ addObject:surface];
                }
            }
        }
        va_end(args);
    }
}

- (LCMicroService *) start:(NSURL*)uri argv:(NSArray*)argv
{
    if (uri == nil) {
        uri = [LCMicroService devServer];
    }
    self.URL = uri.absoluteString;

    service_ = [[LCMicroService alloc] initWithURL:uri delegate:self];
    [service_ setAvailableSurfaces:availableSurfaces_];
    if (argv == nil) {
        [service_ start];
    } else {
        [service_ startWithArguments:argv];
    }
    return service_;
}

- (LCMicroService *) start:(NSURL*)uri arguments:(NSString*)argv, ...
{
    if (uri == nil) {
        uri = [LCMicroService devServer];
    }
    self.URL = uri.absoluteString;

    service_ = [[LCMicroService alloc] initWithURL:uri delegate:self];
    [service_ setAvailableSurfaces:availableSurfaces_];
    if (argv == nil) {
        [service_ start];
    } else {
        NSMutableArray *argz = [[NSMutableArray alloc] initWithObjects:argv, nil];
        va_list args;
        va_start(args,argv);
        NSString *arg;
        while(( arg = va_arg(args, id))){
            [argz addObject:arg];
        }
        va_end(args);
        [service_ startWithArguments:argz];
    }
    return service_;
}

- (LCMicroService *) start:(NSURL*)uri
{
    return [self start:uri arguments:nil];
}

static NSString* createPromiseObject =
    @"(()=>{"
    @"  var po = {}; var clock = true;"
    @"  var timer = setInterval(()=>{if(!clock) clearTimeout(timer);}, 100); "
    @"  po.promise = new Promise((resolve,reject)=>{po.resolve=resolve;po.reject=reject});"
    @"  po.promise.then(()=>{clock=false}).catch(()=>{clock=false});"
    @"  return po;"
    @"})();";

- (JSValue*) bind:(JSContext*)context surface:(NSString*)canonicalSurface config:(JSValue*)config
{
    JSValue* promiseObj = [context evaluateScript:createPromiseObject];
    @try {
        if (boundSurfaces_ && [[boundSurfaces_ allKeys] containsObject:canonicalSurface]) {
            // This surface is already bound or in the process of binding.  Don't do it again.
            // FIXME: The more elegant way to handle this is to resolve the promise if and when
            // the original binding completes, but this would require some refactoring.  For now,
            // just reject the promise.
            @throw [[NSException alloc] initWithName:@"Surface already bound"
                                              reason:@"Surface already bound" userInfo:nil];
        }
        if (service_ == nil) {
            @throw [[NSException alloc] initWithName:@"Service not available"
                                              reason:@"Service not available" userInfo:nil];
        }
        if (!boundSurfaces_) boundSurfaces_ = [[NSMutableDictionary alloc] init];
        BOOL found = false;
        for (Class<LCSurface> surface in self->availableSurfaces_) {
            if ([[[surface class] SURFACE_CANONICAL_NAME] isEqualToString:canonicalSurface]) {
                found = true;
                JSValue *exportObject = [JSValue valueWithNewObjectInContext:context];
                exportObject[@"attach"] = ^{
                    id<LCSurface> surface = self->boundSurfaces_[canonicalSurface];
                    return [self attach:[JSContext currentContext]
                                   this:[JSContext currentThis]
                                surface:surface];
                };
                exportObject[@"detach"]= ^{
                    return [self detach:[JSContext currentContext]];
                };
                
                dispatch_async(dispatch_get_main_queue(), ^{
                    @try {
                        UIView<LCSurface> *bindable = [[[surface class] alloc] initWithFrame:self.frame];
                        [self->boundSurfaces_ setObject:bindable forKey:[[surface class] SURFACE_CANONICAL_NAME]];
                        [bindable bind:self->service_
                                export:exportObject
                                config:config onBound:^{
                            [[self->service_ process] async:^(JSContext *context) {
                                [promiseObj[@"resolve"] callWithArguments:@[exportObject]];
                            }];
                        } onError:^(NSString *errorMessage) {
                            [[self->service_ process] async:^(JSContext *context) {
                                [promiseObj[@"reject"] callWithArguments:@[errorMessage]];
                            }];
                        }];
                    } @catch (NSException *e) {
                        NSLog(@"exception: %@", e);
                        [[self->service_ process] async:^(JSContext *context) {
                            [promiseObj[@"reject"] callWithArguments:@[e.description]];
                        }];
                    }
                });
                break;
            }
        }
        if (!found) {
            @throw [[NSException alloc] initWithName:@"Surface is not available"
                                              reason:@"Surface is not avaliable" userInfo:nil];
        }

    } @catch (NSException *e) {
        NSLog(@"exception: %@", e);
        [promiseObj[@"reject"] callWithArguments:@[e.description]];
    }
    
    return promiseObj[@"promise"];
}

- (JSValue*) attach:(JSContext*)context this:(JSValue*)this surface:(id<LCSurface>)surface
{
    JSValue* promiseObj = [context evaluateScript:createPromiseObject];
    @try {
        if (service_ == nil) {
            @throw [[NSException alloc] initWithName:@"Service not available"
                                              reason:@"Service not available" userInfo:nil];
        }
        
        NSLog(@"attach: surface = %@", surface);
        
        if (boundSurfaces_ == nil) {
            @throw [[NSException alloc] initWithName:@"No surfaces have been bound"
                                              reason:@"No surfaces have been bound" userInfo:nil];
        }
        if (surface == nil) {
            @throw [[NSException alloc] initWithName:@"Surface not available"
                                              reason:@"Cannot attach to non-existant surface" userInfo:nil];
        }
        
        dispatch_async(dispatch_get_main_queue(), ^{
            @try {
                UIView<LCSurface> *view = [surface attach:self->service_ onAttached:^{
                    [self->service_.process async:^(JSContext* context){
                        [promiseObj[@"resolve"] callWithArguments:@[this]];
                    }];
                } onError:^(NSString *errorMessage) {
                    [[self->service_ process] async:^(JSContext *context) {
                        [promiseObj[@"reject"] callWithArguments:@[errorMessage]];
                    }];
                }];
                if (self.surfaceView != nil) {
                    [self.surfaceView removeFromSuperview];
                    self.surfaceView = nil;
                }
                self.surfaceView = view;
                [self.surfaceView setFrame:self.frame];
                [self.surfaceView setNeedsLayout];
                [self addSubview:self.surfaceView];
                
                [self.surfaceView.topAnchor constraintEqualToAnchor:self.topAnchor];
                [self.surfaceView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor];
                [self.surfaceView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor];
                [self.surfaceView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor];
                
            } @catch (NSException *e) {
                NSLog(@"exception: %@", e);
                [[self->service_ process] async:^(JSContext *context) {
                    [promiseObj[@"reject"] callWithArguments:@[e.description]];
                }];
            }
        });
    } @catch (NSException *e) {
        NSLog(@"exception: %@", e);
        [self detach:context];
        [promiseObj[@"reject"] callWithArguments:@[e.description]];
    }
    
    return promiseObj[@"promise"];
}

- (JSValue*) detach:(JSContext*)context
{
    JSValue* promiseObj = [context evaluateScript:createPromiseObject];
    if (self.surfaceView != nil) {
        [self.surfaceView detach];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.surfaceView removeFromSuperview];
            self.surfaceView = nil;
            [self->service_.process async:^(JSContext* context){
                [promiseObj[@"resolve"] callWithArguments:@[]];
            }];
        });
    } else {
        [promiseObj[@"resolve"] callWithArguments:@[]];
    }
    return promiseObj[@"promise"];
}

#pragma - MicroServiceDelegate

- (void) onStart:(LCMicroService*)service
{
    [service.process sync:^(JSContext* context) {
        JSValue *liquidcore = context[@"LiquidCore"];
        NSMutableDictionary *availableSurfaces = [[NSMutableDictionary alloc] init];
        for (Class<LCSurface> surface in self->availableSurfaces_) {
            [availableSurfaces setObject:[[surface class] SURFACE_VERSION]
                                  forKey:[[surface class] SURFACE_CANONICAL_NAME]];
        }
        liquidcore[@"availableSurfaces"] = availableSurfaces;
        liquidcore[@"bind"] = ^(NSString* s, JSValue* config){
            return [self bind:context surface:s config:config];
        };
    }];
}

- (void) onExit:(LCMicroService*)service exitCode:(int)exitCode
{
    boundSurfaces_ = nil;
}

- (void) onError:(LCMicroService*)service exception:(NSException*)exception
{
    boundSurfaces_ = nil;
    NSLog(@"error: %@", exception);
}

@end
