#include "obswrapper.h"
#include <time.h>
#include <stdio.h>
#include <wchar.h>
#include <chrono>
#include <ratio>
#include <sstream>
#include <mutex>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/config-file.h>
#include <obs-config.h>
#include <string>
#include <shlobj.h>
#include <fstream>
#include <vector>
#include <curl/curl.h>
#include "../streaming.h"

#include "video-config.h"
#include "audio-config.h"
#include "basic-config.h"


extern "C" void __declspec(dllexport) get_window_encode_strings(struct dstr *encode, const HWND window);


static void AddExtraModulePaths()
{
	char base_module_dir[512];

	int ret = os_get_program_data_path(base_module_dir, sizeof(base_module_dir),
		"obs-studio/plugins/%module%");

	if (ret <= 0)
		return;

	std::string path = (char*)base_module_dir;
	obs_add_module_path((path + "/bin/32bit").c_str(),
		(path + "/data").c_str());

}

static void AddSource(void *_data, obs_scene_t *scene)
{
	AddSourceData *data = (AddSourceData *)_data;
	obs_sceneitem_t *sceneitem;

	sceneitem = obs_scene_add(scene, data->source);
	obs_sceneitem_set_visible(sceneitem, data->visible);
}


OBSWrapper::OBSWrapper(Streaming* streaming)
:streaming(streaming)
{

}

OBSWrapper::~OBSWrapper()
{

}

void OBSWrapper::Init()
{
	StartOBS();
	InitOBS();	
}

bool OBSWrapper::StartOBS()
{
	char path[512];

	if (os_get_config_path(path, sizeof(path), "obs-studio/plugin_config") <= 0)
		return false;

	return obs_startup("en-US", path, nullptr);
}

bool OBSWrapper::SetGameSource(const HWND window)
{
	obs_source_t *source = obs_get_source_by_name(GAMESOURCE);
	if (!source) {
		source = obs_source_create(GAMESOURCEID, GAMESOURCE, NULL, nullptr);

		if (source) {
			AddSourceData data;
			data.source = source;
			data.visible = true;
			obs_scene_atomic_update(scene, AddSource, &data);
		}
		else
		{
			return false;
		}
	}

	obs_source_properties(source);

  	struct dstr encode = { 0 };
  	get_window_encode_strings(&encode, window);
  	OBSData settings = obs_source_get_settings(source);
	obs_data_set_bool(settings, "capture_any_fullscreen",
		false);
  	obs_data_set_string(settings, "window",
  		encode.array);
	obs_source_update(source, settings);
  	dstr_free(&encode);

	obs_data_t *svr = obs_service_get_settings(service);
	obs_data_set_string(svr, "server", "rtmp://send1.douyu.com/live");
	obs_data_set_string(svr, "key", "907136rOlUwvw0Wp?wsSecret=9c50bfe02145ab7e8f915cdfa54a0ac9&wsTime=57c3a673");
	//obs_data_set_string(svr, "type", "rtmp_custom");


	char serviceJsonPath[512];
	int ret = GetProfilePath(serviceJsonPath, sizeof(serviceJsonPath),
		"service.json");
	if (ret <= 0)
		return false;

	obs_data_t *data = obs_data_create();

	obs_data_set_string(data, "type", obs_service_get_type(service));
	obs_data_set_obj(data, "settings", svr);

	if (!obs_data_save_json_safe(data, serviceJsonPath, "tmp", "bak"))
		blog(LOG_WARNING, "Failed to save service");

	obs_data_release(svr);
	obs_data_release(data);

 	return true;
}

bool OBSWrapper::SetStreamingKey(const std::string key)
{
	return false;
}

bool OBSWrapper::SetBitRate(const std::string bit)
{
	return false;
}

bool OBSWrapper::SetSvrLocate(const std::string loc)
{
	return false;
}

bool OBSWrapper::StartStream()
{
	return outputHandler->StartStreaming(service);
}

void OBSWrapper::StopStream()
{
	outputHandler->StopStreaming();
}



bool OBSWrapper::ResetAudio()
{
	//ProfileScope("OBSBasic::ResetAudio");
	return AudioConfig::SetAudio(basicConfig);

}


int OBSWrapper::ResetVideo()
{
	//ProfileScope("OBSBasic::ResetVideo");
	return VideoConfig::SetVideo(basicConfig);	
}



#define MAIN_SEPARATOR \
	"====================================================================="

bool OBSWrapper::InitOBS()
{
	BasicConfig::MakeUserDirs();

	InitGlobalConfigDefaults();

	config_set_default_string(config, "Basic", "Profile",
		("Untitled"));
	config_set_default_string(config, "Basic", "ProfileDir",
		("Untitled"));
	config_set_default_string(config, "Basic", "SceneCollection",
		("Untitled"));
	config_set_default_string(config, "Basic", "SceneCollectionFile",
		("Untitled"));

	const char *sceneCollection = config_get_string(config,
		"Basic", "SceneCollectionFile");
	char savePath[512];
	char fileName[512];
	int ret;

	if (!sceneCollection)
		throw "Failed to get scene collection name";

	ret = snprintf(fileName, 512, "obs-studio/basic/scenes/%s.json",
		sceneCollection);
	if (ret <= 0)
		throw "Failed to create scene collection file name";

	ret = os_get_config_path(savePath, sizeof(savePath), fileName);
	if (ret <= 0)
		throw "Failed to get scene collection json file path";

	if (!InitBasicConfig())
		throw "Failed to load basic.ini";
	if (!ResetAudio())
		throw "Failed to initialize audio";

	ret = ResetVideo();

	switch (ret) {
	case OBS_VIDEO_MODULE_NOT_FOUND:
		throw "Failed to initialize video:  Graphics module not found";
	case OBS_VIDEO_NOT_SUPPORTED:
		throw "Failed to initialize video:  Required graphics API "
			"functionality not found on these drivers or "
			"unavailable on this equipment";
	case OBS_VIDEO_INVALID_PARAM:
		throw "Failed to initialize video:  Invalid parameters";
	default:
		if (ret != OBS_VIDEO_SUCCESS)
			throw "Failed to initialize video:  Unspecified error";
	}

	//InitOBSCallbacks();
	//InitHotkeys();

	AddExtraModulePaths();
	obs_load_all_modules();

	blog(LOG_INFO, MAIN_SEPARATOR);

	ResetOutputs();
	//CreateHotkeys();

	if (!InitService())
		throw "Failed to initialize service";

	BasicConfig::InitPrimitives();

	Load(savePath);


	return true;
}



void OBSWrapper::ResetOutputs()
{
	//ProfileScope("OBSBasic::ResetOutputs");
	if (!outputHandler || !outputHandler->Active()) {
		outputHandler.reset(CreateSimpleOutputHandler(this));
	}
	else {
		outputHandler->Update();
	}
}


bool OBSWrapper::InitService()
{
	if (LoadService())
		return true;

	service = obs_service_create("rtmp_common", "default_service", nullptr,
		nullptr);
	if (!service)
		return false;
	obs_service_release(service);

	return true;
}

#define SERVICE_PATH "service.json"


bool OBSWrapper::LoadService()
{
	const char *type;

	char serviceJsonPath[512];
	int ret = GetProfilePath(serviceJsonPath, sizeof(serviceJsonPath),
		SERVICE_PATH);
	if (ret <= 0)
		return false;

	obs_data_t *data = obs_data_create_from_json_file_safe(serviceJsonPath,
		"bak");

	obs_data_set_default_string(data, "type", "rtmp_common");
	type = obs_data_get_string(data, "type");

	obs_data_t *settings = obs_data_get_obj(data, "settings");
	obs_data_t *hotkey_data = obs_data_get_obj(data, "hotkeys");

	service = obs_service_create(type, "default_service", settings,
		hotkey_data);
	obs_service_release(service);

	obs_data_release(hotkey_data);
	obs_data_release(settings);
	obs_data_release(data);

	return !!service;
}


void OBSWrapper::InitDefaultTransitions()
{
	std::vector<OBSSource> transitions;
	size_t idx = 0;
	const char *id;

	/* automatically add transitions that have no configuration (things
	* such as cut/fade/etc) */
	while (obs_enum_transition_types(idx++, &id)) {
		if (!obs_is_source_configurable(id)) {
			const char *name = obs_source_get_display_name(id);

			obs_source_t *tr = obs_source_create_private(
				id, name, NULL);
			//InitTransition(tr);
			transitions.emplace_back(tr);

			if (strcmp(id, "fade_transition") == 0)
				fadeTransition = tr;

			obs_source_release(tr);
		}
	}
}

void OBSWrapper::Load(const char *file)
{
	//if (!file || !os_file_exists(file)) {
	//	blog(LOG_INFO, "No scene file found, creating default scene");
		createDefaultScene(); //create it always
	//	return;
	//}
// 		ClearSceneData();
// 		InitDefaultTransitions();
// 		SetTransition(fadeTransition);


}

void OBSWrapper::SetCurrentScene(obs_source_t *scene, bool force)
{
	
		TransitionToScene(scene, force);

	
// 
// 	if (obs_scene_get_source(GetCurrentScene()) != scene) {
// 		for (int i = 0; i < ui->scenes->count(); i++) {
// 			QListWidgetItem *item = ui->scenes->item(i);
// 			OBSScene itemScene = GetOBSRef<OBSScene>(item);
// 			obs_source_t *source = obs_scene_get_source(itemScene);
// 
// 			if (source == scene) {
// 				ui->scenes->blockSignals(true);
// 				ui->scenes->setCurrentItem(item);
// 				ui->scenes->blockSignals(false);
// 				break;
// 			}
// 		}
// 	}

	//UpdateSceneSelection(scene);
}


void OBSWrapper::TransitionToScene(obs_source_t *source, bool force)
{
	obs_scene_t *scene = obs_scene_from_source(source);
	if (!scene)
		return;

	obs_source_t *transition = obs_get_output_source(0);

	if (force)
		obs_transition_set(transition, source);
	//else
	//	obs_transition_start(transition, OBS_TRANSITION_MODE_AUTO,
	//		ui->transitionDuration->value(), source);

	obs_source_release(transition);
}

void OBSWrapper::ClearSceneData()
{
	obs_set_output_source(0, nullptr);
	obs_set_output_source(1, nullptr);
	obs_set_output_source(2, nullptr);
	obs_set_output_source(3, nullptr);
	obs_set_output_source(4, nullptr);
	obs_set_output_source(5, nullptr);


	auto cb = [](void *unused, obs_source_t *source)
	{
		obs_source_remove(source);
		UNUSED_PARAMETER(unused);
		return true;
	};

	obs_enum_sources(cb, nullptr);


	blog(LOG_INFO, "All scene data cleared");
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSWrapper::SetTransition(obs_source_t *transition)
{
	obs_source_t *oldTransition = obs_get_output_source(0);

	if (oldTransition && transition) {
		obs_transition_swap_begin(transition, oldTransition);
// 		if (transition != GetCurrentTransition())
// 			SetComboTransition(ui->transitions, transition);
		obs_set_output_source(0, transition);
		obs_transition_swap_end(transition, oldTransition);
	}
	else {
		obs_set_output_source(0, transition);
	}

	if (oldTransition)
		obs_source_release(oldTransition);

	bool fixed = transition ? obs_transition_fixed(transition) : false;
	//ui->transitionDurationLabel->setVisible(!fixed);
	//ui->transitionDuration->setVisible(!fixed);

	bool configurable = obs_source_configurable(transition);
	//ui->transitionRemove->setEnabled(configurable);
	//ui->transitionProps->setEnabled(configurable);
}





void OBSWrapper::createDefaultScene()
{
	ClearSceneData();
	InitDefaultTransitions();
	//CreateDefaultQuickTransitions();
	//ui->transitionDuration->setValue(300);
	SetTransition(fadeTransition);

	scene = obs_scene_create("Basic.Scene");

	//if (firstStart)
	//	CreateFirstRunSources();

	//AddScene(obs_scene_get_source(scene));

	obs_source_t *source = obs_scene_get_source(scene);
	SetCurrentScene(source, true);
}



int OBSWrapper::GetProfilePath(char *path, size_t size, const char *file) const
{
	char profiles_path[512];
	const char *profile = config_get_string(config,
		"Basic", "ProfileDir");
	int ret;

	if (!profile)
		return -1;
	if (!path)
		return -1;
	if (!file)
		file = "";

	ret = os_get_config_path(profiles_path, 512, "obs-studio/basic/profiles");
	if (ret <= 0)
		return ret;

	if (!*file)
		return snprintf(path, size, "%s/%s", profiles_path, profile);

	return snprintf(path, size, "%s/%s/%s", profiles_path, profile, file);
}


bool OBSWrapper::InitBasicConfig()
{
	char configPath[512];

	int ret = GetProfilePath(configPath, sizeof(configPath), "");
	if (ret <= 0) {
		//OBSErrorBox(nullptr, "Failed to get profile path");
		return false;
	}

	if (os_mkdir(configPath) == MKDIR_ERROR) {
		return false;
	}

	ret = os_get_config_path(configPath, sizeof(configPath), "basic.ini");
	if (ret <= 0) {
		return false;
	}

	int code = basicConfig.Open(configPath, CONFIG_OPEN_ALWAYS);
	if (code != CONFIG_SUCCESS) {
		return false;
	}

	if (config_get_string(basicConfig, "General", "Name") == nullptr) {
		const char *curName = config_get_string(config,
			"Basic", "Profile");

		config_set_string(basicConfig, "General", "Name", curName);
		basicConfig.SaveSafe("tmp");
	}

	return BasicConfig::InitBasicConfigDefaults(basicConfig);
}



bool OBSWrapper::InitGlobalConfigDefaults()
{
	char path[512];

	int len = os_get_config_path(path, sizeof(path),
		"obs-studio/global.ini");
	if (len <= 0) {
		return false;
	}

	int errorcode = config.Open(path, CONFIG_OPEN_ALWAYS);
	if (errorcode != CONFIG_SUCCESS) {
		//OBSErrorBox(NULL, "Failed to open global.ini: %d", errorcode);
		return false;
	}

	config_set_default_string(config, "General", "Language",
		"en-US");
	config_set_default_uint(config, "General", "MaxLogs", 10);
	config_set_default_string(config, "General", "ProcessPriority",
		"Normal");


	config_set_default_string(config, "Video", "Renderer",
		"Direct3D 11");


	config_set_default_bool(config, "BasicWindow", "PreviewEnabled",
		true);
	config_set_default_bool(config, "BasicWindow",
		"PreviewProgramMode", false);
	config_set_default_bool(config, "BasicWindow",
		"SceneDuplicationMode", true);
	config_set_default_bool(config, "BasicWindow",
		"SwapScenesMode", true);
	config_set_default_bool(config, "BasicWindow",
		"SnappingEnabled", true);
	config_set_default_bool(config, "BasicWindow",
		"ScreenSnapping", true);
	config_set_default_bool(config, "BasicWindow",
		"SourceSnapping", true);
	config_set_default_bool(config, "BasicWindow",
		"CenterSnapping", false);
	config_set_default_double(config, "BasicWindow",
		"SnapDistance", 10.0);
	config_set_default_bool(config, "BasicWindow",
		"RecordWhenStreaming", false);
	config_set_default_bool(config, "BasicWindow",
		"KeepRecordingWhenStreamStops", false);
	config_set_default_bool(config, "BasicWindow",
		"ShowTransitions", true);
	config_set_default_bool(config, "BasicWindow",
		"ShowListboxToolbars", true);
	config_set_default_bool(config, "BasicWindow",
		"ShowStatusBar", true);


	return true;
}
