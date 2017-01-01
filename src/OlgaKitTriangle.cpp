//
//	OlgaKitTriangle.cpp
//	Instascope
//
//	Created by Greg Kepler on 5/30/12.
//	Copyright (c) 2012 The Barbarian Group. All rights reserved.
//

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Shader.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"
#include "OlgaKitTriangle.h"

using namespace ci;
using namespace ci::app;
using namespace std;


OlgaKitTriangle::OlgaKitTriangle(vec2 _startPt, vec2 _pt1, vec2 _pt2, vec2 _pt3, float _rotation, vec2 _scale)
{
	mStartPt = _startPt;
	mVertices[0] = _pt1;
	mVertices[1] = _pt2;
	mVertices[2] = _pt3;
	mRotation = _rotation;
	mScale = _scale;

	mReadyToDraw = false;
}

void OlgaKitTriangle::update(gl::TextureRef tex, vec2 pt1, vec2 pt2, vec2 pt3)
{
	mTexVertices[0] = pt1;
	mTexVertices[1] = pt2;
	mTexVertices[2] = pt3;
	mDrawTex = tex;
	mReadyToDraw = true;
}

void OlgaKitTriangle::draw()
{
	if (!mReadyToDraw) return;

	gl::ScopedModelMatrix scopedMat;
	gl::ScopedColor scopedCol;
	gl::translate(mStartPt);	// move to the start point
	gl::scale(mScale);		// scale the triangle
	gl::rotate(mRotation);	// rotate on the Z axis

	gl::color(1.0f, 1.0f, 1.0f, 1.0f);

	// draw the texture to the triangle
	gl::ScopedGlslProg glslScp(gl::getStockShader(gl::ShaderDef().color().texture()));
	gl::ScopedTextureBind texScp(mDrawTex);
	gl::drawSolidTriangle(mVertices, mTexVertices);
}
