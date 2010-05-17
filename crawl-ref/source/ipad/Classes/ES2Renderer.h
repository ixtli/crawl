//
//  ES2Renderer.h
//  ipad
//
//  Created by Ixtli on 5/17/10.
//

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#import "ESRenderer.h"

@class DCSSController;

@interface ES2Renderer : NSObject <ESRenderer> {
@private
	EAGLContext *context;
	
	// Pixel dimensions of associated CAEAGLLayer
	GLint backingWidth;
	GLint backingHeight;
	
	GLfloat tileWidth, tileHeight;
	
	// OpenGL ES names for the framebuffer and renderbuffer used to
	// render this view.
	GLuint defaultFramebuffer, colorRenderbuffer;
	GLuint program;
	
	DCSSController *dcss;
}

@property (nonatomic, retain) DCSSController *dcss;

- (void)render;
- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer;

@end
