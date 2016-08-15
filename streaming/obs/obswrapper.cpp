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


extern "C" void __declspec(dllexport) get_window_encode_strings(struct dstr *encode, const HWND window);
#define SIMPLE_ENCODER_X264 "x264"
#define DL_OPENGL "libobs-opengl.dll"
#define DL_D3D11 "libobs-d3d11.dll"

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


OBSWrapper::OBSWrapper()
{

}

OBSWrapper::~OBSWrapper()
{

}

void OBSWrapper::APPInit()
{
	startOBS();
	initOBS();

	CreateGameSource((HWND)0x00020602);
}

bool OBSWrapper::CreateGameSource(const HWND window)
{
	obs_source_t *source = obs_get_source_by_name(GAMESOURCE);
	if (source) {
		//QMessageBox::information(parent,
		//	QTStr("NameExists.Title"),
		//	QTStr("NameExists.Text"));

	}
	else {
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


  	struct dstr encode = { 0 };
  	get_window_encode_strings(&encode, window);
  	OBSData settings = obs_source_get_settings(source);
  	obs_data_set_string(settings, "window",
  		encode.array);
	obs_source_update(source, settings);
  	dstr_free(&encode);

	obs_data_t *svr = obs_service_get_settings(service);
	obs_data_set_string(svr, "server", "rtmp://send1.douyu.com/live");
	obs_data_set_string(svr, "key", "907136rvGEswUVl6?wsSecret=74f1eea1f0b15fb0cfeca52024c0557e&wsTime=57b11d35");
	obs_data_set_string(svr, "type", "rtmp_custom");


	if (!outputHandler->StartStreaming(service)) {
	}
	return true;
}

bool OBSWrapper::startOBS()
{
	char path[512];

	if (os_get_config_path(path, sizeof(path), "obs-studio/plugin_config") <= 0)
		return false;

	return obs_startup("en-US", path, nullptr);
}

bool OBSWrapper::ResetAudio()
{
	//ProfileScope("OBSBasic::ResetAudio");

	struct obs_audio_info ai;
	ai.samples_per_sec = config_get_uint(basicConfig, "Audio",
		"SampleRate");

	const char *channelSetupStr = config_get_string(basicConfig,
		"Audio", "ChannelSetup");

	if (strcmp(channelSetupStr, "Mono") == 0)
		ai.speakers = SPEAKERS_MONO;
	else
		ai.speakers = SPEAKERS_STEREO;

	return obs_reset_audio(&ai);
}

void OBSWrapper::GetFPSCommon(uint32_t &num, uint32_t &den) const
{
	const char *val = config_get_string(basicConfig, "Video", "FPSCommon");

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

void OBSWrapper::GetFPSInteger(uint32_t &num, uint32_t &den) const
{
	num = (uint32_t)config_get_uint(basicConfig, "Video", "FPSInt");
	den = 1;
}

void OBSWrapper::GetFPSFraction(uint32_t &num, uint32_t &den) const
{
	num = (uint32_t)config_get_uint(basicConfig, "Video", "FPSNum");
	den = (uint32_t)config_get_uint(basicConfig, "Video", "FPSDen");
}

void OBSWrapper::GetFPSNanoseconds(uint32_t &num, uint32_t &den) const
{
	num = 1000000000;
	den = (uint32_t)config_get_uint(basicConfig, "Video", "FPSNS");
}

void OBSWrapper::GetConfigFPS(uint32_t &num, uint32_t &den) const
{
	uint32_t type = config_get_uint(basicConfig, "Video", "FPSType");

	if (type == 1) //"Integer"
		GetFPSInteger(num, den);
	else if (type == 2) //"Fraction"
		GetFPSFraction(num, den);
	else if (false) //"Nanoseconds", currently not implemented
		GetFPSNanoseconds(num, den);
	else
		GetFPSCommon(num, den);
}

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

static inline enum obs_scale_type GetScaleType(ConfigFile &basicConfig)
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

const char* OBSWrapper::GetRenderModule()
{
	const char *renderer = config_get_string(config, "Video",
		"Renderer");

	return (astrcmpi(renderer, "Direct3D 11") == 0) ?
		DL_D3D11 : DL_OPENGL;
}

int OBSWrapper::ResetVideo()
{
	//ProfileScope("OBSBasic::ResetVideo");

	struct obs_video_info ovi;
	int ret;

	GetConfigFPS(ovi.fps_num, ovi.fps_den);

	const char *colorFormat = config_get_string(basicConfig, "Video",
		"ColorFormat");
	const char *colorSpace = config_get_string(basicConfig, "Video",
		"ColorSpace");
	const char *colorRange = config_get_string(basicConfig, "Video",
		"ColorRange");

	ovi.graphics_module = GetRenderModule();
	ovi.base_width = (uint32_t)config_get_uint(basicConfig,
		"Video", "BaseCX");
	ovi.base_height = (uint32_t)config_get_uint(basicConfig,
		"Video", "BaseCY");
	ovi.output_width = (uint32_t)config_get_uint(basicConfig,
		"Video", "OutputCX");
	ovi.output_height = (uint32_t)config_get_uint(basicConfig,
		"Video", "OutputCY");
	ovi.output_format = GetVideoFormatFromName(colorFormat);
	ovi.colorspace = astrcmpi(colorSpace, "601") == 0 ?
		VIDEO_CS_601 : VIDEO_CS_709;
	ovi.range = astrcmpi(colorRange, "Full") == 0 ?
		VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = GetScaleType(basicConfig);

	if (ovi.base_width == 0 || ovi.base_height == 0) {
		ovi.base_width = 1920;
		ovi.base_height = 1080;
		config_set_uint(basicConfig, "Video", "BaseCX", 1920);
		config_set_uint(basicConfig, "Video", "BaseCY", 1080);
	}

	if (ovi.output_width == 0 || ovi.output_height == 0) {
		ovi.output_width = ovi.base_width;
		ovi.output_height = ovi.base_height;
		config_set_uint(basicConfig, "Video", "OutputCX",
			ovi.base_width);
		config_set_uint(basicConfig, "Video", "OutputCY",
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

static bool do_mkdir(const char *path)
{
	if (os_mkdirs(path) == MKDIR_ERROR) {
		//OBSErrorBox(NULL, "Failed to create directory %s", path);
		return false;
	}

	return true;
}

static bool MakeUserDirs()
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


bool OBSWrapper::InitGlobalConfigDefaults()
{
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


bool OBSWrapper::initOBS()
{
	MakeUserDirs();

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

	//blog(LOG_INFO, MAIN_SEPARATOR);

	ResetOutputs();
	//CreateHotkeys();

	if (!InitService())
		throw "Failed to initialize service";

	InitPrimitives();



	Load(savePath);


	//TimedCheckForUpdates();
	//loaded = true;


// 	uint32_t winVer = GetWindowsVersion();
// 	if (winVer > 0 && winVer < 0x602) {
// 		bool disableAero = config_get_bool(basicConfig, "Video",
// 			"DisableAero");
// 		SetAeroEnabled(!disableAero);


	//RefreshSceneCollections();
	//RefreshProfiles();
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


void OBSWrapper::InitPrimitives()
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
		createDefaultScene(true); //create it always
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





void OBSWrapper::createDefaultScene(bool firstStart)
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
	//SetCurrentScene(scene, true);
	//obs_scene_release(scene);
	obs_source_t *source = obs_scene_get_source(scene);
	SetCurrentScene(source, true);
}




void OBSWrapper::AddScene(OBSSource source)
{
	const char *name = obs_source_get_name(source);
	obs_scene_t *scene = obs_scene_from_source(source);

// 	QListWidgetItem *item = new QListWidgetItem(QT_UTF8(name));
// 	SetOBSRef(item, OBSScene(scene));
// 	ui->scenes->addItem(item);

// 	obs_hotkey_register_source(source, "OBSBasic.SelectScene",
// 		Str("Basic.Hotkeys.SelectScene"),
// 		[](void *data,
// 			obs_hotkey_id, obs_hotkey_t*, bool pressed)
// 	{
// 		OBSBasic *main =
// 			reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
// 
// 		auto potential_source = static_cast<obs_source_t*>(data);
// 		auto source = obs_source_get_ref(potential_source);
// 		if (source && pressed)
// 			main->SetCurrentScene(source);
// 		obs_source_release(source);
// 	}, static_cast<obs_source_t*>(source));

// 	signal_handler_t *handler = obs_source_get_signal_handler(source);
// 
// 	SignalContainer<OBSScene> container;
// 	container.ref = scene;
// 	container.handlers.assign({
// 		std::make_shared<OBSSignal>(handler, "item_add",
// 		OBSBasic::SceneItemAdded, this),
// 		std::make_shared<OBSSignal>(handler, "item_remove",
// 		OBSBasic::SceneItemRemoved, this),
// 		std::make_shared<OBSSignal>(handler, "item_select",
// 		OBSBasic::SceneItemSelected, this),
// 		std::make_shared<OBSSignal>(handler, "item_deselect",
// 		OBSBasic::SceneItemDeselected, this),
// 		std::make_shared<OBSSignal>(handler, "reorder",
// 		OBSBasic::SceneReordered, this),
// 	});

// 	item->setData(static_cast<int>(QtDataRole::OBSSignals),
// 		QVariant::fromValue(container));

	/* if the scene already has items (a duplicated scene) add them */
// 	auto addSceneItem = [this](obs_sceneitem_t *item)
// 	{
// 		AddSceneItem(item);
// 	};
// 
// 	using addSceneItem_t = decltype(addSceneItem);
// 
// 	obs_scene_enum_items(scene,
// 		[](obs_scene_t*, obs_sceneitem_t *item, void *param)
// 	{
// 		addSceneItem_t *func;
// 		func = reinterpret_cast<addSceneItem_t*>(param);
// 		(*func)(item);
// 		return true;
// 	}, &addSceneItem);

	//SaveProject();
}

static BOOL CALLBACK OBSMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
	LPRECT rect, LPARAM param)
{
	std::vector<MonitorInfo> &monitors = *(std::vector<MonitorInfo> *)param;

	monitors.emplace_back(
		rect->left,
		rect->top,
		rect->right - rect->left,
		rect->bottom - rect->top);

	UNUSED_PARAMETER(hMonitor);
	UNUSED_PARAMETER(hdcMonitor);
	return true;
}

void GetMonitors(std::vector<MonitorInfo> &monitors)
{
	monitors.clear();
	EnumDisplayMonitors(NULL, NULL, OBSMonitorEnumProc, (LPARAM)&monitors);
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


bool OBSWrapper::InitBasicConfigDefaults()
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

	return InitBasicConfigDefaults();
}
