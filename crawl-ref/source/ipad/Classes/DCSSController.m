//
//  DCSSController.m
//  ipad
//
//  Created by Ixtli on 5/17/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "DCSSController.h"

#include "main.h"

@interface DCSSController (PrivateMethods)
- (int)startGame;
@end


@implementation DCSSController

#pragma mark -
#pragma mark Starting Crawl

- (int)launchGameThread
{
	[NSThread detachNewThreadSelector:@selector(startGame) toTarget:self withObject:nil];
	return 0;
}

- (int)startGame
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	// Run the game
	char *a = "ipad";
	int ret = crawl_main(1, &a);
	
	// Clean up
	[pool release];
	return ret;
}

@end
