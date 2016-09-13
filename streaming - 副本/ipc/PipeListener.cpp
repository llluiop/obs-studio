#include "PipeListener.h"
#include "tstring.h"
#include "IPCWrapper.h"
#include "ipc.h"
#include "../internalmsg.h"

using namespace String;

CSDKPipeListener::CSDKPipeListener(IPCWrapper* ipcWrapper)
:ipcWrapper(ipcWrapper)
{

}

CSDKPipeListener::~CSDKPipeListener(void)
{
}


HRESULT CSDKPipeListener::HandleReadBuf( PIPEINST& Pipe )
{
	if (ipcWrapper == nullptr)
	{
		return -1;
	}

    _tstring sRequest(Pipe.chRequest);
    OutputDebugString(sRequest.c_str());

    _tstring::SplitList Comandlist = sRequest.Split(_T("|"));
    if (Comandlist.empty())
    {
        return 0L;
    }
    int nComand = atoi(Comandlist.front().c_str());
    Comandlist.pop_front();


	if (nComand == MSG_IPC_QUIT)
	{
		ipcWrapper->RecivedMsg(nComand);
	}
	if (nComand == MSG_IPC_SETGAME)
	{
		StreamingSettting setting;
		setting.hwnd = Comandlist.front().toUTF8().c_str(); Comandlist.pop_front();
		setting.bitrate = Comandlist.front().toUTF8().c_str(); Comandlist.pop_front();
		setting.key = Comandlist.front().toUTF8().c_str(); Comandlist.pop_front();
		setting.loc = Comandlist.front().toUTF8().c_str();

		ipcWrapper->RecivedMsg<StreamingSettting>(nComand, setting);
	}
	if (nComand == MSG_IPC_STARTSTREAMING)
	{
		ipcWrapper->RecivedMsg(nComand);
	}
	if (nComand == MSG_IPC_STOPSTREAMING)
	{
		ipcWrapper->RecivedMsg(nComand);
	}

	return 1;
}


