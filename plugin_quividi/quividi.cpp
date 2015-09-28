#include <dlfcn.h>
#include <iostream>
#include <cstddef>
#include <cstdio>
#include <stdio.h>

#ifdef WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif

using namespace std;

typedef void (*Start)(const char *mode, const char *hostName, int hostPort);
typedef void (*Stop)();
typedef int (*GetWatchersCount)();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"


int main()
{
	printf("Opening Quividi plugin...\n");

	char cCurrentPath[FILENAME_MAX];

	if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
	{
		return errno;
	}

	cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';

	std::string libraryName = cCurrentPath;
	//libraryName += "/plugin_quividi_x64.so";
	libraryName = "plugin_quividi.so";

	printf("Library name: %s\n", libraryName.c_str());

  	void *handle = dlopen(libraryName.c_str(), RTLD_LAZY);
	if (!handle) 
	{ 
		printf("dlopen failure: %s\n", dlerror()); 
        exit (EXIT_FAILURE); 
	};

	Start startFunction = NULL;
	Stop stopFunction = NULL;
	GetWatchersCount getCountFunction = NULL;

	printf("Library opened.\n");


	startFunction = (Start)dlsym(handle, "Start");
	if (!startFunction) 
	{ 
		printf("dlsym failure: %s\n", dlerror()); 
        exit (EXIT_FAILURE); 
	};

	stopFunction = (Stop)dlsym(handle, "Stop");
	if (!stopFunction) 
	{ 
		printf("dlsym failure: %s\n", dlerror()); 
        exit (EXIT_FAILURE); 
	};

	getCountFunction = (GetWatchersCount)dlsym(handle, "GetWatchersCount");
	if (!getCountFunction) 
	{ 
		printf("dlsym failure: %s\n", dlerror()); 
        exit (EXIT_FAILURE); 
	};

	startFunction("periodic", "127.0.0.1", 1974);

	do
	{
		int count = getCountFunction();
		printf("Watchers count: %d\n", count); 
		sleep(1);
	}
	while(true);

	stopFunction();

	dlclose(handle);

	printf("Exiting application...\n");
	return 0;
}

#pragma GCC diagnostic pop
