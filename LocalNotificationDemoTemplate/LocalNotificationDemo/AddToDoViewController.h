//
//  AddToDoViewController.h
//  LocalNotificationDemo
//
//  Created by LongLuo on 9/6/13.
//  Copyright (c) 2013 LongLuo. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface AddToDoViewController : UIViewController <UITextFieldDelegate>
- (IBAction)cancel:(id)sender;
- (IBAction)save:(id)sender;
@property (weak, nonatomic) IBOutlet UITextField *itemText;
@property (weak, nonatomic) IBOutlet UIDatePicker *datePicker;

@end
