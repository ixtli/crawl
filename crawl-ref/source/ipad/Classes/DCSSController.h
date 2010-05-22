//
//  DCSSController.h
//  ipad
//
//  Created by Ixtli on 5/17/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@class EAGLView;
@class DCSSViewController;

@interface DCSSController : NSObject {
@private
	// Control our window
	DCSSViewController *view;
	NSThread *gameThread;
}

@property (readonly, nonatomic, retain) NSThread *gameThread;
@property (nonatomic, retain) DCSSViewController *view;

- (int)launchGameThread;

- (void)halt;
- (void)applicationHasLowMemory;

@end
