//
//  DCSSController.m
//  ipad
//
//  Created by Ixtli on 5/17/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "DCSSController.h"
#import "DCSSViewController.h"

#include "main.h"

@interface DCSSController (PrivateMethods)
- (void)startGame;
@end


@implementation DCSSController

@synthesize view, gameThread;

- (id)init
{
	if (self = [super init])
	{
		view = [[DCSSViewController alloc] initWithGameState:self];
	}
	
	return self;
}

#pragma mark -
#pragma mark Starting Crawl

- (int)launchGameThread
{
	if ([gameThread isExecuting])
	{
		NSLog(@"Attempted to start a second instance of DCSS.");
	}
	
	gameThread = [[NSThread alloc] initWithTarget:self selector:@selector(startGame) object:nil];
	[gameThread start];

	// TODO: Return an error value or something
	return 0;
}

- (void)startGame
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	// Run the game
	char *a = "ipad";
	int ret = crawl_main(1, &a);
	
	// Clean up
	NSLog(@"Crawl finished with code %i", ret);
	[pool release];
}

#pragma mark -
#pragma mark Application State Handlers

- (void)halt
{
	// Handle emminant application closure
	
	// Save quickly or ask for more time (4.0+)
	// In either case, signal the main thread that it must terminate immediately
	
	[view stopAnimating];
}

- (void)applicationHasLowMemory
{
	// Handle low memory environment somehow
	// TODO: Make this clean up stashes or something
}

#pragma mark -
#pragma mark Memory Management

- (void)dealloc
{
	[view release];
	[gameThread release];
	
	[super dealloc];
}

@end
