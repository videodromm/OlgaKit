#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"
#include "cinder/Utilities.h"

#include "OlgaKitTriangle.h"

// UserInterface
#include "CinderImGui.h"
// Settings
#include "VDSettings.h"
// Session
#include "VDSession.h"
// Log
#include "VDLog.h"
// UI
#include "VDUI.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace VideoDromm;

#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))

class OlgaApp : public App {

public:
	void setup() override;
	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void setUIVisibility(bool visible);
private:
	// Settings
	VDSettingsRef				mVDSettings;
	// Session
	VDSessionRef				mVDSession;
	// Log
	VDLogRef					mVDLog;
	// UI
	VDUIRef						mVDUI;
	// handle resizing for imgui
	void						resizeWindow();
	bool						mIsResizing;
	// imgui
	float						color[4];
	float						backcolor[4];
	int							playheadPositions[12];
	int							speeds[12];

	float						f = 0.0f;
	char						buf[64];
	unsigned int				i, j;

	bool						mouseGlobal;

	string						mError;
	// fbo
	bool						mIsShutDown;
	Anim<float>					mRenderWindowTimer;
	void						positionRenderWindow();
	bool						mFadeInDelay;
	// o
	void	updateMirrors(vector<OlgaKitTriangle> *vec);
	void	drawMirrors(vector<OlgaKitTriangle> *vec);
	void	defineMirrorGrid();

	void	resetSample();

	gl::TextureRef				mTex;				// the loaded texture

	vector<OlgaKitTriangle>		mTriPieces;				// stores alll of the kaleidoscope mirror pieces
	Anim<vec2>					mSamplePt;				// location of the piece of the image that is being sampled for the kaleidoscope
	float						mSampleSize;			// Size of the image sample to grab for the kaleidoscope		   
	Anim<float>					mMirrorRot;				// rotation of the canvas in kaleidoscope mode
// new
	ci::vec2		mStartPt, mVertices[3], mTexVertices[3];
	gl::GlslProgRef					mGlsl;
};

void OlgaApp::setup()
{
	// Settings
	mVDSettings = VDSettings::create();
	// Session
	mVDSession = VDSession::create(mVDSettings);
	//mVDSettings->mCursorVisible = true;
	setUIVisibility(mVDSettings->mCursorVisible);
	mVDSession->getWindowsResolution();
	mVDUI = VDUI::create(mVDSettings, mVDSession);

	mouseGlobal = false;
	mFadeInDelay = true;
	// windows
	mIsShutDown = false;
	mIsResizing = true;
	mRenderWindowTimer = 0.0f;
	timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });

	mTex = gl::Texture::create(loadImage(loadAsset("0.jpg")));
	defineMirrorGrid();				// redefine the kaleidoscope grid

	// new
	mGlsl = gl::GlslProg::create(loadAsset("passthru.vert"), loadAsset("live.frag"));
	const int r = 400;

	vec2 pt1(0.0, 0.0);
	vec2 pt2(r, 0.0);
	vec2 pt3((cos(M_PI / 3) * r), (sin(M_PI / 3) * r));

	mVertices[0] = pt1;
	mVertices[1] = pt2;
	mVertices[2] = pt3;
	mTexVertices[0] = pt1;
	mTexVertices[1] = pt2;
	mTexVertices[2] = pt3;

}


// Creates the grid of kaleidoscope mirrored triangles
void OlgaApp::defineMirrorGrid()
{
	const int r = 1;
	const float tri_scale = (float)randInt(80, 100);

	// delete any previous pieces and clear the old vector
	mTriPieces.clear();

	vec2 pt1(0.0, 0.0);
	vec2 pt2(r, 0.0);
	vec2 pt3((cos(M_PI / 2.5) * r), (sin(M_PI / 2.5) * r));

	const float tri_width = distance(pt1, pt2) * tri_scale;
	const float tri_height = std::sqrt((tri_width*tri_width) - ((tri_width / 2) * (tri_width / 2)));

	const int amtX = 1;
	const float w = ((amtX*1.5) + .5) * tri_width;
	const float xOffset = -(w - getWindowWidth()) / 2;

	const int amtY = 1;
	const float yOffset = -((amtY*(tri_height)-getWindowHeight()) / 2);

	// creates a series of hexagons composed of 6 triangles each
	for (int i = 0; i < amtX; i++) {
		float startX = ((tri_width) * 1.5 * i);
		startX += xOffset;
		for (int j = 0; j < amtY; j++) {
			float startY = (i % 2 == 0) ? (tri_height * 2 * j) - (tri_height) : tri_height * 2 * j;
			startY += yOffset;

			for (int k = 0; k < 4; k++) {
				// because every other pieces is a mirror of the one next to it, every other has to be reversed on the x scale
				int scaleX = (k % 2 == 0) ? 1 : -1;
				vec2 scale( tri_scale, tri_scale);

				vec2 start(startX, startY);

				OlgaKitTriangle tri = OlgaKitTriangle(vec2(startX, startY), pt1, pt2, pt3, M_PI / 2.5 * k, scale);
				mTriPieces.push_back(tri);
			}
		}
	}
}

void OlgaApp::resetSample()
{
	// reset sample pos
	mSampleSize = randInt(100, 300);
	mSamplePt.value().y = randFloat(0, getWindowWidth() - mSampleSize);
	mSamplePt.value().x = randFloat(0, getWindowHeight() - mSampleSize);

	vec2 newPos;
	int count = 0;
	// Try to find a good sample location thats within the window's frame.
	// Give up if we try and settle after a bunch of times, no big deal.
	do {
		newPos.x = randFloat(0, getWindowWidth() - mSampleSize / 2);
		newPos.y = randFloat(0, getWindowHeight() - mSampleSize / 2);
		count++;
	} while (count < 15 && ((mSamplePt.value().x - newPos.x) < 100 || (mSamplePt.value().y - newPos.y) < 100));
}

void OlgaApp::positionRenderWindow() {
	mVDSettings->iResolution.x = mVDSettings->mRenderWidth;
	mVDSettings->iResolution.y = mVDSettings->mRenderHeight;
	mVDSettings->mRenderPosXY = ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY);
	setWindowPos(mVDSettings->mRenderX, mVDSettings->mRenderY);
	setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
}
void OlgaApp::setUIVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void OlgaApp::fileDrop(FileDropEvent event)
{
	mVDSession->fileDrop(event);
}
void OlgaApp::update()
{
	mVDSession->setControlValue(mVDSettings->IFPS, getAverageFps());
	mVDSession->update();

	resetSample(); 
	updateMirrors(&mTriPieces);

}
void OlgaApp::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		ui::disconnectWindow(getWindow());
		ui::Shutdown();
		// save settings
		mVDSettings->save();
		mVDSession->save();
		quit();
	}
}
void OlgaApp::mouseMove(MouseEvent event)
{
	if (!mVDSession->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
	}
}
void OlgaApp::mouseDown(MouseEvent event)
{
	if (!mVDSession->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
	}
}
void OlgaApp::mouseDrag(MouseEvent event)
{
	if (!mVDSession->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
	}
}
void OlgaApp::mouseUp(MouseEvent event)
{
	if (!mVDSession->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
	}
}

void OlgaApp::keyDown(KeyEvent event)
{
	if (!mVDSession->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_ESCAPE:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_h:
			// mouse cursor and ui visibility
			mVDSettings->mCursorVisible = !mVDSettings->mCursorVisible;
			setUIVisibility(mVDSettings->mCursorVisible);
			break;
		}
	}
}
void OlgaApp::keyUp(KeyEvent event)
{
	if (!mVDSession->handleKeyUp(event)) {
	}
}
void OlgaApp::resizeWindow()
{
	mVDUI->resize();
	mVDSession->resize();
}

void OlgaApp::updateMirrors(vector<OlgaKitTriangle> *vec)
{
	vec2 mSamplePt1(-0.5, -(sin(M_PI / 3) / 3));
	vec2 mSamplePt2(mSamplePt1.x + 1, mSamplePt1.y);
	vec2 mSamplePt3(mSamplePt1.x + (cos(M_PI / 3)), mSamplePt1.y + (sin(M_PI / 3)));

	mat3 mtrx(1.0f);
	mtrx = glm::translate(mtrx, mSamplePt.value());
	mtrx = glm::scale(mtrx, vec2(mSampleSize));
	mtrx = glm::rotate(mtrx, float((getElapsedFrames() * 4) / 2 * M_PI));

	mSamplePt1 = vec2(mtrx * vec3(mSamplePt1, 1.0));
	mSamplePt2 = vec2(mtrx * vec3(mSamplePt2, 1.0));
	mSamplePt3 = vec2(mtrx * vec3(mSamplePt3, 1.0));

	mSamplePt1 /= mTex->getSize();
	mSamplePt2 /= mTex->getSize();
	mSamplePt3 /= mTex->getSize();

	// loop through all the pieces and pass along the current texture and it's coordinates
	for (int i = 0; i < vec->size(); i++) {
		(*vec)[i].update(mTex, mSamplePt1, mSamplePt2, mSamplePt3);
	}
}


void OlgaApp::draw()
{
	gl::clear(Color::black());
	if (mFadeInDelay) {
		mVDSettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mVDSession->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}
	//gl::setMatricesWindow(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, false);
	//gl::draw(mVDSession->getMixTexture(), getWindowBounds());
	getWindow()->setTitle(mVDSettings->sFps + " fps Videodromm Olga");
	//gl::enableAlphaBlending(PREMULT);
	gl::ScopedModelMatrix scopedMat;
	//gl::translate(getWindowCenter());

	gl::color(1.0f, 0.0f, 0.5f, 1.0);
	drawMirrors(&mTriPieces);
	// draw the texture to the triangle
	//gl::ScopedGlslProg glslScp(gl::getStockShader(gl::ShaderDef().color().texture()));
	gl::ScopedGlslProg glslScp(mGlsl);
	gl::ScopedTextureBind texScp(mTex);
	mGlsl->uniform("iGlobalTime", (float)getElapsedSeconds());
	mGlsl->uniform("iResolution", vec3(mVDSettings->mFboWidth, mVDSettings->mFboHeight, 1.0));

	gl::drawSolidTriangle(mVertices, mTexVertices);

	// imgui
	if (!mVDSettings->mCursorVisible) return;

	mVDUI->Run("UI", (int)getAverageFps());
	if (mVDUI->isReady()) {
	}
}
void OlgaApp::drawMirrors(vector<OlgaKitTriangle> *vec)
{
	gl::ScopedModelMatrix scopedMat;
	gl::translate(getWindowCenter());
	for (int i = 0; i < vec->size(); i++) {
		(*vec)[i].draw();
	}
}
CINDER_APP(OlgaApp, RendererGl)
