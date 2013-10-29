//
//  StartHereViewController.m
//  StartHere
//
//  Created by Luo Long on 13-10-29.
//  Copyright (c) 2013年 Long Luo. All rights reserved.
//

#import "StartHereViewController.h"

@interface StartHereViewController ()

@end

@implementation StartHereViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)showMessage
{
    UIAlertView *startHereAlert = [[UIAlertView alloc]
                                   initWithTitle:@"Start Here..." message:@"Start Here"                              delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
    [startHereAlert show];
}


@end
