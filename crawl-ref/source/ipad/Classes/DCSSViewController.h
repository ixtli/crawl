//
//  DCSSViewController.h
//  ipad
//
//  Created by Ixtli on 5/19/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@class DCSSController;

@interface DCSSViewController : UIViewController {
	DCSSController *dcss;
}

@property (nonatomic, retain) DCSSController *dcss;

- (void)startAnimating;
- (void)stopAnimating;

@end
