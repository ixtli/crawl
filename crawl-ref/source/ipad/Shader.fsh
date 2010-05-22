//
//  Shader.fsh
//  testGLESproject
//
//  Created by Ixtli on 5/22/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

varying lowp vec4 colorVarying;

void main()
{
    gl_FragColor = colorVarying;
}
