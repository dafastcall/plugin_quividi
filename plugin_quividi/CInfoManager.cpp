#include "CInfoManager.h"

#include <stdio.h>
#include <errno.h>
#include <time.h>

#ifdef WIN32
	#include <conio.h>
	#include <winsock.h>
#else
	#include <unistd.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <inttypes.h>
	#include <cstring>

	#include <termios.h>
	#include <unistd.h>
	#include <fcntl.h>

	#define SOCKET_ERROR -1
#endif

const unsigned char CInfoManager::MSG_SPLIT_MAGIC_WORD[2] = { 0xFE, 0xCA };

CInfoManager::CInfoManager()
{
	
}

#ifndef WIN32
int CInfoManager::kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}

char CInfoManager::getch() 
{
  struct termios oldt, newt;
  char ch;

  tcgetattr(0, &oldt); /* grab old terminal i/o settings */
  newt = oldt; /* make new settings same as old settings */
  newt.c_lflag &= ~ICANON; /* disable buffered i/o */
  newt.c_lflag &= ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &newt); /* use these new terminal i/o settings now */

  ch = getchar();

  tcsetattr(0, TCSANOW, &oldt);
  return ch;
}
#endif

void CInfoManager::Start(const char *mode, const char *hostName, int hostPort)
{
	if (m_mainThread != nullptr)
	{
		if (!m_executeDone) return;
	}

	// set server parameters
	m_mode = mode;
	m_serverHost = hostName;
	m_serverPort = hostPort;

	m_executeDone = false;
	m_executeThread = true;
	m_mainThread = new std::thread(ThreadFunction);
}

void CInfoManager::Stop()
{
	if (m_mainThread == nullptr) return;
	
	m_executeThread = false;

	if (m_mainThread != nullptr)
	{
		m_mainThread->detach();
		delete m_mainThread;
		m_mainThread = nullptr;
	}

	if (m_watchers != nullptr)
	{
		delete m_watchers;
		m_watchers = nullptr;
	}

	printf("Thread stopped.\n");
}

int CInfoManager::GetWatchersCount()
{
	if (!m_executeThread)
	{
		if (m_executeDone)
		{
			if (m_mainThread != nullptr)
			{
				m_mainThread->detach();
				delete m_mainThread;
			}
			m_mainThread = nullptr;
		}
		return -1;
	}
	
	if (m_watchers == nullptr) return -1;
	else return (int)m_watchers->size();
}

void CInfoManager::ThreadFunction()
{
	int n;
	char buf[1024];
	char name[32];

	struct tm *newtime;
	time_t aclock;
#ifdef WIN32
	__int64 timestamp;
#else
	int64_t timestamp;
#endif

	// Parameters for socket connection retry
	int MAX_NB_TRY = 1;
	int RETRY_DELAY = 3;

	CInfoManager *infoManager = CInfoManager::GetInstance();
	// User-specified configuration
	unsigned char ConfID = CFG_NULL_EVENT_MODE;
	if (infoManager->m_mode=="final")
		ConfID = CFG_FINAL_EVENT_MODE;
	else if (infoManager->m_mode=="periodic")
		ConfID = CFG_PERIODIC_EVENT_MODE;
	else if (infoManager->m_mode == "motion")
		ConfID = CFG_MOTION_EVENT_MODE;
	else if (infoManager->m_mode == "aggregate")
		ConfID = CFG_AGGREGATE_ONLY_MODE;

#ifdef WIN32
	{
		WSADATA wsaData;
		WORD wVersionRequested;
		int err;
		wVersionRequested = MAKEWORD(2, 0); // 2.0 and above version of WinSock
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
		{
			fprintf(stderr, "Couldn't not find a usable WinSock DLL");
			infoManager->m_executeDone = true;
			return;
		}
	}
#endif

	////////////////////////////////////////////////////////////////////////////////////
	// create socket
	printf("Creating socket...\n");
#ifdef WIN32
	SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
#else
	int Socket = socket(AF_INET, SOCK_STREAM, 0);
#endif
	if (Socket == -1)
		fprintf(stderr, "Unable to create socket\n");

	// Prepare server address
#ifdef WIN32
	SOCKADDR_IN ServerAddress;
#else
	sockaddr_in ServerAddress;
#endif
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(infoManager->m_serverPort);
	ServerAddress.sin_addr.s_addr = (inet_addr(infoManager->m_serverHost.c_str()));

	// Connect
	printf("Connecting to %s:%d\n", infoManager->m_serverHost.c_str(), infoManager->m_serverPort);
	for (n = 0; n < MAX_NB_TRY; n++)
	{
#ifdef WIN32
		if (connect(Socket, (SOCKADDR *)&ServerAddress, sizeof(ServerAddress)) == 0) break;
#else
		if (connect(Socket, (sockaddr *)&ServerAddress, sizeof(ServerAddress)) == 0) break;
#endif
		else
		{
			printf("Connection failed... retrying...\n");
#ifdef WIN32
			Sleep(1000 * RETRY_DELAY);
#else
			sleep(1000 * RETRY_DELAY);
#endif
		}

		if (!infoManager->m_executeThread)
		{
			infoManager->m_executeDone = true;
			return;
		}
	}
	if (n == MAX_NB_TRY)
	{
		fprintf(stderr, "Unable to connect to socket\n");
		infoManager->m_executeDone = true;
		return;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Message Buffer (pre-build header)
	infoManager->Header.Magic = MSG_MAGIC_WORD;
	infoManager->Header.Version = MSG_VERSION;
	infoManager->Header.Type = CFG_TYPE;
	infoManager->Header.Reserved = 0x00;
	infoManager->Header.Payload = 0x0004;

	infoManager->ConfigMsg.EventMode = ConfID;
	infoManager->ConfigMsg.Param1 = 0x00;
	infoManager->ConfigMsg.Unused1 = 0x00;
	infoManager->ConfigMsg.Unused2 = 0x00;

	memcpy(buf, (char *)(&infoManager->Header), sizeof(Header));
	memcpy(buf + sizeof(Header), (char *)(&infoManager->ConfigMsg), sizeof(ConfigMsg));

	// Send configuration message
	printf("Connected.\nSending configuration message for %s configuration...\n", infoManager->m_mode.c_str());
	if (send(Socket, buf, sizeof(Header) + sizeof(ConfigMsg), 0) == 0)
	{
		fprintf(stderr, "Could not send configuration message.\n");
#ifdef WIN32
		closesocket(Socket);
#else
		close(Socket);
#endif
		
		infoManager->m_executeDone = true;
		return;
	}

	// allocate watchers array
	if (infoManager->m_watchers == nullptr)
	{
		infoManager->m_watchers = new std::map<int, WatcherMsgStruct>();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Receive messages
	printf("Waiting for data:\n");

	int ix = 0;
	while (true)
	{
		// Wait for magic word
		ix = 0;
		while (true)
		{
			if (!infoManager->m_executeThread) break;

			int count = recv(Socket, buf + ix, 1, 0);
			if (count == 0 || count==SOCKET_ERROR)
			{
				// break execution. connection closed
				infoManager->m_executeThread = false;
			}
			if (((unsigned char)buf[ix] == MSG_SPLIT_MAGIC_WORD[1]) &&
				((unsigned char)buf[ix ^ 0x01] == MSG_SPLIT_MAGIC_WORD[0]))
				break;
			ix ^= 0x01;
		}

		if (!infoManager->m_executeThread) break;
#ifdef WIN32
		int k = _kbhit();
		if (k && _getch() == 's')
#else
		int k = kbhit();
		if (k && getch() == 's')
#endif
		{
			printf("sending status request\n");
			infoManager->Header.Type = STATUS_REQUEST_TYPE;
			memcpy(buf, (char *)(&infoManager->Header), sizeof(Header));
			if (send(Socket, buf, sizeof(Header), 0) == 0)
				fprintf(stderr, "Could not send configuration message.\n");
		}

		// Synchronized; get header
		time(&aclock);
		newtime = localtime(&aclock);
		printf("[%.24s] Message received: ", asctime(newtime));
		recv(Socket, (char *)(&infoManager->Header) + 2, sizeof(Header) - 2, 0);

		// Analyze header and get payload
		if (infoManager->Header.Version != MSG_VERSION)
		{
			printf("Wrong protocol version.\n\n");
			continue;
		}

		printf("UTC offset %d.\n\n", infoManager->Header.UTCOffsetQH);

		// NOTE: although expressed in time_t format, protocol timestamps encode
		// LOCAL time, so use gmtime() for conversion
		//  to a struct tm. Please see Section 5 of the protocol documentation.

		switch (infoManager->Header.Type)
		{
			/////////////////////////////////////////////////////////////////////
		case ACK_TYPE:
			printf("ACK_MSG\n");
			break;

			/////////////////////////////////////////////////////////////////////
		case NACK_TYPE:
			printf("NACK_MSG\n");
			printf("\nWARNING: Socket server is actively refusing packets.\nCheck "
				"that your VidiReports license allows socket use\n");
			break;

			/////////////////////////////////////////////////////////////////////
		case VIEWER_TYPE:
			printf("VIEWER_EVENT_MSG\n");
			if (infoManager->Header.Payload != sizeof(WatcherMsg))
			{
				printf("Wrong payload size! (received %d, expected %zd)\n\n",
					infoManager->Header.Payload, sizeof(WatcherMsg));
				continue;
			}

			recv(Socket, (char *)(&infoManager->WatcherMsg), infoManager->Header.Payload, 0);

			printf("  > WatcherID: %d\n", infoManager->WatcherMsg.WatcherID);
			printf("  > Status: ");
			switch (infoManager->WatcherMsg.Status)
			{
			case 0x00:
				printf("Viewer still unvalidated\n");
				break;
			case 0x01:
				printf("Unvalidated viewer has left (false positive)\n");
				break;
			case 0x02:
				printf("Validated viewer\n");
				break;
			case 0x03:
				printf("Validated viewer left\n");
				break;
			default:
				printf("unknown status\n");
				break;
			}

			printf("  > Viewer is%s watching now\n",
				(infoManager->WatcherMsg.Status & 0x20) ? "" : " NOT");
			printf("  > Number of glances: %d\n", infoManager->WatcherMsg.NumGlances);

			timestamp = infoManager->WatcherMsg.StartTime;
			newtime = gmtime(
				(time_t *)(&timestamp)); // see note above on the use of gmtime
			printf("  > Start time: %s", asctime(newtime));
			printf("  > Total Dwell Time: %.2f sec.\n  > Cumulated Attention Time: "
				"%.2f sec.\n",
				infoManager->WatcherMsg.Duration / 10.0, infoManager->WatcherMsg.AttentionTime / 10.0);
			printf("  > Gender: ");
			switch (infoManager->WatcherMsg.Gender)
			{
			case 0x01:
				printf("Male\n");
				break;
			case 0x02:
				printf("Female\n");
				break;
			default:
				printf("Unknown\n");
				break;
			}
			printf("  > Age group: ");
			switch (infoManager->WatcherMsg.Age)
			{
			case 0x01:
				printf("Child\n");
				break;
			case 0x02:
				printf("Young\n");
				break;
			case 0x03:
				printf("Adult\n");
				break;
			case 0x04:
				printf("Senior\n");
				break;
			default:
				printf("Unknown\n");
				break;
			}

			printf("> Estimated Distance: %d cm.\n", infoManager->WatcherMsg.EstimatedDist);

			// try add or remove viewer to the watchers map			
			switch (infoManager->WatcherMsg.Status)
			{
			case 0x01: // Unvalidated viewer has left (false positive)
			case 0x03: // Validated viewer left
				infoManager->m_watchers->erase(infoManager->WatcherMsg.WatcherID);
				break;

			case 0x00: // Viewer still unvalidated
			case 0x02: // Validated viewer
			default: // unknown status

				infoManager->m_watchers->insert(std::pair<int, WatcherMsgStruct>(infoManager->WatcherMsg.WatcherID, infoManager->WatcherMsg));
				break;
			}

			break;

			/////////////////////////////////////////////////////////////////////
		case MOTION_TYPE:
			printf("MOTION_EVENT_MSG\n");
			if (infoManager->Header.Payload != sizeof(MotionMsg))
			{
				printf("Wrong payload size! (received %d, expected %zd)\n\n",
					infoManager->Header.Payload, sizeof(MotionMsg));
				continue;
			}

			recv(Socket, (char *)(&infoManager->MotionMsg), infoManager->Header.Payload, 0);

			printf("  > WatcherID: %d\n", infoManager->MotionMsg.WatcherID);
			printf("  > Status: ");
			if (infoManager->MotionMsg.Status & 0x10)
				printf("SELECTED ");
			switch (infoManager->MotionMsg.Status & 0x0F)
			{
			case 0x00:
				printf("Viewer still unvalidated\n");
				break;
			case 0x01:
				printf("Unvalidated viewer has left (false positive)\n");
				break;
			case 0x02:
				printf("Validated viewer\n");
				break;
			case 0x03:
				printf("Validated viewer left\n");
				break;
			default:
				printf("unknown status\n");
				break;
			}

			printf("  > Viewer is%s watching now\n",
				(infoManager->MotionMsg.Status & 0x20) ? "" : " NOT");

			printf("  >  Face Position: %.1f%% horizontal, %.1f%% vertical\n",
				100. * infoManager->MotionMsg.X_Position / float(0xFFFF),
				100. * infoManager->MotionMsg.Y_Position / float(0xFFFF));
			printf("  >  Face Size: %.1f%% x %.1f%%\n",
				100. * infoManager->MotionMsg.Width / float(0xFFFF),
				100. * infoManager->MotionMsg.Height / float(0xFFFF));
			printf("  > Gender: ");
			switch (infoManager->MotionMsg.Gender)
			{
			case 0x01:
				printf("Male\n");
				break;
			case 0x02:
				printf("Female\n");
				break;
			case 0x03:
				printf("Unknown\n");
				break;
			}
			printf("> Estimated Distance: %d cm.\n", infoManager->MotionMsg.EstimatedDist);

			break;

			/////////////////////////////////////////////////////////////////////
		case OTS_TYPE:
			printf("OTS_EVENT_MSG:\n");
			if (infoManager->Header.Payload != sizeof(OTSMsg))
			{
				printf("Wrong payload size! (received %d, expected %zd)\n\n",
					infoManager->Header.Payload, sizeof(OTSMsg));
				continue;
			}

			recv(Socket, (char *)(&infoManager->OTSMsg), infoManager->Header.Payload, 0);

			timestamp = infoManager->OTSMsg.StartTime;
			newtime = gmtime((time_t *)(&timestamp));
			printf("  > OTS status: %d\n", infoManager->OTSMsg.Status);
			printf("  > %d people over the %d sec interval starting at %s\n",
				infoManager->OTSMsg.Count, infoManager->OTSMsg.Duration, asctime(newtime));
			break;

			/////////////////////////////////////////////////////////////////////
		case OVERHEAD_TYPE:
			printf("OVERHEAD_EVENT_MSG:\n");
			if (infoManager->Header.Payload != sizeof(OverheadEvent))
			{
				printf("Wrong payload size! (received %d, expected %zd)\n\n",
					infoManager->Header.Payload, sizeof(OverheadEvent));
				continue;
			}

			recv(Socket, (char *)(&infoManager->OverheadEvent), infoManager->Header.Payload, 0);

			timestamp = infoManager->OverheadEvent.StartTime;
			newtime = gmtime((time_t *)(&timestamp));
			printf("  > Start time: %s", asctime(newtime));
			printf("  > Total Passage Time: %.2f sec.\n",
				infoManager->OverheadEvent.Duration / 10.0);
			printf("  > Avg Size (radius): %d pixels\n",
				(int)infoManager->OverheadEvent.AvgRadius);
			printf("  > Trajectory keypoints (image size %dx%d):\n     ",
				infoManager->OverheadEvent.RefSize[0], infoManager->OverheadEvent.RefSize[1]);
			for (k = 0; k < infoManager->OverheadEvent.NumPoints - 1; k++)
			{
				printf("(%d %d) -> ", infoManager->OverheadEvent.Trajectory[2 * k],
					infoManager->OverheadEvent.Trajectory[2 * k + 1]);
				if ((k + 1) % 5 == 0)
					printf("\n     ");
			}
			printf("(%d %d)\n", infoManager->OverheadEvent.Trajectory[2 * k],
				infoManager->OverheadEvent.Trajectory[2 * k + 1]);

			break;

			/////////////////////////////////////////////////////////////////////
		case GATE_COUNT_TYPE:
			printf("GATE_COUNT_MSG:\n");
			if (infoManager->Header.Payload != sizeof(GateCount))
			{
				printf("Wrong payload size! (received %d, expected %zd)\n\n",
					infoManager->Header.Payload, sizeof(GateCount));
				continue;
			}

			recv(Socket, (char *)(&infoManager->GateCount), infoManager->Header.Payload, 0);

			timestamp = infoManager->GateCount.StartTime;
			newtime = gmtime((time_t *)(&timestamp));
			strncpy(name, (char *)infoManager->GateCount.Name, 16);
			name[16] = '\0';

			printf("  > Gate %s (#%d, %d total)\n", name, infoManager->GateCount.ID,
				infoManager->GateCount.NumGates);
			printf("  > Start time: %s", asctime(newtime));
			printf("  > Duration: %d seconds\n", infoManager->GateCount.Duration);
			printf("  > Crossings in interval:\n");
			printf("         IN: %d\n        OUT: %d\n", infoManager->GateCount.InCount,
				infoManager->GateCount.OutCount);
			if (infoManager->GateCount.Flags & 0x01)
				printf("  > *** Gate geometry just changed ***\n");
			break;

			/////////////////////////////////////////////////////////////////////
		case GATE_SNAPSHOT_TYPE:
			printf("GATE_SNAPSHOT_MSG:\n");
			if (infoManager->Header.Payload != sizeof(GateSnapshot))
			{
				printf("Wrong payload size! (received %d, expected %zd)\n\n",
					infoManager->Header.Payload, sizeof(GateSnapshot));
				continue;
			}

			recv(Socket, (char *)(&infoManager->GateSnapshot), infoManager->Header.Payload, 0);

			timestamp = infoManager->GateSnapshot.StartTime;
			newtime = gmtime((time_t *)(&timestamp));
			printf("  > People over gate %d since %s", infoManager->GateSnapshot.ID,
				asctime(newtime));
			printf("         IN: %d\n        OUT: %d\n", infoManager->GateSnapshot.TotalInCount,
				infoManager->GateSnapshot.TotalOutCount);
			break;

			/////////////////////////////////////////////////////////////////////
		default:
			// Invalid packet
			printf("\n\n\n*********************************************\n");
			printf("Invalid/Unknown Packet (%d)\n", infoManager->Header.Type);
			printf("*********************************************\n\n");
			continue;
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Disconnect
#ifdef WIN32
	closesocket(Socket);
#else
	close(Socket);
#endif

	if (infoManager->m_watchers!=nullptr) delete infoManager->m_watchers;
	infoManager->m_watchers = nullptr;

	infoManager->m_executeDone = true;
}
