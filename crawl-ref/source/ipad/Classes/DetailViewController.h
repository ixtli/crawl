//
//  DetailViewController.h
//  ipad
//
//  Created by Ixtli on 5/15/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreData/CoreData.h>

@class RootViewController;
@class DCSSController;

@interface DetailViewController : UIViewController <UIPopoverControllerDelegate, UISplitViewControllerDelegate> {
    
	UIPopoverController *popoverController;
	UIToolbar *toolbar;

	NSManagedObject *detailItem;
	UILabel *detailDescriptionLabel;

	RootViewController *rootViewController;
	
	DCSSController *dcss;
}

@property (nonatomic, retain) DCSSController *dcss;

@property (nonatomic, retain) IBOutlet UIToolbar *toolbar;

@property (nonatomic, retain) NSManagedObject *detailItem;
@property (nonatomic, retain) IBOutlet UILabel *detailDescriptionLabel;

@property (nonatomic, assign) IBOutlet RootViewController *rootViewController;

- (IBAction)insertNewObject:(id)sender;
- (void)crawlMainDidExit:(id)arg;

@end
