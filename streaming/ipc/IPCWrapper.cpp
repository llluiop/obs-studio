#include "IPCWrapper.h"
#include "../streaming.h"
#include "PipeListener.h"



IPCWrapper::IPCWrapper(Streaming* streaming)
:streaming(streaming)
{
	listener = new CSDKPipeListener(this);
}

IPCWrapper::~IPCWrapper()
{
	delete listener;
	listener = nullptr;
}

bool IPCWrapper::Listen()
{	
	return RscvMsg(_T("Arc_Streaming"), listener);
}

bool IPCWrapper::Send(TCHAR* lpInput)
{
	TCHAR szOutPut[OUTBUFSIZE] = { 0 };
	return S_OK == SendMsg(lpInput, wcslen(lpInput), szOutPut, OUTBUFSIZE, 5000, _T("BrowserPluginServerPipe"));
}

bool IPCWrapper::RecivedMsg(int id, std::string body)
{
	StreamMessage msg;
	msg.type = StreamMessage::IPC_MSG;
	msg.id = id;
	msg.body = body;

	return PostMsg(msg);
}

bool IPCWrapper::RecivedMsg(int id)
{
	StreamMessage msg;
	msg.type = StreamMessage::IPC_MSG;
	msg.id = id;
	
	return PostMsg(msg);
}

bool IPCWrapper::PostMsg(const StreamMessage& msg)
{
	if (streaming != nullptr)
	{
		streaming->PostMsg(msg);
		return true;
	}
	return false;
}
