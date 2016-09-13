#ifndef __video_config_h__
#define __video_config_h__
#include <util/util.hpp>

class VideoConfig
{
public:
	static int SetVideo(const ConfigFile& configFile);
	static void GetFPSCommon(uint32_t & num, uint32_t & den, const ConfigFile& configFile);
	static void GetFPSInteger(uint32_t & num, uint32_t & den, const ConfigFile& configFile);
	static void GetFPSFraction(uint32_t & num, uint32_t & den, const ConfigFile& configFile);
	static void GetFPSNanoseconds(uint32_t & num, uint32_t & den, const ConfigFile& configFile);
	static void GetConfigFPS(uint32_t & num, uint32_t & den, const ConfigFile& configFile);
	static const char * GetRenderModule();
};

#endif



