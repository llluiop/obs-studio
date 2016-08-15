#ifndef __obs_wrapper_h__
#define __obs_wrapper_h__

#include "obs.h"
#include <windows.h>
#include <util/util.hpp>
#include <memory>
#include "window-basic-main-outputs.hpp"

struct AddSourceData {
	obs_source_t *source;
	bool visible;
};

struct MonitorInfo {
	int32_t  x, y;
	uint32_t cx, cy;

	inline MonitorInfo() {}
	inline MonitorInfo(int32_t x, int32_t y, uint32_t cx, uint32_t cy)
		: x(x), y(y), cx(cx), cy(cy)
	{}
};

class OBSWrapper
{
	const char* GAMESOURCE = "Window Capture";
	const char* GAMESOURCEID = "window_capture";
public:
	OBSWrapper();
	~OBSWrapper();

public:
	void APPInit();
	bool CreateGameSource(const HWND window);


	config_t *Config() const
	{
		return basicConfig;
	}

	obs_service_t *GetService()
	{
		if (!service) {
			service = obs_service_create("rtmp_common", NULL, NULL,
				nullptr);
			obs_service_release(service);
		}
		return service;
	}

private:
	bool startOBS();
	bool ResetAudio();
	void GetFPSCommon(uint32_t & num, uint32_t & den) const;
	void GetFPSInteger(uint32_t & num, uint32_t & den) const;
	void GetFPSFraction(uint32_t & num, uint32_t & den) const;
	void GetFPSNanoseconds(uint32_t & num, uint32_t & den) const;
	void GetConfigFPS(uint32_t & num, uint32_t & den) const;
	const char* GetRenderModule();
	int ResetVideo();
	void ResetOutputs();
	bool InitService();
	bool LoadService();
	void InitPrimitives();
	void InitDefaultTransitions();
	void Load(const char * file);
	void SetCurrentScene(obs_source_t * scene, bool force);
	void TransitionToScene(obs_source_t * source, bool force);
	void ClearSceneData();
	void SetTransition(obs_source_t * transition);
	void createDefaultScene(bool firstStart);
	void clearSceneData();
	void AddScene(OBSSource source);
	bool InitBasicConfigDefaults();
	int GetProfilePath(char * path, size_t size, const char * file) const;
	bool InitBasicConfig();
	bool InitGlobalConfigDefaults();
	bool initOBS();



private:
	obs_scene_t  *scene;
	ConfigFile config;
	ConfigFile basicConfig;
	OBSService service;
	OBSSource fadeTransition;

	gs_vertbuffer_t *box = nullptr;
	gs_vertbuffer_t *boxLeft = nullptr;
	gs_vertbuffer_t *boxTop = nullptr;
	gs_vertbuffer_t *boxRight = nullptr;
	gs_vertbuffer_t *boxBottom = nullptr;
	gs_vertbuffer_t *circle = nullptr;
	std::unique_ptr<BasicOutputHandler> outputHandler;
};

#endif // !__obs_h__

