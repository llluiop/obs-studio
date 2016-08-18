#pragma once

#include "ServerNamedPipe.h"


#define  SDK_SERVER_PIPE_NAME _T("ArcSDKServerPipe")

class IPCWrapper;
class CSDKPipeListener:public IPipeBufferListener
{
public:
	CSDKPipeListener(IPCWrapper* ipcWrapper);
	~CSDKPipeListener(void);
	
	virtual HRESULT HandleReadBuf(PIPEINST& Pipe);

private:
	IPCWrapper* ipcWrapper;
};
