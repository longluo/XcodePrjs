//
//  MyLocationViewController.h
//  MyLocationMap
//
//  Created by Luo Long on 13-4-6.
//  Copyright (c) 2013年 Long Luo. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MapKit/MapKit.h>


@interface MyLocationViewController : UIViewController <MKMapViewDelegate>

@property (nonatomic, strong) IBOutlet MKMapView *mapView;

@end


