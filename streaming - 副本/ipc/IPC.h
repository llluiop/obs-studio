#pragma once

#define MSG_IPC_QUIT 0
#define MSG_IPC_SETGAME 1
#define MSG_IPC_STARTSTREAMING 2
#define MSG_IPC_STOPSTREAMING 3

#define MSG_OBS_STREAMING_FAILED 10


#include <string>
#include <functional>

struct  StreamMessage
{
	enum MsgType {
		OBS_MSG,
		IPC_MSG,
	};

	MsgType type;
	int id;

};

template<class T>
struct StreamMessageBody : public StreamMessage {
	T body;
};

