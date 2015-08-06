// plugin_quividi.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "CInfoManager.h"

#if _MSC_VER // this is defined when compiling with Visual Studio
#define EXPORT_API __declspec(dllexport) // Visual Studio needs annotating exported functions with this
#else
#define EXPORT_API // XCode does not need annotating exported functions, so define is empty
#endif



extern "C"
{
	void EXPORT_API Start(const char *mode, const char *hostName, int hostPort)
	{
		CInfoManager *infoManager = CInfoManager::GetInstance();
		infoManager->Start(mode, hostName, hostPort);
	}

	void EXPORT_API Stop()
	{
		CInfoManager *infoManager = CInfoManager::GetInstance();
		infoManager->Stop();
	}

	int EXPORT_API GetWatchersCount()
	{
		CInfoManager *infoManager = CInfoManager::GetInstance();
		return infoManager->GetWatchersCount();
	}
}