#include "clb_tunnel_agent.h"
#include "get_time.h"

void CLBTunnelAgent::init()
{
	printf("Intialize CLBTunnelAgent!\n");
	BlockSize_K = BlockSize_N - BlockSize_MaxP;

	encodeAgent_ = new EncodeAgent(parentFilter,this);
	decodeAgent_ = new DecodeAgent(parentFilter,this);

	VPCost=new double [VPNumber];
	memset(VPCost,0,VPNumber*sizeof(double));
	VPRateEstimation=new double [VPNumber];
	memset(VPRateEstimation,0,VPNumber*sizeof(double));
	VPLastUpdateTime=new clock_t [VPNumber];
	memset(VPLastUpdateTime,0,VPNumber*sizeof(clock_t));
	
	VPPacketsSent=new unsigned short [VPNumber];
	memset(VPPacketsSent,0,VPNumber*sizeof(unsigned short));
	VPPacketsAcked=new unsigned short [VPNumber];
	memset(VPPacketsAcked,0,VPNumber*sizeof(unsigned short));
	VPPacketsOnFly=new unsigned short [VPNumber];
	memset(VPPacketsOnFly,0,VPNumber*sizeof(unsigned short));
	FeedbackVPAckedPackets=new unsigned short [VPNumber];
	memset(FeedbackVPAckedPackets,0,VPNumber*sizeof(unsigned short));

	usedVP=0;
	countExploredOnceVP=0;
	clb_decode_times=0;
	clb_decode_timeout_times = 0;
	parity_generated=0;
}

bool CLBTunnelAgent::Encode(FILTER_NBL_PACKET originalPacket)
{
	return encodeAgent_->Encode(originalPacket);
}

bool CLBTunnelAgent::Decode(PFILTER_NBL_FETCH_PACKET originalPacket, unsigned long size)
{
	return decodeAgent_->Decode(originalPacket,size);
}

//void CLBTunnelAgent::uninitialize()
//{
//	printf("Uintialize CLBTunnelAgent!\n");
//fflush(stdout);
//	encodeAgent_->uninitialize();
//	decodeAgent_->uninitialize();
//	delete encodeAgent_;
//	delete decodeAgent_;
//	delete [] VPCost;
//	delete [] VPRateEstimation;
//	delete [] VPLastUpdateTime;
//	delete [] VPPacketsSent;
//	delete [] VPPacketsAcked;
//	delete [] VPPacketsOnFly;
//	delete [] FeedbackVPAckedPackets;
//}
//

void CLBTunnelAgent::PrintVPWindow(int force)
{
	if(!IF_PRINT_DEBUG && force==0)
	{
		return;
	}
	struct timeval tv;
	time_t nowtime;
	struct tm nowtm={0};
	char tmbuf[64], buf[64];

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	localtime_s(&nowtm,&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", &nowtm);
	sprintf_s(buf, sizeof buf, "%s.%06d", tmbuf, tv.tv_usec);

	printf("%s----(flow %d): usedVP=%d, countExploredOnceVP=%d\n"
		,buf
		,flowID
		,usedVP
		,countExploredOnceVP);

	for(int i=0;i<VPNumber;i++)
	{
		printf("FLOW %d VP[%d]: lastUpdateTime=%ld, rate=%3.2e, Sent=%u, Acked=%u, OnFly=%u, Cost=%3.2e, FB_Acked=%u\n"
			,flowID
			,i
			,VPLastUpdateTime[i]
			,VPRateEstimation[i]
			,VPPacketsSent[i]
			,VPPacketsAcked[i]
			,VPPacketsOnFly[i]
			,VPCost[i]
			,FeedbackVPAckedPackets[i]);
	}
}

void CLBTunnelAgent::PrintPacket(FILTER_NBL_PACKET pkt, char SendorReceive[128])
{
	PrintPacket(pkt, SendorReceive, 0);
}

void CLBTunnelAgent::printIndex(FILTER_NBL_PACKET pkt, char SendorReceive[128])
{
	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength() + 42);
	
	printf("%s\t%d\t%u\t%d\t%d\t%d\n"
		, SendorReceive
		, decodeAgent_->expectHandOverGenIndex
		, ntohs(clb_header->gen_index)
		, decodeAgent_->reorderBuffer_[decodeAgent_->headReorderBuffer].head
		, decodeAgent_->reorderBuffer_[decodeAgent_->headReorderBuffer].headIndex
		, decodeAgent_->reorderBuffer_[decodeAgent_->headReorderBuffer].count);
}

void CLBTunnelAgent::PrintPacket(FILTER_NBL_PACKET pkt, char SendorReceive[128], int force)
{
	if(!IF_PRINT_DEBUG && force==0)
	{
		return;
	}

	IP_HEADER* out_ip_header;
	out_ip_header = (IP_HEADER*) (pkt.GetBuffer() + pkt.GetPrefixLength() + 14);
	UDP_HEADER* udp_tunnel_header;
	udp_tunnel_header = (UDP_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength() + 34);
	CLB_HEADER* clb_header;
	clb_header= (CLB_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength() + 42);
	IP_HEADER* inner_ip_header;
	if(clb_header->ifParity==0)
	{
		inner_ip_header = (IP_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength() + 50);
	}
	else
	{
		inner_ip_header = (IP_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength() + 52);
	}

	struct timeval tv;
	time_t nowtime;
	struct tm nowtm={0};
	char tmbuf[64], buf[64];

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	localtime_s(&nowtm,&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", &nowtm);
	sprintf_s(buf, sizeof buf, "%s.%06d", tmbuf, tv.tv_usec);

	in_addr outSrc;
    outSrc.S_un.S_addr = out_ip_header->ip_srcaddr;
    in_addr outDst;
    outDst.S_un.S_addr = out_ip_header->ip_destaddr;
	char strOutSrc[256];
	sprintf_s(strOutSrc,"%s", inet_ntoa(outSrc));
	char strOutDst[256];
	sprintf_s(strOutDst,"%s", inet_ntoa(outDst));
	in_addr inSrc;
    inSrc.S_un.S_addr = inner_ip_header->ip_srcaddr;
    in_addr inDst;
	inDst.S_un.S_addr = inner_ip_header->ip_destaddr;
	char strInSrc[256];
	sprintf_s(strInSrc,"%s", inet_ntoa(inSrc));
	char strInDst[256];
	sprintf_s(strInDst,"%s", inet_ntoa(inDst));

	printf("%s-----(flow %d): %s packet IP[%s-%s](%s-%s), out_ip_ID:%u, inner_ip_ID:%u\n",
		buf,
		flowID	,
		SendorReceive,
		strOutSrc,
		strOutDst,
		strInSrc,
		strInDst,
		ntohs(out_ip_header->ip_id),
		ntohs(inner_ip_header->ip_id)
		);

	if (inner_ip_header->ip_protocol == 0x06)
	{
		TCP_HEADER* inner_tcp_header;
		if (clb_header->ifParity == 0)
		{
			inner_tcp_header = (TCP_HEADER*)( pkt.GetBuffer() + pkt.GetPrefixLength() + 70);
		}
		else
		{
			inner_tcp_header = (TCP_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength() + 72);
		}
		printf("seqNo=%ul,ackNo=%ul\n"
			, ntohl(inner_tcp_header->sequence)
			, ntohl(inner_tcp_header->acknowledge));
	}

	printf("vPath=%u,genID=%u,genIndex=%u,feedBackVP=%u,feedBackVP_ACKedPackets=%u\n"
		, ntohs(udp_tunnel_header->src_port)
		, clb_header->gen_id
		, ntohs(clb_header->gen_index)
		, ntohs(clb_header->feedback_vp)
		, ntohs(clb_header->feedback_vp_acked_packets));
	if (clb_header->ifParity == 1)
	{
		CLB_PARITY_OPTION clb_option;
		memcpy(&clb_option,pkt.GetBuffer() + pkt.GetPrefixLength() + 50,sizeof(CLB_PARITY_OPTION));
		printf("++++Parity: parityIndex=%u,headGenIndex=%d\n"
			, clb_header->parity_index
			,ntohs(clb_option.head_gen_index));
	}
}

void CLBTunnelAgent::PrintEncodeBuffer(int force)
{
	if(!IF_PRINT_DEBUG && force==0)
	{
		return;
	}

	struct timeval tv;
	time_t nowtime;
	struct tm nowtm={0};
	char tmbuf[64], buf[64];

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	localtime_s(&nowtm,&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", &nowtm);
	sprintf_s(buf, sizeof buf, "%s.%06d", tmbuf, tv.tv_usec);

	printf("%s-----(flow %d): EncodeBuffer head=%d, tail=%d, length=%d, capacity=%d! currentGen=%u.%u, maxLenInCurrentGen=%d\n"
		,buf
		,flowID
		,encodeAgent_->encodeBuffer.head
		,encodeAgent_->encodeBuffer.tail
		,encodeAgent_->encodeBuffer.length
		,encodeAgent_->encodeBuffer.capacity
		,encodeAgent_->currentGenID
		,encodeAgent_->currentGenIndex
		,encodeAgent_->maxLenInCurrentGen);
	for(int i=0;i<BlockSize_N-BlockSize_MaxP;i++)
	{
		if(encodeAgent_->encodeBuffer.ifOccupied[i]==false)
		{
			printf("FLOW %d: EncodeBuffer[%d]=NULL\n",flowID,i);
		}
		else
		{
			FILTER_NBL_PACKET pkt=encodeAgent_->encodeBuffer.buffer[i];
			IP_HEADER out_ip_header;
			memcpy(&out_ip_header,pkt.GetBuffer() + pkt.GetPrefixLength() + 14,sizeof(IP_HEADER));
			printf("FLOW %d: EncodeBuffer[%d]=%u(len=%lu)\n"
				,flowID
				,i
				,ntohs(out_ip_header.ip_id)
				,encodeAgent_->encodeBuffer.buffer[i].GetLength());
		}
	}
}

void CLBTunnelAgent::PrintReorderBuffer(int force)
{
	if(!IF_PRINT_DEBUG && force==0)
	{
		return;
	}
	struct timeval tv;
	time_t nowtime;
	struct tm nowtm={0};
	char tmbuf[64], buf[64];

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	localtime_s(&nowtm,&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", &nowtm);
	sprintf_s(buf, sizeof buf, "%s.%06d", tmbuf, tv.tv_usec);

	printf("%s-----(flow %d): ReorderBuffer headReorderBuffer=%d,expectHandOverGenID=%d,expectHandOverGenIndex=%d,packetsCountInReorderBuffer=%d!\n"
		,buf
		,flowID
		,decodeAgent_->headReorderBuffer
		,decodeAgent_->expectHandOverGenID
		,decodeAgent_->expectHandOverGenIndex
		,decodeAgent_->packetsCountInReorderBuffer);

	for(int i=0;i<ReorderGenNumber;i++)
	{
		printf("FLOW %d: ReorderBuffer[%d].head=%d,count=%d,genID=%d,blockSize=%d,headIndex=%d,lastGenOriginalIndex=%d\n"
			,flowID
			,i
			,decodeAgent_->reorderBuffer_[i].head
			,decodeAgent_->reorderBuffer_[i].count
			,decodeAgent_->reorderBuffer_[i].genID
			,decodeAgent_->reorderBuffer_[i].blockSize
			,decodeAgent_->reorderBuffer_[i].headIndex
			,decodeAgent_->reorderBuffer_[i].lastGenOriginalIndex
			);
		for(int j=0;j<BlockSize_N;j++)
		{
			if(decodeAgent_->reorderBuffer_[i].ifOccupied[j]==false)
			{
				printf("FLOW %d: ReorderBuffer[%d][%d]=NULL\n",flowID,i,j);
			}
			else
			{
				IP_HEADER out_ip_header;
				memcpy(&out_ip_header,decodeAgent_->reorderBuffer_[i].buffer[j].GetBuffer() + decodeAgent_->reorderBuffer_[i].buffer[j].GetPrefixLength() + 14,sizeof(IP_HEADER));
				CLB_HEADER clb_header;
				memcpy(&clb_header,decodeAgent_->reorderBuffer_[i].buffer[j].GetBuffer() + decodeAgent_->reorderBuffer_[i].buffer[j].GetPrefixLength() + 42,sizeof(CLB_HEADER));
				printf("FLOW %d: ReorderBuffer[%d][%d]=%u(%u)(len=%lu)\n"
					,flowID
					,i,j
					,ntohs(out_ip_header.ip_id)
					,ntohs(clb_header.gen_index)
					, decodeAgent_->reorderBuffer_[i].buffer[j].GetLength());
			}
		}
	}
}

//
//void CLBTunnelAgent::PrintReorderBuffer()
//{
//	/*if(!IF_PRINT_DEBUG)
//	{
//		return;
//	}*/
//
//	return;
//
//	unsigned short  currentHeadReorderGen=decodeAgent_->getCurrentHeadReorderGen();
//	int HeadReorderGeninWhichBuffer=decodeAgent_->getHeadReorderGeninWhichBuffer();
//	int packetsCountInReorderBuffer=decodeAgent_->getPacketsCountInReorderBuffer();
//
//	printf("currentHeadReorderGen=%u, HeadReorderGeninWhichBuffer=%d, packetsCountInReorderBuffer=%d\n"
//		,currentHeadReorderGen
//		,HeadReorderGeninWhichBuffer
//		,packetsCountInReorderBuffer);
//fflush(stdout);
//
//	for(int i=0;i<ReorderGenNumber;i++)
//	{
//		for(int j=0;j<BlockSize_N;j++)
//		{
//			if(decodeAgent_->reorderBuffer_[i].ifOccupied[j]==false)
//			{
//				printf("ReorderBuffer[%d][%d]=NULL\n",i,j);
//fflush(stdout);
//			}
//			else
//			{
//				IP_HEADER ip_header;
//				memcpy(&ip_header,decodeAgent_->reorderBuffer_[i].buffer[j].m_packet[0].GetBuffer() + decodeAgent_->reorderBuffer_[i].buffer[j].m_packet[0].GetPrefixLength() + 14,sizeof(IP_HEADER));
//				IP_HEADER inner_ip_header;
//				memcpy(&inner_ip_header,decodeAgent_->reorderBuffer_[i].buffer[j].m_packet[0].GetBuffer() + decodeAgent_->reorderBuffer_[i].buffer[j].m_packet[0].GetPrefixLength() + 46,sizeof(IP_HEADER));
//
//				printf("ReorderBuffer[%d][%d]=out_id{%u},inner_id{%u}\n",i,j,ntohs(ip_header.ip_id),ntohs(inner_ip_header.ip_id));
//fflush(stdout);
//			}
//		}
//		int head=decodeAgent_->reorderBuffer_[i].head;
//		int tail=decodeAgent_->reorderBuffer_[i].tail;
//		printf("ReorderBuffer[%d].head=%d, tail=%d\n",i,head,tail);
//fflush(stdout);
//	}
//
//}
