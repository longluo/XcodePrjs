//
//  SimpleTableCell.h
//  SingleTable
//
//  Created by Luo Long on 13-4-5.
//  Copyright (c) 2013年 Long Luo. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SimpleTableCell : UITableViewCell
@property (nonatomic, weak) IBOutlet UILabel *nameLabel;
@property (nonatomic, weak) IBOutlet UILabel *prepTimeLabel;
@property (nonatomic, weak) IBOutlet UIImageView *thumbnailImageView;


@end
