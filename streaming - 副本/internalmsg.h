#pragma once

#include <string>
#include <functional>

class OBSWrapper;
struct StreamingSettting
{
	std::string loc;
	std::string bitrate;
	std::string hwnd;
	std::string key;

	std::function<void(void)> Dispatch(OBSWrapper* obs);

};