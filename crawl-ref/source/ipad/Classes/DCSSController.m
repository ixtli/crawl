//
//  DCSSController.m
//  ipad
//
//  Created by Ixtli on 5/17/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "DetailViewController.h"
#import "DCSSController.h"
#import "DCSSViewController.h"

#include "main.h"

// Currently active DCSS controller.
// This is a bit of a hack to simplify telling *-ctouch.mm modules what object
// they should be messaging without having to change a lot of crawl-ref/master
DCSSController *current = nil;

@interface DCSSController (PrivateMethods)
- (void)startGame;
@end

@implementation DCSSController

@synthesize view, gameThread, vinfo, detailView;

#pragma mark -
#pragma mark Set and get currently active DCSSController

+ (DCSSController *)currentlyActiveController
{
	return current;
}

- (BOOL)makeCurrentlyActive
{
	if (!current)
	{
		current = self;
		return TRUE;
	}
	
	return FALSE;
}

- (void)resignCurrentlyActive
{
	if (current == self)
		current = nil;
}

#pragma mark -
#pragma mark Starting Crawl

- (int)launchGameThread
{
	// For now, only allow one DCSS thread to exist at a time
	if (current != self)
	{
		NSLog(@"Attempted to start a DCSS thread without being current.");
		return -1;
	}
	
	if ([gameThread isExecuting])
	{
		NSLog(@"Attempted to start a second instance of DCSS.");
		return -1;
	}
	
	// Set us as the currently active controller
	
	gameThread = [[NSThread alloc] initWithTarget:self selector:@selector(startGame) object:nil];
	[gameThread start];

	// TODO: Return an error value or something
	return 0;
}

- (void)startGame
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	// Start up
	[view startAnimating];
	
	// Run the game
	char *a = "ipad";
	int returnValue = crawl_main(1, &a);
	NSLog(@"crawl_main returned status: %i", returnValue);
	
	// Clean up
	[view stopAnimating];
	// TODO: This should really be called on the main thread.
	[detailView crawlMainDidExit:(id)returnValue];
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
#pragma mark Crawl main thread interface

- (int)crawlStartup
{
	NSLog(@"CTWrapper initializing.");
	
	// Populate the vinfo struct
	vinfo.current_w = 768;
	vinfo.current_h = 1024;
	
	return TRUE;
}

- (void)crawlShutdown
{
	NSLog(@"CTWrapper destructor called.");
}

#pragma mark -
#pragma mark Memory Management

- (id)init
{
	if (self = [super init])
	{
		view = [[DCSSViewController alloc] init];
		view.dcss = self;
	}
	
	return self;
}

- (void)dealloc
{
	[view release];
	[gameThread release];
	[detailView release];
	
	[super dealloc];
}

@end
