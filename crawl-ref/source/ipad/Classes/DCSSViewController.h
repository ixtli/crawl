//
//  DCSSViewController.h
//  ipad
//
//  Created by Ixtli on 5/19/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIkit.h>

@class EAGLView;
@class DCSSController;

@interface DCSSViewController : UIViewController {
@private
	EAGLView *glView;
	DCSSController *dcss;
}

@property (nonatomic, retain) DCSSController *dcss;

- (id)initWithGameState: (DCSSController *)state;

- (void)startAnimating;
- (void)stopAnimating;

@end
