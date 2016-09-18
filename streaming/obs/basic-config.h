#ifndef __basic_config_h__
#define __basic_config_h__
#include <util/util.hpp>
#include <graphics/graphics.h>

class BasicConfig
{
public:
	struct MonitorInfo {
		int32_t  x, y;
		uint32_t cx, cy;

		inline MonitorInfo() {}
		inline MonitorInfo(int32_t x, int32_t y, uint32_t cx, uint32_t cy)
			: x(x), y(y), cx(cx), cy(cy)
		{}
	};

public:	
	static void InitPrimitives();
	static bool MakeUserDirs();
	static bool MakeUserProfileDirs();

	static bool InitBasicConfigDefaults(const ConfigFile& basicConfig);

private:
	static gs_vertbuffer_t *box;
	static gs_vertbuffer_t *boxLeft;
	static gs_vertbuffer_t *boxTop;
	static gs_vertbuffer_t *boxRight;
	static gs_vertbuffer_t *boxBottom;
	static gs_vertbuffer_t *circle;
};

#endif



