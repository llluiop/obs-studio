#pragma once
#include "ServerNamedPipe.h"
#include "atlstr.h"
#include "IPC.h"

class Streaming;
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
	bool RecivedMsg(int id, std::string body);

private:
	bool PostMsg(const StreamMessage& msg);

private:
	Streaming* streaming;
	CSDKPipeListener* listener;
};

extern HRESULT SendMsg(TCHAR* lpInput, UINT nInputLen, TCHAR* lpOutput, UINT nOutputLen, DWORD dwTimeout, TCHAR* pipeName);
extern bool RscvMsg(const CString& serverName, CSDKPipeListener* listener);