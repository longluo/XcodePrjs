//
//  SimpleMailViewController.h
//  SimpleMail
//
//  Created by Luo Long on 13-4-6.
//  Copyright (c) 2013年 Long Luo. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>


@interface SimpleMailViewController : UIViewController <MFMailComposeViewControllerDelegate> // Add the delegate
- (IBAction)showEmail:(id)sender;

@end
