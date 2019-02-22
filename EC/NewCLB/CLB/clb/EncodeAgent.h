#ifndef clb_encode_h
#define clb_encode_h

#include "clb_main.h"
#include "TemplateTimer.h"

class CLBTunnelAgent;

class EncodeAgent
{
public:
	EncodeAgent(PacketFilter* filter, CLBTunnelAgent* CLBTunnelAgent)
	{
		parentFilter = filter;
		parentCLBTunnelAgent=CLBTunnelAgent;
		init();
	};
	bool Encode(FILTER_NBL_PACKET originalPacket);
	void uninitialize();

	struct ENCODE_BUFFER
	{
		FILTER_NBL_PACKET* buffer;
		bool* ifOccupied;
		int head; 
		int tail;  
		int length;
		int capacity;
	}encodeBuffer;

	unsigned char currentGenID;
	unsigned short currentGenIndex;
	unsigned short currentFeedbackVP;
	int currentGeneratedParityNumber;
	int * VPSelectCondition;
	int maxLenInCurrentGen;

protected:
	PacketFilter* parentFilter;
	CLBTunnelAgent* parentCLBTunnelAgent;
	void init();

	void setPacket(FILTER_NBL_PACKET pkt, IP_HEADER* old_ip_header, bool ifParity);
	void sendParityPacket();

	void createParityPacket(FILTER_NBL_PACKET pkt);/////Create a parity packet and set the data to pkt. 
				//The parity packet has the same ethernet header as the former packet, and the rest of data as the bitwise and of all the former packets!

	void createParityPacketQ(FILTER_NBL_PACKET pkt);/////////Create a parity packet and set the data to pkt. 
				//The parity packet has the same ethernet header as the former packet, and the rest of data as the reed-solomun encoded data!

	unsigned short setVP(FILTER_NBL_PACKET pkt);
	int createNewVP();
	unsigned short findBestVP();
	unsigned short findNextFeedbackVP(unsigned short vp);
	void clearWorstVP();

	PFILTER_NBL_SEND_PACKET send_packet;/////Send packet for sending parity packet
	unsigned long send_size;
	FILTER_NBL_PACKET parityPacket;

private:
    void encodeTimeout();

    TTimer<EncodeAgent> encodeTimer ;
};

#endif
