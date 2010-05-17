//
//  EAGLView.m
//  ipad
//
//  Created by Ixtli on 5/17/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "EAGLView.h"
#import "ES2Renderer.h"

@implementation EAGLView

@synthesize animating;
@synthesize dcss;
@dynamic animationFrameInterval;

+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

// When EAGL view is unarchived from the nib, it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder *)aDecoder
{
	if ((self = [super initWithCoder:aDecoder]))
	{
		// iPads start at iPhoneOS 3.2, so we assume that we'll have CADisplayLink
		// available to us (3.1+) but in case someone tries to port this somewhere
		// not originally intended, cause a scene.
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if ([currSysVer compare:reqSysVer options:NSNumericSearch] == NSOrderedAscending)
		{
			[self release];
			NSLog(@"This application requires System Version 3.1 or greater.");
			return nil;
		}
		
		// Get the layer
		CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
		
		eaglLayer.opaque = TRUE;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
										[NSNumber numberWithBool: FALSE],
										kEAGLDrawablePropertyRetainedBacking,
										kEAGLColorFormatRGBA8,
										kEAGLDrawablePropertyColorFormat, nil];
		
		renderer = [[ES2Renderer alloc] init];
		
		// This is where we might check to see if ES 2.0 is not supported, but since
		// this is a 'pad app, there is no chance of this being the case.
		
		if (!renderer)
		{
			[self release];
			NSLog(@"Could not allocate a renderer.");
			return nil;
		}
		
		animating = FALSE;
		animationFrameInterval = 1;
		displayLink = nil;
		
		self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	}
	
	return self;
}

- (void)drawView:(id)sender
{
	[renderer render];
}

- (void)layoutSubviews
{
	[renderer setDcss:dcss];
	[renderer resizeFromLayer:self.layer];
	[self drawView:nil];
}

- (NSInteger)animationFrameInterval
{
	return animationFrameInterval;
}

- (void)setAnimationFrameInterval:(NSInteger)frameInterval
{
	// Frame interval defines how many display frames must pass before display
	// link fires.  The link will only fire 30 times a second when the interval
	// is two on a display that refreshes 60 times per second.  The default will
	// fire 50 times per second when the display refreshes 60 times per second.
	
	// An interval of less than one results in underfined behavior.
	
	if (frameInterval >= 1)
	{
		animationFrameInterval = frameInterval;
		if (animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void)startAnimation
{
	if (!animating)
	{
		displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
		[displayLink setFrameInterval:animationFrameInterval];
		[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		animating = TRUE;
		NSLog(@"Animation started.");
	}
	
}

- (void)stopAnimation
{
	if (animating)
	{
		[displayLink invalidate];
		displayLink = nil;
		animating = FALSE;
		NSLog(@"Animation stopped.");
	}
}

- (void)dealloc
{
	[renderer release];
	[dcss release];
	[super dealloc];
}

@end

