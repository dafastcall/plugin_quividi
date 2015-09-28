// plugin_quividi.cpp : Defines the exported functions for the DLL application.
//
#include "CInfoManager.h"

#if WIN32
#define EXPORT_API __declspec(dllexport) 
#else
#define EXPORT_API
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
