#pragma once

#include <string>
#include <queue>
#include <mutex>
#include "obs/obswrapper.h"
#include "ipc/IPCWrapper.h"

class Streaming
{
public:
	Streaming();
	~Streaming();


public:
	void Start();
	void PostMsg(const StreamMessage& msg);

private:
	void HandleMsg();
	void HandleIPCMsg(const StreamMessage& msg);
	void HandleOBSMsg(const StreamMessage& msg);

private:
	OBSWrapper obsWrapper;
	IPCWrapper ipcWrapper;
	std::queue<StreamMessage> msgIncomingQueue;
	std::queue<StreamMessage> msgQueue;
	std::mutex mutexMsg;

	bool exit;
};