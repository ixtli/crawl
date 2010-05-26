//
//  EAGLView.h
//  ipad
//
//  Created by Ixtli on 5/17/10.
//

// Wrap the CAEAGLLayer from CoreAnimation into a subclass of UIView.

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

#import "ESRenderer.h"

@interface EAGLView : UIView {
@private
	id <ESRenderer> renderer;
	
	BOOL animating;
	NSInteger animationFrameInterval;
	
	// Use CADisplayLink to control animation timing: links to main display and
	// fire every vsync when added to the run loop.
	
	CADisplayLink *displayLink;
	
	DCSSController *dcss;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;
@property (nonatomic, retain) DCSSController *dcss;

- (void)startAnimation;
- (void)stopAnimation;
- (void)drawView:(id)sender;

- (void)textureFromImage:(CGImageRef *)img textureName:(GLuint *)texName;

@end
