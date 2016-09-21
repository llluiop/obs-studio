// streaming.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "streaming.h"
#include <thread>
#include "ipc\IPC.h"

using namespace std;



Streaming::Streaming()
:exit(false),
obsWrapper(this),
ipcWrapper(this)
{
	obsWrapper.Init();
	ipcWrapper.Listen();
}

Streaming::~Streaming()
{

}

void Streaming::Start()
{
	while (!exit)
	{
		HandleMsg();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void Streaming::PostMsg(const StreamMessage& msg)
{
	std::lock_guard<std::mutex> guard(mutexMsg);
	msgIncomingQueue.push(msg);
}

void Streaming::HandleMsg()
{
	if (msgIncomingQueue.empty())
	{
		return;
	}

	{
		std::lock_guard<std::mutex> guard(mutexMsg);
		msgQueue.swap(msgIncomingQueue);
	}

	do 
	{
		auto msg = msgQueue.front();
		msgQueue.pop();

		if (msg.type == StreamMessage::IPC_MSG)
		{
			HandleIPCMsg(msg);
		}
		else
		{
			HandleOBSMsg(msg);
		}
	} while (!msgQueue.empty());
}

void Streaming::HandleIPCMsg(StreamMessage& msg)
{
	if (msg.id == MSG_IPC_QUIT)
	{
		obsWrapper.StopStream();
		exit = true;
	}
	if (msg.id == MSG_IPC_STARTSTREAMING)
	{
		obsWrapper.SetGameSource((HWND)atoi(msg.body.front().c_str()));
		msg.body.pop_front();
		obsWrapper.SetBitRate(msg.body.front().c_str());
		msg.body.pop_front();
		obsWrapper.SetSvrLocate(msg.body.front().c_str());
		msg.body.pop_front();
		obsWrapper.SetStreamingKey(msg.body.front().c_str());
		msg.body.pop_front();
		obsWrapper.SetMuteMic(msg.body.front().c_str());

		obsWrapper.StartStream();

	}
	if (msg.id == MSG_IPC_STOPSTREAMING)
	{
		obsWrapper.StopStream();
	}
}

void Streaming::HandleOBSMsg(StreamMessage& msg)
{
	if (msg.id == MSG_OBS_STREAMING_FAILED)
	{
		ipcWrapper.Send(OBS_TO_CLIENT_STREAMING_FAILED, _T(""));
	}
}



int main(int argc, char* argv)
{
	Streaming streaming;
	streaming.Start();

	return 0;
}