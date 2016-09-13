#include "video-config.h"
#include <obs.h>
#include <util/config-file.h>
#include <obs-config.h>
#include <util/bmem.h>
#include <util/dstr.h>

#define SIMPLE_ENCODER_X264 "x264"
#define DL_OPENGL "libobs-opengl.dll"
#define DL_D3D11 "libobs-d3d11.dll"



static inline enum video_format GetVideoFormatFromName(const char *name)
{
	if (astrcmpi(name, "I420") == 0)
		return VIDEO_FORMAT_I420;
	else if (astrcmpi(name, "NV12") == 0)
		return VIDEO_FORMAT_NV12;
	else if (astrcmpi(name, "I444") == 0)
		return VIDEO_FORMAT_I444;
#if 0 //currently unsupported
	else if (astrcmpi(name, "YVYU") == 0)
		return VIDEO_FORMAT_YVYU;
	else if (astrcmpi(name, "YUY2") == 0)
		return VIDEO_FORMAT_YUY2;
	else if (astrcmpi(name, "UYVY") == 0)
		return VIDEO_FORMAT_UYVY;
#endif
	else
		return VIDEO_FORMAT_RGBA;
}

static inline int AttemptToResetVideo(struct obs_video_info *ovi)
{
	return obs_reset_video(ovi);
}

static inline enum obs_scale_type GetScaleType(const ConfigFile &basicConfig)
{
	const char *scaleTypeStr = config_get_string(basicConfig,
		"Video", "ScaleType");

	if (astrcmpi(scaleTypeStr, "bilinear") == 0)
		return OBS_SCALE_BILINEAR;
	else if (astrcmpi(scaleTypeStr, "lanczos") == 0)
		return OBS_SCALE_LANCZOS;
	else
		return OBS_SCALE_BICUBIC;
}


int VideoConfig::SetVideo(const ConfigFile& configFile)
{
	//ProfileScope("OBSBasic::ResetVideo");

	struct obs_video_info ovi;
	int ret;

	GetConfigFPS(ovi.fps_num, ovi.fps_den, configFile);

	const char *colorFormat = config_get_string(configFile, "Video",
		"ColorFormat");
	const char *colorSpace = config_get_string(configFile, "Video",
		"ColorSpace");
	const char *colorRange = config_get_string(configFile, "Video",
		"ColorRange");

	ovi.graphics_module = GetRenderModule();
	ovi.base_width = (uint32_t)config_get_uint(configFile,
		"Video", "BaseCX");
	ovi.base_height = (uint32_t)config_get_uint(configFile,
		"Video", "BaseCY");
	ovi.output_width = (uint32_t)config_get_uint(configFile,
		"Video", "OutputCX");
	ovi.output_height = (uint32_t)config_get_uint(configFile,
		"Video", "OutputCY");
	ovi.output_format = GetVideoFormatFromName(colorFormat);
	ovi.colorspace = astrcmpi(colorSpace, "601") == 0 ?
		VIDEO_CS_601 : VIDEO_CS_709;
	ovi.range = astrcmpi(colorRange, "Full") == 0 ?
		VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = GetScaleType(configFile);

	if (ovi.base_width == 0 || ovi.base_height == 0) {
		ovi.base_width = 1920;
		ovi.base_height = 1080;
		config_set_uint(configFile, "Video", "BaseCX", 1920);
		config_set_uint(configFile, "Video", "BaseCY", 1080);
	}

	if (ovi.output_width == 0 || ovi.output_height == 0) {
		ovi.output_width = ovi.base_width;
		ovi.output_height = ovi.base_height;
		config_set_uint(configFile, "Video", "OutputCX",
			ovi.base_width);
		config_set_uint(configFile, "Video", "OutputCY",
			ovi.base_height);
	}

	ret = AttemptToResetVideo(&ovi);
	if (ret != OBS_VIDEO_SUCCESS) {
		/* Try OpenGL if DirectX fails on windows */
		if (astrcmpi(ovi.graphics_module, DL_OPENGL) != 0) {
			blog(LOG_WARNING, "Failed to initialize obs video (%d) "
				"with graphics_module='%s', retrying "
				"with graphics_module='%s'",
				ret, ovi.graphics_module,
				DL_OPENGL);
			ovi.graphics_module = DL_OPENGL;
			ret = AttemptToResetVideo(&ovi);
		}
	}
	else if (ret == OBS_VIDEO_SUCCESS) {
		//ResizePreview(ovi.base_width, ovi.base_height);
		//if (program)
		//	ResizeProgram(ovi.base_width, ovi.base_height);
	}

	return ret;
}



void VideoConfig::GetFPSCommon(uint32_t &num, uint32_t &den, const ConfigFile& configFile) 
{
	const char *val = config_get_string(configFile, "Video", "FPSCommon");

	if (strcmp(val, "10") == 0) {
		num = 10;
		den = 1;
	}
	else if (strcmp(val, "20") == 0) {
		num = 20;
		den = 1;
	}
	else if (strcmp(val, "25") == 0) {
		num = 25;
		den = 1;
	}
	else if (strcmp(val, "29.97") == 0) {
		num = 30000;
		den = 1001;
	}
	else if (strcmp(val, "48") == 0) {
		num = 48;
		den = 1;
	}
	else if (strcmp(val, "59.94") == 0) {
		num = 60000;
		den = 1001;
	}
	else if (strcmp(val, "60") == 0) {
		num = 60;
		den = 1;
	}
	else {
		num = 30;
		den = 1;
	}
}

void VideoConfig::GetFPSInteger(uint32_t &num, uint32_t &den, const ConfigFile& configFile) 
{
	num = (uint32_t)config_get_uint(configFile, "Video", "FPSInt");
	den = 1;
}

void VideoConfig::GetFPSFraction(uint32_t &num, uint32_t &den, const ConfigFile& configFile) 
{
	num = (uint32_t)config_get_uint(configFile, "Video", "FPSNum");
	den = (uint32_t)config_get_uint(configFile, "Video", "FPSDen");
}

void VideoConfig::GetFPSNanoseconds(uint32_t &num, uint32_t &den, const ConfigFile& configFile) 
{
	num = 1000000000;
	den = (uint32_t)config_get_uint(configFile, "Video", "FPSNS");
}

void VideoConfig::GetConfigFPS(uint32_t &num, uint32_t &den, const ConfigFile& configFile) 
{
	uint32_t type = config_get_uint(configFile, "Video", "FPSType");

	if (type == 1) //"Integer"
		GetFPSInteger(num, den, configFile);
	else if (type == 2) //"Fraction"
		GetFPSFraction(num, den, configFile);
	else if (false) //"Nanoseconds", currently not implemented
		GetFPSNanoseconds(num, den, configFile);
	else
		GetFPSCommon(num, den, configFile);
}



const char* VideoConfig::GetRenderModule()
{
// 	const char *renderer = config_get_string(config, "Video",
// 		"Renderer");
// 
// 	return (astrcmpi(renderer, "Direct3D 11") == 0) ?
// 		DL_D3D11 : DL_OPENGL;

	return DL_D3D11;
}