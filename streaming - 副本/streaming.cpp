// streaming.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "streaming.h"
#include <thread>
#include "ipc\IPC.h"
#include "internalmsg.h"

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

		msg();
	} while (!msgQueue.empty());
}


void Streaming::PostMsg(const StreamMessage & msg)
{
	std::lock_guard<std::mutex> guard(mutexMsg);
	if (msg.type == StreamMessage::IPC_MSG) SimpleIPCMsg(msg);
	if (msg.type == StreamMessage::OBS_MSG) SimpleOBSMsg(msg);
}

template<class T>
void Streaming::PostMsg(const StreamMessageBody<T>& msg)
{
	std::lock_guard<std::mutex> guard(mutexMsg);
	if (msg.type == StreamMessage::IPC_MSG) IPCMsg(msg);
	if (msg.type == StreamMessage::OBS_MSG) OBSMsg(msg);
}

void Streaming::SimpleIPCMsg(const StreamMessage & msg)
{
	if (msg.id == MSG_IPC_QUIT)
	{
		msgIncomingQueue.push([&]() {
			obsWrapper.StopStream();
			exit = true;
		});
	}
	if (msg.id == MSG_IPC_STARTSTREAMING)
	{
		msgIncomingQueue.push([&]() {
			obsWrapper.StartStream();
		});
	}
	if (msg.id == MSG_IPC_STOPSTREAMING)
	{
		msgIncomingQueue.push([&]() {
			obsWrapper.StopStream();
		});
	}
}

void Streaming::SimpleOBSMsg(const StreamMessage & msg)
{
	if (msg.id == MSG_OBS_STREAMING_FAILED)
	{
		msgIncomingQueue.push([&]() {
			ipcWrapper.Send(_T("streaming failed"));
		});
	}
}


template<class T>
void Streaming::IPCMsg(const StreamMessageBody<T>& msg)
{
	msgIncomingQueue.push(msg.body.Dispatch(&obsWrapper));
}


template<class T>
void Streaming::OBSMsg(const StreamMessageBody<T>& msg)
{
}


int main(int argc, char* argv)
{
	//Streaming streaming;
	//streaming.Start();
	OBSWrapper obs(nullptr);
	obs.Init();
	obs.SetGameSource(HWND(0x001D1870));
	obs.StartStream();

	return getchar();
}