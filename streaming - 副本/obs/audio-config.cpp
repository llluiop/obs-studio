#include "audio-config.h"
#include <obs.h>
#include <util/config-file.h>
#include <obs-config.h>
#include <util/bmem.h>
#include <util/dstr.h>

bool AudioConfig::SetAudio(const ConfigFile & configFile)
{
	struct obs_audio_info ai;
	ai.samples_per_sec = config_get_uint(configFile, "Audio",
		"SampleRate");

	const char *channelSetupStr = config_get_string(configFile,
		"Audio", "ChannelSetup");

	if (strcmp(channelSetupStr, "Mono") == 0)
		ai.speakers = SPEAKERS_MONO;
	else
		ai.speakers = SPEAKERS_STEREO;

	return obs_reset_audio(&ai);
}
