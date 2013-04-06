//
//  MyLocationViewController.m
//  MyLocationMap
//
//  Created by Luo Long on 13-4-6.
//  Copyright (c) 2013年 Long Luo. All rights reserved.
//

#import "MyLocationViewController.h"

/*
@interface MyLocationViewController ()

@end

@implementation MyLocationViewController

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

@end
*/

@interface MyLocationViewController ()

@end

@implementation MyLocationViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.mapView.delegate = self;
}


- (void)mapView:(MKMapView *)mapView didUpdateUserLocation:(MKUserLocation *)userLocation
{
    MKCoordinateRegion region = MKCoordinateRegionMakeWithDistance(userLocation.coordinate, 800, 800);
    [self.mapView setRegion:[self.mapView regionThatFits:region] animated:YES];
    
    // Add an annotation
    MKPointAnnotation *point = [[MKPointAnnotation alloc] init];
    point.coordinate = userLocation.coordinate;
    point.title = @"Where am I?";
    point.subtitle = @"I'm here!!!";
    
    [self.mapView addAnnotation:point];
}



@synthesize mapView;

@end
