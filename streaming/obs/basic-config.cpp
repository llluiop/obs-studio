#include "basic-config.h"
#include <obs.h>
#include <util/config-file.h>
#include <obs-config.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <string>
#include <windows.h>
#include <memory>
#include <shlobj.h>
#include <vector>

#define SIMPLE_ENCODER_X264 "x264"


gs_vertbuffer_t* BasicConfig::box = nullptr;
gs_vertbuffer_t* BasicConfig::boxLeft = nullptr;
gs_vertbuffer_t *BasicConfig::boxTop = nullptr;
gs_vertbuffer_t *BasicConfig::boxRight = nullptr;
gs_vertbuffer_t *BasicConfig::boxBottom = nullptr;
gs_vertbuffer_t *BasicConfig::circle = nullptr;

void BasicConfig::InitPrimitives()
{
	obs_enter_graphics();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(0.0f, 0.0f);
	box = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	boxLeft = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(1.0f, 0.0f);
	boxTop = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
	boxRight = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f);
	boxBottom = gs_render_save();

	gs_render_start(true);
	for (int i = 0; i <= 360; i += (360 / 20)) {
		float pos = RAD(float(i));
		gs_vertex2f(cosf(pos), sinf(pos));
	}
	circle = gs_render_save();

	obs_leave_graphics();
}


static bool do_mkdir(const char *path)
{
	if (os_mkdirs(path) == MKDIR_ERROR) {
		return false;
	}

	return true;
}

bool BasicConfig::MakeUserDirs()
{
	char path[512];

	if (os_get_config_path(path, sizeof(path), "obs-studio/basic") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	if (os_get_config_path(path, sizeof(path), "obs-studio/logs") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	if (os_get_config_path(path, sizeof(path), "obs-studio/profiler_data") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

#ifdef _WIN32
	if (os_get_config_path(path, sizeof(path), "obs-studio/crashes") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;
#endif
	if (os_get_config_path(path, sizeof(path), "obs-studio/plugin_config") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	return true;
}

std::string GetDefaultVideoSavePath()
{
	wchar_t path_utf16[MAX_PATH];
	char    path_utf8[MAX_PATH] = {};

	SHGetFolderPathW(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT,
		path_utf16);

	os_wcs_to_utf8(path_utf16, wcslen(path_utf16), path_utf8, MAX_PATH);
	return std::string(path_utf8);
}

static const double scaled_vals[] =
{
	1.0,
	1.25,
	(1.0 / 0.75),
	1.5,
	(1.0 / 0.6),
	1.75,
	2.0,
	2.25,
	2.5,
	2.75,
	3.0,
	0.0
};

static BOOL CALLBACK OBSMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
	LPRECT rect, LPARAM param)
{
	std::vector<BasicConfig::MonitorInfo> &monitors = *(std::vector<BasicConfig::MonitorInfo> *)param;

	monitors.emplace_back(
		rect->left,
		rect->top,
		rect->right - rect->left,
		rect->bottom - rect->top);

	UNUSED_PARAMETER(hMonitor);
	UNUSED_PARAMETER(hdcMonitor);
	return true;
}


void GetMonitors(std::vector<BasicConfig::MonitorInfo> &monitors)
{
	monitors.clear();
	EnumDisplayMonitors(NULL, NULL, OBSMonitorEnumProc, (LPARAM)&monitors);
}

bool BasicConfig::InitBasicConfigDefaults(const ConfigFile& basicConfig)
{
	std::vector<MonitorInfo> monitors;
	GetMonitors(monitors);

	if (!monitors.size()) {
		//OBSErrorBox(NULL, "There appears to be no monitors.  Er, this "
		//	"technically shouldn't be possible.");
		return false;
	}

	uint32_t cx = monitors[0].cx;
	uint32_t cy = monitors[0].cy;

	config_set_default_string(basicConfig, "Output", "Mode", "Simple");

	config_set_default_string(basicConfig, "SimpleOutput", "FilePath",
		GetDefaultVideoSavePath().c_str());
	config_set_default_string(basicConfig, "SimpleOutput", "RecFormat",
		"flv");
	config_set_default_uint(basicConfig, "SimpleOutput", "VBitrate",
		2500);
	config_set_default_string(basicConfig, "SimpleOutput", "StreamEncoder",
		SIMPLE_ENCODER_X264);
	config_set_default_uint(basicConfig, "SimpleOutput", "ABitrate", 160);
	config_set_default_bool(basicConfig, "SimpleOutput", "UseAdvanced",
		false);
	config_set_default_bool(basicConfig, "SimpleOutput", "EnforceBitrate",
		true);
	config_set_default_string(basicConfig, "SimpleOutput", "Preset",
		"veryfast");
	config_set_default_string(basicConfig, "SimpleOutput", "RecQuality",
		"Stream");
	config_set_default_string(basicConfig, "SimpleOutput", "RecEncoder",
		SIMPLE_ENCODER_X264);

	config_set_default_bool(basicConfig, "AdvOut", "ApplyServiceSettings",
		true);
	config_set_default_bool(basicConfig, "AdvOut", "UseRescale", false);
	config_set_default_uint(basicConfig, "AdvOut", "TrackIndex", 1);
	config_set_default_string(basicConfig, "AdvOut", "Encoder", "obs_x264");

	config_set_default_string(basicConfig, "AdvOut", "RecType", "Standard");

	config_set_default_string(basicConfig, "AdvOut", "RecFilePath",
		GetDefaultVideoSavePath().c_str());
	config_set_default_string(basicConfig, "AdvOut", "RecFormat", "flv");
	config_set_default_bool(basicConfig, "AdvOut", "RecUseRescale",
		false);
	config_set_default_uint(basicConfig, "AdvOut", "RecTracks", (1 << 0));
	config_set_default_string(basicConfig, "AdvOut", "RecEncoder",
		"none");

	config_set_default_bool(basicConfig, "AdvOut", "FFOutputToFile",
		true);
	config_set_default_string(basicConfig, "AdvOut", "FFFilePath",
		GetDefaultVideoSavePath().c_str());
	config_set_default_string(basicConfig, "AdvOut", "FFExtension", "mp4");
	config_set_default_uint(basicConfig, "AdvOut", "FFVBitrate", 2500);
	config_set_default_bool(basicConfig, "AdvOut", "FFUseRescale",
		false);
	config_set_default_uint(basicConfig, "AdvOut", "FFABitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "FFAudioTrack", 1);

	config_set_default_uint(basicConfig, "AdvOut", "Track1Bitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "Track2Bitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "Track3Bitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "Track4Bitrate", 160);

	config_set_default_uint(basicConfig, "Video", "BaseCX", cx);
	config_set_default_uint(basicConfig, "Video", "BaseCY", cy);

	config_set_default_string(basicConfig, "Output", "FilenameFormatting",
		"%CCYY-%MM-%DD %hh-%mm-%ss");

	config_set_default_bool(basicConfig, "Output", "DelayEnable", false);
	config_set_default_uint(basicConfig, "Output", "DelaySec", 20);
	config_set_default_bool(basicConfig, "Output", "DelayPreserve", true);

	config_set_default_bool(basicConfig, "Output", "Reconnect", true);
	config_set_default_uint(basicConfig, "Output", "RetryDelay", 10);
	config_set_default_uint(basicConfig, "Output", "MaxRetries", 20);

	int i = 0;
	uint32_t scale_cx = 1024;
	uint32_t scale_cy = cy;

	/* use a default scaled resolution that has a pixel count no higher
	* than 1280x720 */
	while (((scale_cx * scale_cy) > (1280 * 720)) && scaled_vals[i] > 0.0) {
		double scale = scaled_vals[i++];
		scale_cx = uint32_t(double(cx) / scale);
		scale_cy = uint32_t(double(cy) / scale);
	}

	config_set_default_uint(basicConfig, "Video", "OutputCX", scale_cx);
	config_set_default_uint(basicConfig, "Video", "OutputCY", scale_cy);

	config_set_default_uint(basicConfig, "Video", "FPSType", 0);
	config_set_default_string(basicConfig, "Video", "FPSCommon", "30");
	config_set_default_uint(basicConfig, "Video", "FPSInt", 30);
	config_set_default_uint(basicConfig, "Video", "FPSNum", 30);
	config_set_default_uint(basicConfig, "Video", "FPSDen", 1);
	config_set_default_string(basicConfig, "Video", "ScaleType", "bicubic");
	config_set_default_string(basicConfig, "Video", "ColorFormat", "NV12");
	config_set_default_string(basicConfig, "Video", "ColorSpace", "601");
	config_set_default_string(basicConfig, "Video", "ColorRange",
		"Partial");

	config_set_default_uint(basicConfig, "Audio", "SampleRate", 44100);
	config_set_default_string(basicConfig, "Audio", "ChannelSetup",
		"Stereo");

	return true;
}
