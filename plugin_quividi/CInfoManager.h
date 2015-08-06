#ifndef _CINFO_MANAGER_
#define _CINFO_MANAGER_

#include <string>
#include <thread>
#include <map>
#include <atomic>

class CInfoManager
{
	// Different event that can be received
	static const unsigned char CFG_NULL_EVENT_MODE = 0x00; ///< Client will not receive events
	static const unsigned char CFG_FINAL_EVENT_MODE = 0x01; ///< Client will receive final events (viewer, gates, overhead, OTS)
	static const unsigned char CFG_PERIODIC_EVENT_MODE = 0x02; ///< Client will receive periodic events on tracked objects plus

	///gates, overhead and ots count messages
	static const unsigned char CFG_MOTION_EVENT_MODE = 0x03; ///< Client will only receive motion events for viewers
	static const unsigned char CFG_AGGREGATE_ONLY_MODE = 0x04; ///< Client will only receive ots count messages or gates messages

	// Message types
	static const unsigned char CFG_TYPE = 0x20;
	static const unsigned char VIEWER_TYPE = 0x30;
	static const unsigned char MOTION_TYPE = 0x40;
	static const unsigned char ACK_TYPE = 0x06;
	static const unsigned char NACK_TYPE = 0x07;
	static const unsigned char OTS_TYPE = 0x60;
	static const unsigned char OVERHEAD_TYPE = 0x80;
	static const unsigned char GATE_COUNT_TYPE = 0x81;
	static const unsigned char GATE_SNAPSHOT_TYPE = 0x82;
	static const unsigned char STATUS_REQUEST_TYPE = 0x13;
	static const unsigned char STATUS_MSG_TYPE = 0x14;

	// Different useful values of Quividi protocol header
	static const int HEADER_SIZE = 0x08;
	static const short int MSG_MAGIC_WORD = (short int)0xCAFE; // Magic word
	static const unsigned char MSG_SPLIT_MAGIC_WORD[2]; // Quividi messages are in little-endian
	static const unsigned char MSG_VERSION = 0x02; // Latest version of the protocol
	static const unsigned char MSG_RESERVED = 0x00;

	////////////////////////////////////////////////////////////////////////////////////
	// Data structures
#pragma pack(1)

	struct HeaderStruct
	{
		unsigned short Magic;
		unsigned char Version;
		unsigned char Type;
		char UTCOffsetQH;
		unsigned char Reserved;
		unsigned short Payload;
	} Header;

	struct ConfigMsgStruct
	{
		unsigned char EventMode;
		unsigned char Param1;
		unsigned char Unused1;
		unsigned char Unused2;
	} ConfigMsg;

	struct WatcherMsgStruct
	{
		unsigned int StartTime;
		unsigned int Duration;
		unsigned int AttentionTime;
		unsigned char Gender;
		unsigned char Age;
		unsigned char Reserved[6];
		unsigned int WatcherID;
		unsigned int Status;
		unsigned short EstimatedDist;
		unsigned char NumGlances;
		unsigned char Unused;
	} WatcherMsg;

	struct MotionMsgStruct
	{
		unsigned short X_Position;
		unsigned short Y_Position;
		unsigned short Width;
		unsigned short Height;
		unsigned short EstimatedDist;
		unsigned int WatcherID;
		unsigned int Status;
		unsigned char Gender;
		unsigned char Age; // unreliable, preferrably do not use
		unsigned char Unused[3];
	} MotionMsg;

	struct OTSMsgStruct
	{
		unsigned int StartTime; ///< Timestamp
		unsigned int Duration;  ///< Duration of analysis window (in seconds)
		unsigned int Count;     ///< OTS
		unsigned short Detects; ///< Number of validated detections in the interval
		unsigned char Status;   ///< Status: 0 - KO; 1 - OK; 2 - HALT
		unsigned char Unused[25];
	} OTSMsg;

	struct OverheadEventStruct
	{
		unsigned int StartTime; ///< Entry Timestamp
		unsigned int Duration;  ///< Duration in 1/10

		unsigned short AvgRadius;  ///< Object's average equivalent radius
		unsigned short RefSize[2]; ///< Reference image size for previous values
		unsigned short InGateFlag; ///< Bits at 1 indicate gates crossed in the IN direction
		unsigned short OutGateFlag; ///< Bits at 1 indicate gates crossed in the IN direction
		unsigned short Flags;
		unsigned char NumPoints;       ///< Number of used trajectory points
		unsigned short Trajectory[32]; ///< Trajectory points buffer
	} OverheadEvent;

	struct GateCountStruct
	{
		unsigned int StartTime; ///< Timestamp
		unsigned int Duration;  ///< Duration of analysis window (in seconds)

		unsigned char ID;          ///< Numeric ID
		unsigned char Name[16];    ///< Assigned name (string)
		unsigned short Line[4];    ///< Start coordinates (start, stop, x, y; IN
		///direction is by left-hand rule)
		unsigned short Width;      ///< Width
		unsigned short RefSize[2]; ///< Reference image size for previous coordinates

		unsigned int InCount;  ///< IN crossings
		unsigned int OutCount; ///< OUT crossings

		unsigned char NumGates; ///< Total number of active gates
		unsigned short Flags;   ///< Flags. Bit 0 signals a recent change of geometry

		unsigned short ActiveFlags; ///< Bits set to 1 signal gates that have been crossed

		unsigned char Unused[28];
	} GateCount;

	struct GateSnapshotStruct
	{
		unsigned int StartTime; ///< Timestamp of last reset
		unsigned int Timestamp; ///< Current timestamp

		unsigned char ID;
		unsigned int TotalInCount;  ///< IN crossings
		unsigned int TotalOutCount; ///< OUT crossings
	} GateSnapshot;

	std::atomic<bool> m_executeThread = true;
	std::atomic<bool> m_executeDone = false;

	std::thread *m_mainThread = nullptr;

	std::map<int, WatcherMsgStruct> *m_watchers = nullptr;
public:
	
	std::string m_mode = "final";
	std::string m_serverHost = "127.0.0.1"; // localhost
	int m_serverPort = 1974; // Quividi server Port

	void Start(const char *mode, const char *hostName, int hostPort);
	void Stop();
	int GetWatchersCount();

	static CInfoManager* GetInstance()
	{
		static CInfoManager instance;
		return &instance;
	}

private:
	CInfoManager();
	
	CInfoManager(CInfoManager const&) = delete;
	void operator=(CInfoManager const&) = delete;

	static void ThreadFunction();
};

#endif
