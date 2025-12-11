#include <gui/screen1_screen/Screen1View.hpp>

Screen1View::Screen1View()
{

}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleKeyEvent(uint8_t key)
{
	// application().gotoScreen2ScreenSlideTransitionEast();

	Unicode::snprintf(textArea1Buffer, TEXTAREA1_SIZE, "%u", key);
	textArea1.invalidate();

	application().gotoScreen2ScreenWipeTransitionEast();
}

