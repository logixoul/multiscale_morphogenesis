#pragma once
#if 0
#include <cinder/Area.h>
//#include <cinder/

namespace Ci
{
	class MouseEvent
	{
	public:
		MouseEvent(Button whichOne, int pressedOnes) {
			this->whichOne=whichOne;
			this->pressedOnes=pressedOnes;
		}

		bool isLeft() { return whichOne==Left; }
		bool isMiddle() { return whichOne==Middle; }
		bool isRight() { return whichOne==Right; }

		bool isLeftDown() { return pressedOnes&Left!=0x0; }
		bool isMiddleDown() { return pressedOnes&Middle!=0x0; }
		bool isRightDown() { return pressedOnes&Right!=0x0; }

	private:
		enum Button { Left = 0x1, Middle = 0x2, Right = 0x4 };
		Button whichOne;
		int pressedOnes; // flags
	};

	class AppBasic {
		virtual void setup() = 0;
		virtual void draw() = 0;
	
		virtual void mouseDown(Ci::MouseEvent e) { }
		virtual void mouseUp(Ci::MouseEvent e) { }
	};
#endif