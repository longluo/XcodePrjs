//
//  LTAppDelegate.h
//  LTBlacklist
//
//  Created by Lex on 6/26/13.
//  Copyright (c) 2013 LexTang.com. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface LTAppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;

@property (assign, nonatomic) UIBackgroundTaskIdentifier bgTask;
@property (assign, nonatomic) BOOL background;
@property (strong, nonatomic) dispatch_block_t expirationHandler;
@property (assign, nonatomic) BOOL jobExpired;


@end
