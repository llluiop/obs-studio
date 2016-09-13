#include "ClientNamedPipe.h"
#include "ServerNamedPipe.h"

#include <vector>

using namespace std ;


typedef struct _CC_PIPE_REQ
{
	TCHAR lpInput[INBUFSIZE];
	UINT nInputLen;
	TCHAR lpOutput[OUTBUFSIZE];
	UINT nOutputLen;
	DWORD dwTimeout;
	HRESULT hRet;
	CClientNamedPipe* pPipe;
	_CC_PIPE_REQ(TCHAR* lpszInput,UINT nInLen,TCHAR* lpszOutput,UINT nOutLen,DWORD nTimeout,HRESULT hr,CClientNamedPipe* lpPipe)
	{
		nInputLen = (nInLen>INBUFSIZE)?INBUFSIZE:nInLen;
		nOutputLen = (nOutLen>OUTBUFSIZE)?OUTBUFSIZE:nOutLen;
		lstrcpyn(lpInput,lpszInput,nInputLen);
		lstrcpyn(lpOutput,lpszOutput,nOutputLen);
		dwTimeout = nTimeout;
		hRet =hr;
		pPipe = lpPipe;
	};
}PIPE_REQ,*PPIPE_REQ;

static void _stdcall PipeReqProc(LPVOID lp)
{
	PPIPE_REQ pReq = (PPIPE_REQ)lp;
	pReq->hRet = pReq->pPipe->PipeSendAndReceive(pReq->lpInput,(pReq->nInputLen)*sizeof(TCHAR),pReq->lpOutput,(pReq->nOutputLen)*sizeof(TCHAR));
}

HRESULT SendMsg(TCHAR* lpInput, UINT nInputLen, TCHAR* lpOutput, UINT nOutputLen, DWORD dwTimeout, TCHAR* pipeName)
{
	CClientNamedPipe pipe(NULL, pipeName, dwTimeout);
	PIPE_REQ PipeReq(lpInput, nInputLen, lpOutput, nOutputLen, dwTimeout, S_OK, &pipe);

	PipeReq.hRet = PipeReq.pPipe->Initialize();
	if (FAILED(PipeReq.hRet))
	{
		return PipeReq.hRet;
	}

	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PipeReqProc, &PipeReq, 0, NULL);
	if (hThread == NULL)
	{
		return PipeReq.hRet = S_FALSE;
	}

	DWORD dwRet = WaitForSingleObject(hThread, dwTimeout);
	switch (dwRet)
	{
	case WAIT_OBJECT_0://successful response.
	{
		lstrcpyn(lpOutput, PipeReq.lpOutput, nOutputLen);
	}
	break;
	case WAIT_TIMEOUT://time out
	{
		PipeReq.hRet = -1;
		TerminateThread(hThread, 0);
	}
	break;
	default:
	{
		PipeReq.hRet = S_FALSE;
		TerminateThread(hThread, 0);
	}
	break;
	}
	CloseHandle(hThread);
	hThread = NULL;

	return PipeReq.hRet;
}