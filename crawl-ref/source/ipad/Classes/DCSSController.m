//
//  DCSSController.m
//  ipad
//
//  Created by Ixtli on 5/17/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "DCSSController.h"

#include "main.h"

@implementation DCSSController

- (int)launchGameThread
{
	[NSThread detachNewThreadSelector:@selector(startGame) toTarget:self withObject:nil];
	return 0;
}

- (int)startGame
{
	char *a = "ipad";
	return (crawl_main(1, &a));
}

@end
