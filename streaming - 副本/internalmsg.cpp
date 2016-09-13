#include "internalmsg.h"
#include "obs/obswrapper.h"

std::function<void(void)> StreamingSettting::Dispatch(OBSWrapper * obs)
{
	return [&]{
		obs->SetGameSource((HWND)atoi(hwnd.c_str()));
		obs->SetBitRate(bitrate);
		obs->SetStreamingKey(key);
		obs->SetSvrLocate(loc);
	};
}
