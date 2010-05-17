//
//  ESRenderer.h
//  ipad
//
//  Created by Ixtli on 5/17/10.
//

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>

@class DCSSController;

@protocol ESRenderer <NSObject>

@property (nonatomic, retain) DCSSController *dcss;

- (void)render;
- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer;

@end
