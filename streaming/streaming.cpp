// streaming.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "streaming.h"
#include <thread>
#include "ipc\IPC.h"

using namespace std;
#define EVENT_STREAMING _T("Global\\CoreClient_Streaming")
#define EVENT_CLIENT_START _T("Global\\CoreClient_StartEvent")

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) // 设置入口地址  



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
		ShouldExit();
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

		obsWrapper.SetFitScreen();
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



void Streaming::ShouldExit()
{
	HANDLE hEvent = ::OpenEvent(SYNCHRONIZE, FALSE, EVENT_CLIENT_START);
	if (hEvent == NULL)
	{
		exit = true;
	}
	::CloseHandle(hEvent);
}

int main(int argc, char* argv)
{	
	HANDLE hInstance = ::CreateEvent(NULL, FALSE, FALSE, EVENT_STREAMING);
	if (ERROR_ALREADY_EXISTS == ::GetLastError())
		return -1;

	Streaming streaming;
	streaming.Start();

	CloseHandle(hInstance);
	return 0;
}