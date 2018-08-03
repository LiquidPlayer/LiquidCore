//
//  LCLiquidView.m
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/29/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

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
    NSDictionary *boundSurfaces_;
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

    if (self.URL != nil) {
        if (array.count > 0) {
            [self start:[NSURL URLWithString:self.URL] argv:argv_];
        } else {
            [self start:[NSURL URLWithString:self.URL]];
        }
        
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
    if (uri != nil) {
        service_ = [[LCMicroService alloc] initWithURL:uri delegate:self];
        [service_ setAvailableSurfaces:availableSurfaces_];
        if (argv == nil) {
            [service_ start];
        } else {
            [service_ startWithArguments:argv];
        }
        return service_;
    }
    return nil;
}

- (LCMicroService *) start:(NSURL*)uri arguments:(NSString*)argv, ...
{
    if (uri != nil) {
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
    return nil;
}

- (LCMicroService *) start:(NSURL*)uri
{
    return [self start:uri arguments:nil];
}

- (void) attach:(NSString *)surface callback:(JSValue*)callback
{
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
        UIView<LCSurface> *boundSurface = [boundSurfaces_ objectForKey:surface];
        if (boundSurface == nil) {
            @throw [[NSException alloc] initWithName:@"Surface not available"
                                              reason:@"Cannot attach to non-existant surface" userInfo:nil];
        }
        
        dispatch_async(dispatch_get_main_queue(), ^{
            @try {
                UIView<LCSurface> *view = [boundSurface attach:self->service_ onAttached:^{
                    [self->service_.process async:^(JSContext* context){
                        if (callback != nil) {
                            [callback callWithArguments:@[]];
                        }
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
                [self->preserver_ letDie];
            }
        });
    } @catch (NSException *e) {
        NSLog(@"exception: %@", e);
        [self->service_.process async:^(JSContext* context){
            if (callback != nil) {
                JSValue* err = [JSValue valueWithNewErrorFromMessage:e.name inContext:context];
                [callback callWithArguments:@[err]];
            }
        }];
        [self detach:nil];
    }
}

- (void) detach:(JSValue*)callback
{
    if (self.surfaceView != nil) {
        [self.surfaceView detach];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.surfaceView removeFromSuperview];
            self.surfaceView = nil;
            [self->service_.process async:^(JSContext* context){
                if (callback != nil) {
                    [callback callWithArguments:@[]];
                }
            }];
        });
    }
}

#pragma - MicroServiceDelegate

- (void) onStart:(LCMicroService*)service synchronizer:(LCSynchronizer*)synchronizer
{
    [service.process sync:^(JSContext* context) {
        JSValue *liquidcore = context[@"LiquidCore"];
        NSMutableDictionary *availableSurfaces = [[NSMutableDictionary alloc] init];
        for (Class<LCSurface> surface in self->availableSurfaces_) {
            [availableSurfaces setObject:[[surface class] SURFACE_VERSION]
                                  forKey:[[surface class] SURFACE_CANONICAL_NAME]];
        }
        liquidcore[@"availableSurfaces"] = availableSurfaces;
        liquidcore[@"attach"] = ^(NSString *sfc, JSValue* callback) {
            [self attach:sfc callback:callback];
        };
        liquidcore[@"detach"] = ^(JSValue* callback) {
            [self detach:callback];
        };

        // Bind surfaces
        self->preserver_ = [self->service_.process keepAlive];
        
        NSMutableDictionary *bound = [[NSMutableDictionary alloc] init];
        for (Class<LCSurface> surface in self->availableSurfaces_) {
            [synchronizer enter];
            dispatch_async(dispatch_get_main_queue(), ^{
                @try {
                    UIView<LCSurface> *bindable = [[[surface class] alloc] initWithFrame:self.frame];
                    [bindable bind:service synchronizer:synchronizer];
                    [bound setObject:bindable forKey:[[surface class] SURFACE_CANONICAL_NAME]];
                } @catch (NSException *e) {
                    NSLog(@"exception: %@", e);
                    [self->preserver_ letDie];
                } @finally {
                    [synchronizer exit];
                }
            });
        }
        self->boundSurfaces_ = bound;
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
