#pragma once

#include "clb_main.h"
#include "EncodeAgent.h"
#include "DecodeAgent.h"

class EncodeAgent;
class DecodeAgent;

class CLBTunnelAgent
{
public:
	CLBTunnelAgent(PacketFilter* filter)
	{
		parentFilter = filter;
		init();
	};
	bool Encode(FILTER_NBL_PACKET originalPacket);
	bool Decode(PFILTER_NBL_FETCH_PACKET fetchPkt, unsigned long size);
	int BlockSize_K;
	void uninitialize();
	unsigned short * FeedbackVPAckedPackets;
	unsigned short * VPPacketsSent;
	unsigned short * VPPacketsAcked;
	unsigned short * VPPacketsOnFly;
	double * VPRateEstimation;
	clock_t * VPLastUpdateTime;
	double * VPCost;
	int countExploredOnceVP;
	int usedVP;
	int clb_decode_times;
	int clb_decode_timeout_times;
	int parity_generated;


	void PrintVPWindow(int force);
	void PrintPacket(FILTER_NBL_PACKET pkt, char SendorReceive[10]);
	void PrintPacket(FILTER_NBL_PACKET pkt, char SendorReceive[10], int force);
	void PrintReorderBuffer(int force);
	void PrintEncodeBuffer(int force);
	void print_bytes(const void *object, size_t size)
	{
	  size_t i;
	  printf("[ ");
	  for(i = 0; i < size; i++)
	  {
		printf("%02x ", ((const unsigned char *) object)[i] & 0xff);
	  }
	  printf("]\n");
	};
	int flowID;

	std::mutex encodePacket_mutex;
	std::mutex decodePacket_mutex;
	std::mutex vp_mutex;


	void printIndex(FILTER_NBL_PACKET pkt, char SendorReceive[128]);

protected:
	void init();
	PacketFilter* parentFilter;
	EncodeAgent* encodeAgent_;
	DecodeAgent* decodeAgent_;
};
