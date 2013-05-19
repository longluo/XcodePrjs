//
//  MazeViewController.h
//  Maze
//
//  Created by Luo Long on 13-5-19.
//  Copyright (c) 2013年 Long Luo. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/CAAnimation.h>

#define kUpdateInterval (1.0f / 60.0f)

@interface MazeViewController : UIViewController

@property (strong, nonatomic) IBOutlet UIImageView *pacman;

@property (strong, nonatomic) IBOutlet UIImageView *ghost1;
@property (strong, nonatomic) IBOutlet UIImageView *ghost2;
@property (strong, nonatomic) IBOutlet UIImageView *ghost3;

@property (strong, nonatomic) IBOutlet UIImageView *exit;


@property (strong, nonatomic) IBOutletCollection(UIImageView) NSArray *wall;


@end

