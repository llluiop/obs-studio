#pragma once
#include "ServerNamedPipe.h"
#include "atlstr.h"
#include "IPC.h"
#include "../streaming.h"

class CSDKPipeListener;
class IPCWrapper 
{
public:
	IPCWrapper(Streaming* streaming);
	~IPCWrapper();

public:
	bool Listen();
	bool Send(TCHAR* lpInput);

	bool RecivedMsg(int id);
	template<class T>
	bool RecivedMsg(int id, const T& body);

private:
	template<class T>
	bool PostMsg(const StreamMessageBody<T>& msg);
	bool PostMsg(const StreamMessage& msg);

private:
	Streaming* streaming;
	CSDKPipeListener* listener;
};


template<class T>
bool IPCWrapper::RecivedMsg(int id, const T& body)
{
	StreamMessageBody<T> msg;
	msg.type = StreamMessage::IPC_MSG;
	msg.id = id;
	msg.body = body;

	return PostMsg(msg);
}



template<class T>
bool IPCWrapper::PostMsg(const StreamMessageBody<T>& msg)
{
	if (streaming != nullptr)
	{
		streaming->PostMsg(msg);
		return true;
	}
	return false;
}

extern HRESULT SendMsg(TCHAR* lpInput, UINT nInputLen, TCHAR* lpOutput, UINT nOutputLen, DWORD dwTimeout, TCHAR* pipeName);
extern bool RscvMsg(const CString& serverName, CSDKPipeListener* listener);