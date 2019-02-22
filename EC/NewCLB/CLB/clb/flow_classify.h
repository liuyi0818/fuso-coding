#pragma once

#include "clb_main.h"
#include "clb_tunnel_agent.h"

class CLBTunnelAgent;

class FlowClassifier
{
public:
	FlowClassifier(PacketFilter* filter)
	{
		parentFilter = filter;
		init();
	};
	~FlowClassifier();
	CLBTunnelAgent* getCLBTunnelAgent(unsigned int otherIP);
	void shrinkFlowTable();

protected:
	PacketFilter* parentFilter;
	void init();
	void expandOneFlowTableEntry(unsigned int otherIP);
	struct FLOW_TABLE
	{
		unsigned int otherEndIPAddr; 
		CLBTunnelAgent* clbTunnelAgent; 
		bool age; //////Every hitting packet will set the bit.  Timer will check if the bit was set, and reset it 
		bool valid;
		unsigned int hitTimes;
	}* flowTable;
	unsigned short maxActiveFlowNumber;
	unsigned short invalidEntryNumber;
};
