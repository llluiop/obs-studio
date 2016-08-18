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

void Streaming::HandleIPCMsg(const StreamMessage& msg)
{
	if (msg.id == MSG_IPC_QUIT)
	{
		obsWrapper.StopStream();
		exit = true;
	}
	if (msg.id == MSG_IPC_SETGAME)
	{
		int hwnd = atoi(msg.body.c_str());
		obsWrapper.SetGameSource((HWND)hwnd);
	}
	if (msg.id == MSG_IPC_STARTSTREAMING)
	{
		obsWrapper.StartStream();
	}
	if (msg.id == MSG_IPC_STOPSTREAMING)
	{
		obsWrapper.StopStream();
	}
}

void Streaming::HandleOBSMsg(const StreamMessage& msg)
{
	if (msg.id == MSG_OBS_STREAMING_FAILED)
	{
		ipcWrapper.Send(_T("streaming failed"));
	}
}



int main(int argc, char* argv)
{
	Streaming streaming;
	streaming.Start();

	return 0;
}