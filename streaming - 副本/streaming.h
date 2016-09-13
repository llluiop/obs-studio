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
	template<class T>
	void PostMsg(const StreamMessageBody<T>& msg);

private:
	void HandleMsg();

	template<class T>
	void IPCMsg(const StreamMessageBody<T>& msg);
	template<class T>
	void OBSMsg(const StreamMessageBody<T>& msg);

	void SimpleIPCMsg(const StreamMessage& msg);
	void SimpleOBSMsg(const StreamMessage& msg);

private:
	OBSWrapper obsWrapper;
	IPCWrapper ipcWrapper;
	std::queue<std::function<void(void)>> msgIncomingQueue;
	std::queue<std::function<void(void)>> msgQueue;
	std::mutex mutexMsg;

	bool exit;
};