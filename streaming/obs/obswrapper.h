#ifndef __obs_wrapper_h__
#define __obs_wrapper_h__

#include "obs.h"
#include <windows.h>
#include <string>
#include <util/util.hpp>
#include <memory>
#include <vector>
#include "window-basic-main-outputs.hpp"

struct AddSourceData {
	obs_source_t *source;
	bool visible;
};


class Streaming;
class OBSWrapper
{
	const char* GAMESOURCE = "Game Capture";
	const char* GAMESOURCEID = "game_capture";
public:
	OBSWrapper(Streaming* streaming);
	~OBSWrapper();

public:
	void Init();
	bool SetGameSource(const HWND window);
	bool SetStreamingKey(const std::string key);
	bool SetBitRate(const std::string bit);
	bool SetSvrLocate(const std::string loc);
	bool SetMuteMic(const std::string mic);

	bool StartStream();
	void StopStream();

	void StreamStop();

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
	void createDefaultScene();
	int GetProfilePath(char * path, size_t size, const char * file) const;
	bool InitBasicConfig();
	bool InitGlobalConfigDefaults();
	bool InitOBS();
	void InitOBSCallbacks();

	static void SourceActivated(void *data, calldata_t *params);

;
private:
	obs_scene_t  *scene;
	ConfigFile config;
	ConfigFile basicConfig;
	OBSService service;
	OBSSource fadeTransition;
	OBSSource micService;

	std::unique_ptr<BasicOutputHandler> outputHandler;
	std::vector<OBSSignal> signalHandlers;

	Streaming* streaming;
};

#endif // !__obs_h__

