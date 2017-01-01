//
//	OlgaKitTriangle.h
//	Instascope
//
//	Created by Greg Kepler on 5/31/12.
//	Copyright (c) 2012 The Barbarian Group. All rights reserved.
//

#pragma once

#include "cinder/gl/Texture.h"
#include "cinder/Timeline.h"

class OlgaKitTriangle {
  public:
	OlgaKitTriangle( ci::vec2 _startPt, ci::vec2 _pt1, ci::vec2 _pt2, ci::vec2 _pt3, float _rotation, ci::vec2 _scale );

	void		update( ci::gl::TextureRef tex, ci::vec2 pt1, ci::vec2 pt2, ci::vec2 pt3 );
	void		draw();
	
  protected:

	ci::gl::TextureRef	mDrawTex;
	
	ci::vec2		mStartPt, mVertices[3], mTexVertices[3];
	
	float			mRotation;
	ci::vec2		mScale;
	
	bool			mReadyToDraw;

};