//
//  DCSSViewController.m
//  ipad
//
//  Created by Ixtli on 5/19/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "DCSSViewController.h"
#import "EAGLView.h"
#import "DCSSController.h"

@implementation DCSSViewController

@synthesize dcss;

#pragma mark -
#pragma mark Graphics State Query and Manipulation

- (void)startAnimating
{
	[(EAGLView *)self.view startAnimation];
}

- (void)stopAnimating
{
	[(EAGLView *)self.view stopAnimation];
}

#pragma mark -
#pragma mark Memory Management

- (void)dealloc
{
	[dcss release];
	[super dealloc];
}

@end
