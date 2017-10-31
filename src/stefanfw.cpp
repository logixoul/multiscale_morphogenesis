#include "precompiled.h"
#include "stefanfw.h"

float mouseX, mouseY;
bool keys[256];
bool keys2[256];
bool mouseDown_[3];

namespace stefanfw {

	EventHandler eventHandler;

	void beginFrame() {
		ci::app::AppBasic* app = ci::app::AppBasic::get();
		::mouseX = app->getMousePos().x / (float)app->getWindowWidth();
		::mouseY = app->getMousePos().y / (float)app->getWindowHeight();

		sw::beginFrame();

		wsx = app->getWindowWidth();
		wsy = app->getWindowHeight();
	}

	void endFrame() {
		sw::endFrame();
		cfg1::print();
		std::cout << std::flush;
	}

	bool EventHandler::keyDown(KeyEvent e) {
		keys[e.getChar()] = true;
		if(e.isControlDown()&&e.getCode()!=KeyEvent::KEY_LCTRL)
		{
			keys2[e.getChar()] = !keys2[e.getChar()];
			return true;
		}
		return true;
	}

	bool EventHandler::keyUp(KeyEvent e) {
		keys[e.getChar()] = false;
		return true;
	}

	bool EventHandler::mouseDown(MouseEvent e)
	{
		mouseDown_[e.isLeft() ? 0 : e.isMiddle() ? 1 : 2] = true;
		return true;
	}
	bool EventHandler::mouseUp(MouseEvent e)
	{
		mouseDown_[e.isLeft() ? 0 : e.isMiddle() ? 1 : 2] = false;
		return true;
	}

	void EventHandler::subscribeToEvents(ci::app::App& app) {
		app.registerKeyDown(this, &EventHandler::keyDown);
		app.registerKeyUp(this, &EventHandler::keyUp);
		app.registerMouseDown(this, &EventHandler::mouseDown);
		app.registerMouseUp(this, &EventHandler::mouseUp);
	}

} // namespace stefanfw