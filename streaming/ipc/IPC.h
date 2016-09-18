#pragma once

#define MSG_IPC_QUIT 0
#define MSG_IPC_STARTSTREAMING 1
#define MSG_IPC_STOPSTREAMING 2

#define MSG_OBS_STREAMING_FAILED 10

#define OBS_TO_CLIENT_STREAMING_FAILED 100

#include "tstring.h"
using namespace String;

struct  StreamMessage
{
	enum MsgType {
		OBS_MSG,
		IPC_MSG,
	};

	MsgType type;
	int id;
	_tstring::SplitList body;
};
