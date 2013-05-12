//
//  SocialSharingViewController.m
//  SocialSharing
//
//  Created by Luo Long on 13-4-6.
//  Copyright (c) 2013年 Long Luo. All rights reserved.
//

#import "SocialSharingViewController.h"
#import <Social/Social.h>


@interface SocialSharingViewController ()

@end

@implementation SocialSharingViewController

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



- (IBAction)postToWeibo:(id)sender
{
    if ([SLComposeViewController isAvailableForServiceType:SLServiceTypeSinaWeibo])
    {
        SLComposeViewController *weiboSheet = [SLComposeViewController
                                               composeViewControllerForServiceType:SLServiceTypeSinaWeibo];
        
        [weiboSheet setInitialText:@"My First Weibo Message in iOS by Long Luo"];
        
        [self presentViewController:weiboSheet animated:YES completion:nil];
    }
}


- (IBAction)postToFacebook:(id)sender
{
    if([SLComposeViewController isAvailableForServiceType:SLServiceTypeFacebook])
    {
        SLComposeViewController *controller = [SLComposeViewController composeViewControllerForServiceType:SLServiceTypeFacebook];
        
        [controller setInitialText:@"First post from my iPhone app"];
        [self presentViewController:controller animated:YES completion:Nil];
    }
}



- (IBAction)postToTwitter:(id)sender
{
    if ([SLComposeViewController isAvailableForServiceType:SLServiceTypeTwitter])
    {
        SLComposeViewController *tweetSheet = [SLComposeViewController
                                               composeViewControllerForServiceType:SLServiceTypeTwitter];
        [tweetSheet setInitialText:@"Great fun to learn iOS programming at appcoda.com!"];
        [self presentViewController:tweetSheet animated:YES completion:nil];
    }
}


@end
