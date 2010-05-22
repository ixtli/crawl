//
//  DCSSController.h
//  ipad
//
//  Created by Ixtli on 5/17/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#include <unistd.h>

#import <UIKit/UIKit.h>

typedef struct
{
	size_t current_w, current_h;
} video_info;

@class EAGLView;
@class DCSSViewController;

@interface DCSSController : NSObject {
@private
	DCSSViewController *view;
	NSThread *gameThread;
	video_info vinfo;
}

@property (readonly, nonatomic, retain) NSThread *gameThread;
@property (nonatomic, retain) DCSSViewController *view;
@property (readonly, nonatomic) video_info vinfo;

+ (DCSSController *)currentlyActiveController;

- (int)launchGameThread;

- (void)halt;
- (void)applicationHasLowMemory;

- (int)crawlStartup;
- (void)crawlShutdown;

@end
