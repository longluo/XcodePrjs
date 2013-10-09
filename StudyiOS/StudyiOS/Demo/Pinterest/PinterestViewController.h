//
//  PinterestViewController.h
//  StudyiOS
//
//  Created by ZhangYiCheng on 13-1-26.
//  Copyright (c) 2013年 ZhangYiCheng. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PSCollectionView.h"

@interface PinterestViewController : BaseViewController <PSCollectionViewDelegate, PSCollectionViewDataSource>

@property (nonatomic, strong) NSMutableArray *items;
@property (nonatomic, strong) PSCollectionView *collectionView;

@end
