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
	bool StartOBS();
	bool ResetAudio();
	int ResetVideo();
	void ResetOutputs();
	bool InitService();
	bool LoadService();
	void InitDefaultTransitions();
	void Load(const char * file);
	void SetCurrentScene(obs_source_t * scene, bool force);
	void TransitionToScene(obs_source_t * source, bool force);
	void ClearSceneData();
	void SetTransition(obs_source_t * transition);
	void createDefaultScene(bool firstStart);
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

	std::unique_ptr<BasicOutputHandler> outputHandler;
};

#endif // !__obs_h__

