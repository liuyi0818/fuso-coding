#ifndef clb_decode_h
#define clb_decode_h

#include "clb_main.h"
#include "TemplateTimer.h"

class CLBTunnelAgent;

class DecodeAgent
{
public:
	DecodeAgent(PacketFilter* filter, CLBTunnelAgent* CLBTunnelAgent)
	{
		parentFilter = filter;
		parentCLBTunnelAgent=CLBTunnelAgent;
		init();
	};
	bool Decode(PFILTER_NBL_FETCH_PACKET originalPacket, unsigned long size);
	void uninitialize();

	int expectHandOverGenID;
	int expectHandOverGenIndex;
	int headReorderBuffer;

	int packetsCountInReorderBuffer;
	struct ReorderBuffer{
		FILTER_NBL_PACKET* buffer;
		bool* ifOccupied;
		int head; ////head of reorderBuffer_
		int count;///How many packets are in reorderbuffer right now.
		int capacity;

		int genID;
		int blockSize;
		int headIndex;
		int lastGenOriginalIndex;
	} *reorderBuffer_;/////////////reorderBuffer_[ReorderGenNumber][BlockSize_N]


protected:
	PacketFilter* parentFilter;
	CLBTunnelAgent* parentCLBTunnelAgent;
	void init();

	void DecapPacket(FILTER_NBL_PACKET pkt);
	void checkReorderBuffer();
	void decodeReorderBuffer();

	///These two functions clear reorder gen. But don't set new genID for this buffer
	void handOverAndClearGen(int bufferID);
	void clearGen(int bufferID);

	bool insertReorderBuffer(PFILTER_NBL_FETCH_PACKET pkt, unsigned long size);

	void recoverMissingPacket(int bufferID,int position, FILTER_NBL_PACKET firstParity);/////Recover missing packet in HeadReorderGeninWhichBuffer, through decoding and pass it to the recv_packet. 

	void recoverMissingPacketQ(int bufferID, FILTER_NBL_PACKET firstParity);/////RS algorithm. Recover missing packet in HeadReorderGeninWhichBuffer, through decoding and pass it to the recv_packet.

	PFILTER_NBL_RECV_PACKET recv_packet;/////Recv packet to store packets waiting to be received by OS
	int maxRecvCount;
	unsigned long recv_size;


private:
    void decodeTimeout();
	TTimer<DecodeAgent> decodeTimer_;
};

#endif
