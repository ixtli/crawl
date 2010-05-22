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

- (id)initWithGameState: (DCSSController *)state
{
	if (self = [super initWithNibName:@"GameView" bundle:nil])
	{
		dcss = state;
		glView = [[EAGLView alloc] init];
		self.view = (UIView *)glView;
	}
	
	return self;
}

- (id)init
{
	return [self initWithGameState:nil];
}

#pragma mark -
#pragma mark Graphics State Query and Manipulation

- (void)startAnimating
{
	[glView startAnimation];
}

- (void)stopAnimating
{
	[glView stopAnimation];
}

#pragma mark -
#pragma mark Memory Management

- (void)dealloc
{
	[dcss release];
	[glView release];
	[super dealloc];
}

@end
