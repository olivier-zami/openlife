//
// Created by olivier on 11/10/2021.
//

#include "console.h" //TODO: rename renderer/openGL/drawConsole => ? component/graphic/console graphic/sdl/console

#include <cstdio>
#include <GL/gl.h>
#include <GL/glu.h>
//#include <GLES2/gl2.h>

void openLife::client::renderer::openGL::drawConsole()
{
	printf("\nTrying Draw Console...");


	//!prepare
	/*
	//prepareDraw( inFrame, inPosition, inScale, inLinearMagFilter,
	//			 inMipMapFilter,
	//			 inRotation, inFlipH );

	if( inComputeCornerPos )
	{
		double xLeftRadius = inScale * mBaseScaleX * mColoredRadiusLeftX;
		double xRightRadius = inScale * mBaseScaleX * mColoredRadiusRightX;
		double yTopRadius = inScale * mBaseScaleY * mColoredRadiusTopY;
		double yBottomRadius = inScale * mBaseScaleY * mColoredRadiusBottomY;

		doublePair centerOffset = mult( mCenterOffset, inScale );

		if( sCountingPixels )
	 	{
			// do this pre-flip and pre-rotation
			double increment = ( xLeftRadius + xRightRadius ) * ( yTopRadius + yBottomRadius );
			sPixelsDrawn += increment;
		}

		if( !(mFlipHorizontal) != !(inFlipH) )
	 	{
			// make sure flips don't override eachother, xor
			xLeftRadius = -xLeftRadius;
			xRightRadius = -xRightRadius;
			centerOffset.x = - centerOffset.x;
		}

		// first, set up corners relative to 0,0
		// loop is unrolled here, with all offsets added in
		// also, mZ ignored now, since rotation no longer done

		if( inRotation == 0 )
	 	{
			double posX = inPosition->mX - centerOffset.x;
			double posY = inPosition->mY + centerOffset.y;
			squareVertices[0] = posX - xLeftRadius;
			squareVertices[1] = posY - yBottomRadius;
			squareVertices[2] = posX + xRightRadius;
			squareVertices[3] = posY - yBottomRadius;
			squareVertices[4] = posX - xLeftRadius;
			squareVertices[5] = posY + yTopRadius;
			squareVertices[6] = posX + xRightRadius;
			squareVertices[7] = posY + yTopRadius;
		}
		else
	 	{
			double posX = inPosition->mX;
			double posY = inPosition->mY;
			squareVertices[0] = - xLeftRadius - centerOffset.x;
			squareVertices[1] = - yBottomRadius + centerOffset.y;
			squareVertices[2] = xRightRadius - centerOffset.x;
			squareVertices[3] = - yBottomRadius + centerOffset.y;
			squareVertices[4] = - xLeftRadius - centerOffset.x;
			squareVertices[5] = yTopRadius + centerOffset.y;
			squareVertices[6] = xRightRadius - centerOffset.x;
			squareVertices[7] = yTopRadius + centerOffset.y;
			double cosAngle = cos( - 2 * M_PI * inRotation );
			double sinAngle = sin( - 2 * M_PI * inRotation );

			for( int i=0; i<7; i+=2 )
			{
				double x = squareVertices[i];
				double y = squareVertices[i+1];
				squareVertices[i] = x * cosAngle - y * sinAngle;
				squareVertices[i+1] = x * sinAngle + y * cosAngle;
				squareVertices[i] += posX;
				squareVertices[i+1] += posY;
			}
		}
	}

	mTexture->enable();
	if( inMipMapFilter )
	{
		if( mLastSetMinFilter != 2 )
	 	{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			mLastSetMinFilter = 2;
		}
	}
	else
	{
		if( inLinearMagFilter )
	 	{
			if( mLastSetMinFilter != 1 )
			{
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
								 GL_LINEAR );
				mLastSetMinFilter = 1;
			}
		}
		else {
			if( mLastSetMinFilter != 0 ) {
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
								 GL_NEAREST );
				mLastSetMinFilter = 0;
			}
		}
	}

	if( inLinearMagFilter ) {
		if( mLastSetMagFilter != 1 ) {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			mLastSetMagFilter = 1;
		}
	}
	else {
		if( mLastSetMagFilter != 0 ) {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			mLastSetMagFilter = 0;
		}
	}

	textXA = (1.0 / mNumPages) * mCurrentPage;
	textXB = textXA + (1.0 / mNumPages );

	textXA += 0.5 - mColoredRadiusLeftX;
	textXB -= 0.5 - mColoredRadiusRightX;

	textYB = (1.0 / mNumFrames) * inFrame;
	textYA = textYB + (1.0 / mNumFrames );

	textYB += 0.5 - mColoredRadiusTopY;
	textYA -= 0.5 - mColoredRadiusBottomY;

	squareTextureCoords[0] = textXA;
	squareTextureCoords[1] = textYA;

	squareTextureCoords[2] = textXB;
	squareTextureCoords[3] = textYA;

	squareTextureCoords[4] = textXA;
	squareTextureCoords[5] = textYB;

	squareTextureCoords[6] = textXB;
	squareTextureCoords[7] = textYB;

	//!endprepare

	glVertexPointer( 2, GL_FLOAT, 0, squareVertices );


	glTexCoordPointer( 2, GL_FLOAT, 0, squareTextureCoords );

	if( !sStateSet ) {
		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		sStateSet = true;
	}
	 glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	*/



	//glEnableClientState(GL_VERTEX_ARRAY);//!TODO not par of stream
	GLfloat squareVertices[4*2];
	squareVertices[0] = 100;//x
	squareVertices[1] = 100;//y
	squareVertices[2] = 300;//x
	squareVertices[3] = 100;//y
	squareVertices[4] = 100;//x
	squareVertices[5] = 300;//y
	squareVertices[6] = 300;//x
	squareVertices[7] = 300;//y

	/*
	//GLuint vbo;
	//glGenBuffers(1, &vbo); // Generate 1 buffer in GPU
	//glBindBuffer(GL_ARRAY_BUFFER, vbo); //"activate" vbo in GPU
	//glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW); //copy vertice in GPU vbo
	//!shader building
	const char* vertexSource = R"glsl(
	#version core 150
	in vec2 position;
	void main()
	{
		gl_Position = vec4(position, 0.0, 1.0)
	}
	)glsl";
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if(status)printf("\nshader compilation is a success");
	else
	{
		char buffer[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
		printf("\nshader compilation fail : %s", buffer);
	}

	//!fragment shader building
	const char* fragmentSource = R"glsl(
	#version core 150
	out vec4 position;
	void main()
	{
		outColor = vec4(1.0, 1.0, 1.0, 1.0)
	}
	)glsl";
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	GLint status1;
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status1);
	if(status1)printf("\nshader compilation is a success");
	else
	{
		char buffer1[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, buffer1);
		printf("\nshader compilation fail : %s", buffer1);
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");//need to be call before linking program. Since there are only one output aka "outColor" this line is not necessary
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	GLint posAttrib = glGetAttribLocation(shaderProgram, "position")//"get position attrtibute of var "position"
	glVertexAttribPointer(//specie how data should be retrieved
			posAttrib,
			2,//size of vec vertexShader
			GL_FLOAT,//type of vec component (here float)*
			GL_FALSE,//GL_TRUE = should be normlized betwen -1.0 and 1.0 (float if GL_FLOAT) or 0 and 1
			0,//the "stride" or how many bytes between each position
			0); //offset
	glEnableVertexAttribArray(posAttrib);//enable vertex attribute array

	GLuint vao;// VAOs store all of the links between the attributes and your VBOs with raw vertex data. => avoid to use glUseProgram() each times
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);//As soon as you've bound a certain VAO, every time you call glVertexAttribPointer, that information will be stored in that VAO. Since only calls after binding a VAO stick to it, make sure that you've created and bound the VAO at the start of your program. Any vertex buffers and element buffers bound before it will be ignored.

	//!Use glDrawBuffers when rendering to multiple framebuffers, because only the first output will be enabled by default.
	*/


	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);//set color
	//glClear(GL_COLOR_BUFFER_BIT);//clear color

	//GLubyte smiley = 0;
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_COLOR_INDEX, GL_BITMAP, &smiley);
	glVertexPointer( 2, GL_FLOAT, 0, squareVertices );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	//glDisableClientState(GL_VERTEX_ARRAY);//!TODO not par of stream
}