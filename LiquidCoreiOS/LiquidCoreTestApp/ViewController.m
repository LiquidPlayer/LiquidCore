//
//  ViewController.m
//  LiquidCoreTestApp
//
//  Created by Eric Lange on 7/27/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#import "ViewController.h"
#import "LiquidCoreiOS.h"

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    NSBundle *testBundle = [NSBundle bundleForClass:[self class]];
    NSURL *console_js = [testBundle URLForResource:@"console" withExtension:@"js"];

    [self.liquidView start:console_js];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
