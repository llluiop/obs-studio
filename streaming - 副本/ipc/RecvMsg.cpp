#include <atlstr.h>
#include "PipeListener.h"
#include "ClientNamedPipe.h"
#include "ServerNamedPipe.h"




bool RscvMsg(const CString& serverName, CSDKPipeListener* listener)
{
	CServerNamedPipe* pPipe = new CServerNamedPipe(NULL, serverName, listener);
	return pPipe->StartPipeServer();
}