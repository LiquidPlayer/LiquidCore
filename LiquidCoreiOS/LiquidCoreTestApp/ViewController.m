//
//  ViewController.m
//  LiquidCoreTestApp
//
//  Created by Eric Lange on 7/27/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import "ViewController.h"
#import "LiquidCoreiOS.h"

@interface ViewController () <MicroServiceDelegate>

@end

@implementation ViewController {
    MicroService *service_;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    NSBundle *testBundle = [NSBundle bundleForClass:[self class]];
    NSURL *console_js = [testBundle URLForResource:@"console" withExtension:@"js"];
    
    // First, start a MicroService from a file.  This service creates a small HTTP file server
    service_ = [[MicroService alloc] initWithURL:console_js delegate:self];
    [service_ start];
}

- (void) onStart:(MicroService *)service synchronizer:(Synchronizer *)synchronizer
{
    [self.consoleSurface attach:service onAttached:^() {
        
    }];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
