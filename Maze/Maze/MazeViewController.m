//
//  MazeViewController.m
//  Maze
//
//  Created by Luo Long on 13-5-19.
//  Copyright (c) 2013年 Long Luo. All rights reserved.
//

#import "MazeViewController.h"

@interface MazeViewController ()

@end

@implementation MazeViewController

- (void)viewDidLoad {
    
    [super viewDidLoad];
    
    // Animate ghosts
    CGPoint origin1 = self.ghost1.center;
    CGPoint target1 = CGPointMake(self.ghost1.center.x, self.ghost1.center.y-124);
    CABasicAnimation *bounce1 = [CABasicAnimation animationWithKeyPath:@"position.y"];
    bounce1.duration = 2;
    bounce1.fromValue = [NSNumber numberWithInt:origin1.y];
    bounce1.toValue = [NSNumber numberWithInt:target1.y];
    bounce1.repeatCount = HUGE_VALF;
    bounce1.autoreverses = YES;
    [self.ghost1.layer addAnimation:bounce1 forKey:@"position"];
    
    CGPoint origin2 = self.ghost2.center;
    CGPoint target2 = CGPointMake(self.ghost2.center.x, self.ghost2.center.y+284);
    CABasicAnimation *bounce2 = [CABasicAnimation animationWithKeyPath:@"position.y"];
    bounce2.fromValue = [NSNumber numberWithInt:origin2.y];
    bounce2.toValue = [NSNumber numberWithInt:target2.y];
    bounce2.duration = 2;
    bounce2.repeatCount = HUGE_VALF;
    bounce2.autoreverses = YES;
    [self.ghost2.layer addAnimation:bounce2 forKey:@"position"];
    
    CGPoint origin3 = self.ghost3.center;
    CGPoint target3 = CGPointMake(self.ghost3.center.x, self.ghost3.center.y-284);
    CABasicAnimation *bounce3 = [CABasicAnimation animationWithKeyPath:@"position.y"];
    bounce3.fromValue = [NSNumber numberWithInt:origin3.y];
    bounce3.toValue = [NSNumber numberWithInt:target3.y];
    bounce3.duration = 2;
    bounce3.repeatCount = HUGE_VALF;
    bounce3.autoreverses = YES;
    [self.ghost3.layer addAnimation:bounce3 forKey:@"position"];
    
    
    // Movement of pacman
    self.lastUpdateTime = [[NSDate alloc] init];
    
    self.currentPoint  = CGPointMake(0, 144);
    self.motionManager = [[CMMotionManager alloc]  init];
    self.queue         = [[NSOperationQueue alloc] init];
    
    self.motionManager.accelerometerUpdateInterval = kUpdateInterval;
    
    [self.motionManager startAccelerometerUpdatesToQueue:self.queue withHandler:
     ^(CMAccelerometerData *accelerometerData, NSError *error) {
         [(id) self setAcceleration:accelerometerData.acceleration];
         [self performSelectorOnMainThread:@selector(update) withObject:nil waitUntilDone:NO];
     }];
}


- (void)movePacman {
    
    // Save previous position
    self.previousPoint = self.currentPoint;

    // Move pacman to its new position
    CGRect frame = self.pacman.frame;
    frame.origin.x = self.currentPoint.x;
    frame.origin.y = self.currentPoint.y;
    
    self.pacman.frame = frame;
    
    // Rotate the sprite
    CGFloat newAngle = (self.pacmanXVelocity + self.pacmanYVelocity) * M_PI * 4;
    self.angle += newAngle * kUpdateInterval;
    
    CABasicAnimation *rotate;
    rotate                     = [CABasicAnimation animationWithKeyPath:@"transform.rotation"];
    rotate.fromValue           = [NSNumber numberWithFloat:0];
    rotate.toValue             = [NSNumber numberWithFloat:self.angle];
    rotate.duration            = kUpdateInterval;
    rotate.repeatCount         = 1;
    rotate.removedOnCompletion = NO;
    rotate.fillMode            = kCAFillModeForwards;
    [self.pacman.layer addAnimation:rotate forKey:@"10"];
    
}

- (void)update {
    
    NSTimeInterval secondsSinceLastDraw = -([self.lastUpdateTime timeIntervalSinceNow]);
    
    self.pacmanYVelocity = self.pacmanYVelocity - (self.acceleration.x * secondsSinceLastDraw);
    self.pacmanXVelocity = self.pacmanXVelocity - (self.acceleration.y * secondsSinceLastDraw);
    
    CGFloat xDelta = secondsSinceLastDraw * self.pacmanXVelocity * 500;
    CGFloat yDelta = secondsSinceLastDraw * self.pacmanYVelocity * 500;
    
    self.currentPoint = CGPointMake(self.currentPoint.x + xDelta,
                                    self.currentPoint.y + yDelta);
    
    [self movePacman];
    
    self.lastUpdateTime = [NSDate date];
    
}


@end
