#include "DecodeAgent.h"
#include "raid6_all.h"

FILTER_NBL_ACQUIRE_PACKET acquire_packet;

void DecodeAgent::init()
{
	printf("Intialize decode agent!\n");
	expectHandOverGenID=0;
	expectHandOverGenIndex=0;
	headReorderBuffer=0;
	packetsCountInReorderBuffer = 0;

	reorderBuffer_ = new ReorderBuffer[ReorderGenNumber];
	memset(reorderBuffer_, 0, ReorderGenNumber*sizeof(ReorderBuffer));
	for (int i = 0; i < ReorderGenNumber; i++)
	{
		reorderBuffer_[i].head = 0;
		reorderBuffer_[i].count=0;
		reorderBuffer_[i].capacity=BlockSize_N;
		reorderBuffer_[i].genID=i;
		reorderBuffer_[i].blockSize=BlockSize_N-BlockSize_MaxP;
		reorderBuffer_[i].headIndex=0;
		reorderBuffer_[i].lastGenOriginalIndex=MAX_PACKETS_COUNT;

		reorderBuffer_[i].ifOccupied = new bool[BlockSize_N];
		memset(reorderBuffer_[i].ifOccupied, 0, (BlockSize_N)*sizeof(bool));
		reorderBuffer_[i].buffer = new FILTER_NBL_PACKET[BlockSize_N];

		for (int j = 0; j < BlockSize_N; j++) {
		FILTER_NBL_ACQUIRE_PACKET acquire_packet = { 0 };
		acquire_packet.m_size = MAX_PACKET_LENGTH;
		if (!parentFilter->AcquirePacket(&acquire_packet)) {
			printf("AcquirePacket failed\n");
			exit(0);
		}
		reorderBuffer_[i].buffer[j] = acquire_packet.m_packet[0];
		reorderBuffer_[i].buffer[j].SetPrefixLength(0);
		}
	}

	////At most hand BlockSize_N-BlockSize_MaxP packets to OS at once.
	maxRecvCount=BlockSize_N-BlockSize_MaxP;
	recv_size = sizeof(FILTER_NBL_FETCH_PACKET)+maxRecvCount*sizeof(FILTER_NBL_PACKET);
	recv_packet = (PFILTER_NBL_RECV_PACKET)new char[recv_size];
	if (!recv_packet) {
		printf("recv_packet new failed\n");
		exit(0);
	}
	recv_packet->m_count=0;

	decodeTimer_.SetTimedEvent(this, &DecodeAgent::decodeTimeout);
}

void DecodeAgent::uninitialize()
{
	//printf("Uninitialize encode agent!\n");
	fflush(stdout);

	if (decodeTimer_.IsStarted())
	{
		decodeTimer_.Stop();
	}

	for (int i = 0; i < ReorderGenNumber; i++)
	{
		delete[] reorderBuffer_[i].buffer;
		delete[] reorderBuffer_[i].ifOccupied;
	}
	delete[] reorderBuffer_;

	if (recv_packet)
	{
		delete recv_packet;
		recv_packet = NULL;
	}
}

bool DecodeAgent::Decode(PFILTER_NBL_FETCH_PACKET fetchPkt, unsigned long size)
{
	///For debug
	//parentCLBTunnelAgent->printIndex(fetchPkt->m_packet[0],"r");

	/*printf("Decode Packet!!!!\n");*/
	parentCLBTunnelAgent->PrintPacket(fetchPkt->m_packet[0], "Receive");

	parentCLBTunnelAgent->vp_mutex.lock();

	UDP_HEADER* udp_tunnel_header;
	udp_tunnel_header = (UDP_HEADER*)(fetchPkt->m_packet[0].GetBuffer() + fetchPkt->m_packet[0].GetPrefixLength() + 34);
	parentCLBTunnelAgent->FeedbackVPAckedPackets[ntohs(udp_tunnel_header->src_port)]
		= (parentCLBTunnelAgent->FeedbackVPAckedPackets[ntohs(udp_tunnel_header->src_port)] + 1) % MAX_PACKETS_COUNT;

	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(fetchPkt->m_packet[0].GetBuffer() + fetchPkt->m_packet[0].GetPrefixLength() + 42);
	////Cleared VP!
	if(parentCLBTunnelAgent->VPRateEstimation[ntohs(clb_header->feedback_vp)]==0 
		&& parentCLBTunnelAgent->VPCost[ntohs(clb_header->feedback_vp)]==0)
	{
		////Do nothing about VP info
	}
	else
	{
		int formerVPPacketsAcked=parentCLBTunnelAgent->VPPacketsAcked[ntohs(clb_header->feedback_vp)];
		if ((ntohs(clb_header->feedback_vp_acked_packets) - formerVPPacketsAcked + MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT < MAX_PACKETS_COUNT / 2)
		{
			parentCLBTunnelAgent->VPPacketsAcked[ntohs(clb_header->feedback_vp)] = ntohs(clb_header->feedback_vp_acked_packets);
		}
		parentCLBTunnelAgent->VPPacketsOnFly[ntohs(clb_header->feedback_vp)]
											= (parentCLBTunnelAgent->VPPacketsSent[ntohs(clb_header->feedback_vp)]
											- parentCLBTunnelAgent->VPPacketsAcked[ntohs(clb_header->feedback_vp)]
											+ MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT;

		clock_t nowTime = clock();
		double timeElapsed = ((double)nowTime - (double)parentCLBTunnelAgent->VPLastUpdateTime[ntohs(clb_header->feedback_vp)] / CLOCKS_PER_SEC);
		double currentRate=0;
		if(timeElapsed == 0)
		{
			if(parentCLBTunnelAgent->VPPacketsAcked[ntohs(clb_header->feedback_vp)]==0)////Initial state, may be time duration=0
			{
				currentRate=0;
			}
			else
			{
				printf("ERROR: nowTime=%ld, VPLastUpdateTime=%ld, nowTime-VPLastUpdateTime=0!\n"
					,nowTime,parentCLBTunnelAgent->VPLastUpdateTime[ntohs(clb_header->feedback_vp)]);
				// exit(0);
				currentRate=0;
			}
		}
		else
		{
			currentRate=((double)parentCLBTunnelAgent->VPPacketsAcked[ntohs(clb_header->feedback_vp)] - formerVPPacketsAcked) * 1500 / timeElapsed;
		}
		
		if(currentRate>0)
		{
			parentCLBTunnelAgent->VPRateEstimation[ntohs(clb_header->feedback_vp)] 
			= ALPHA*parentCLBTunnelAgent->VPRateEstimation[ntohs(clb_header->feedback_vp)] + (1 - ALPHA)*currentRate;
			parentCLBTunnelAgent->VPLastUpdateTime[ntohs(clb_header->feedback_vp)] = nowTime;
		}

		if(parentCLBTunnelAgent->VPRateEstimation[ntohs(clb_header->feedback_vp)] > 0)
		{
			if(parentCLBTunnelAgent->VPCost[ntohs(clb_header->feedback_vp)]==-1)
			{
				parentCLBTunnelAgent->countExploredOnceVP--;
			}
			parentCLBTunnelAgent->VPCost[ntohs(clb_header->feedback_vp)] 
			= parentCLBTunnelAgent->VPPacketsOnFly[ntohs(clb_header->feedback_vp)] / parentCLBTunnelAgent->VPRateEstimation[ntohs(clb_header->feedback_vp)];
		}
	}

	parentCLBTunnelAgent->PrintVPWindow(0);

	parentCLBTunnelAgent->vp_mutex.unlock();

	parentCLBTunnelAgent->PrintReorderBuffer(0);

#if 0
	/*printf("insertReorderBuffer\n");*/
	insertReorderBuffer(fetchPkt,size);////Pkt may be set to Drop in this func
#endif
	parentCLBTunnelAgent->PrintReorderBuffer(0);

	////If this packet is insequence!
	int inSequence = 0;
	if(expectHandOverGenID==clb_header->gen_id
		&& expectHandOverGenIndex==ntohs(clb_header->gen_index)
		&& clb_header->ifParity==0)
	{
		if (fetchPkt->m_packet[0].GetAction() == Drop)
		{
			printf("ERROR: Hand Drop Packet!\n");
			parentCLBTunnelAgent->PrintPacket(fetchPkt->m_packet[0], "", 1);
			exit(0);
		}
		inSequence = 1;

		parentCLBTunnelAgent->PrintPacket(fetchPkt->m_packet[0],"Hand to OS",0);
		DecapPacket(fetchPkt->m_packet[0]);
		parentFilter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)fetchPkt, size);

		expectHandOverGenIndex = (expectHandOverGenIndex+1)%MAX_PACKETS_COUNT;
#if 0
		if (reorderBuffer_[headReorderBuffer].lastGenOriginalIndex != MAX_PACKETS_COUNT &&
			(reorderBuffer_[headReorderBuffer].lastGenOriginalIndex - expectHandOverGenIndex +MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT
			> (MAX_PACKETS_COUNT / 2))
		{
			clearGen(headReorderBuffer);
			expectHandOverGenIndex=0;
			expectHandOverGenID = (expectHandOverGenID+1)%MAX_GENERATION;
			reorderBuffer_[headReorderBuffer].genID = (reorderBuffer_[headReorderBuffer].genID + ReorderGenNumber) % MAX_GENERATION;
			headReorderBuffer=(headReorderBuffer+1)%ReorderGenNumber;
		}
#endif
		///For debug
		reorderBuffer_[headReorderBuffer].headIndex
			= (reorderBuffer_[headReorderBuffer].headIndex + 1) % MAX_PACKETS_COUNT;
		reorderBuffer_[headReorderBuffer].head
			= (reorderBuffer_[headReorderBuffer].head + 1) % reorderBuffer_[headReorderBuffer].capacity;


		parentCLBTunnelAgent->PrintReorderBuffer(0);
	}

	///Drop original out-sequence packet
	if (inSequence == 0)
	{
		////For debug
		if (insertReorderBuffer(fetchPkt, size))////Pkt may be set to Drop in this func
		{
			fetchPkt->m_packet[0].SetAction(Drop);
			parentFilter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)fetchPkt, size);
		}
	}

	/*printf("decodeReorderBuffer\n");*/
#if 0
	decodeReorderBuffer();
#endif
	parentCLBTunnelAgent->PrintReorderBuffer(0);

	/*printf("checkReorderBuffer!\n");*/
	checkReorderBuffer();

	parentCLBTunnelAgent->PrintReorderBuffer(0);

	if(recv_packet->m_count>0)
	{
		/*printf("recv_packet hand over! m_count=%ld\n",
			recv_packet->m_count);*/
		for(int j=0;j<recv_packet->m_count;j++)
		{
			parentCLBTunnelAgent->PrintPacket(recv_packet->m_packet[j],"Hand to OS");
			DecapPacket(recv_packet->m_packet[j]);
		}

		LPOVERLAPPED overlapped = new OVERLAPPED;
		memset(overlapped, 0, sizeof(OVERLAPPED));
		overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		DWORD err;
		err = parentFilter->RecvPacket(recv_packet, recv_size, overlapped);
		if (err != ERROR_SUCCESS &&
			err != ERROR_IO_PENDING) {
			printf("RecvPacket error: %d\n", err);
			exit(0);
		}
		WaitForSingleObject(overlapped->hEvent, INFINITE);
		recv_packet->m_count=0;
	}
	return true;
}

bool DecodeAgent::insertReorderBuffer(PFILTER_NBL_FETCH_PACKET pkt, unsigned long size)
{
	PrintIPPacket(pkt->m_packet[0],"insertReorderBuffer",0);
	/*printf("+++++++++++++++inserting packet Data, length=%lu++++++++++\n"
		, pkt->m_packet[0].GetLength());
	parentCLBTunnelAgent->print_bytes(pkt->m_packet[0].GetBuffer() + pkt->m_packet[0].GetPrefixLength(), pkt->m_packet[0].GetLength());
	*/
	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(pkt->m_packet[0].GetBuffer() + pkt->m_packet[0].GetPrefixLength() + 42);
	if ((clb_header->gen_id - reorderBuffer_[headReorderBuffer].genID + MAX_GENERATION) % MAX_GENERATION
		> MAX_GENERATION/2)
	{
		if(clb_header->ifParity==0)
		{
			parentCLBTunnelAgent->PrintPacket(pkt->m_packet[0],"Hand to OS out-dated",1);
			DecapPacket(pkt->m_packet[0]);
			parentFilter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)pkt, size);
			return false;
		}
		else
		{
			parentCLBTunnelAgent->PrintPacket(pkt->m_packet[0], "Drop out-dated parity",1);
			pkt->m_packet[0].SetAction(Drop);
			return true;
		}
	}
	

	int whichBufferShouldIn 
		= ((clb_header->gen_id - reorderBuffer_[headReorderBuffer].genID + MAX_GENERATION) % MAX_GENERATION 
		+ headReorderBuffer) % ReorderGenNumber;
	if (whichBufferShouldIn < 0 || whichBufferShouldIn >= ReorderGenNumber)
	{
		parentCLBTunnelAgent->PrintPacket(pkt->m_packet[0],"Receive",1);
		printf("whichBufferShouldIn error!\n");
		exit(0);
	}

	if(clb_header->gen_id!=reorderBuffer_[whichBufferShouldIn].genID)
	{
		parentCLBTunnelAgent->PrintPacket(pkt->m_packet[0],"Receive",1);
		printf("Reorder buffer Gen overflow! Not handle yet!\n");
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}
	
	
	int posOffSet = (ntohs(clb_header->gen_index) - reorderBuffer_[whichBufferShouldIn].headIndex + MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT;
	int expectOffSet = (expectHandOverGenIndex - reorderBuffer_[whichBufferShouldIn].headIndex + MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT;
	if (clb_header->ifParity == 0 && posOffSet - expectOffSet >= BlockSize_MaxK)
	{
		parentCLBTunnelAgent->PrintPacket(pkt->m_packet[0],"Receive",1);
		printf("Reorderbuffer[%d] full and Index overflow. Can't deal with it now!\n"
			,whichBufferShouldIn);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}
	else if (clb_header->ifParity == 1 && posOffSet - expectOffSet >= BlockSize_N)
	{
		parentCLBTunnelAgent->PrintPacket(pkt->m_packet[0], "Receive", 1);
		printf("Reorderbuffer[%d] full and Index overflow. Can't deal with it now!\n"
			, whichBufferShouldIn);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}

#if 0
	if(clb_header->ifParity==0 && posOffSet>=BlockSize_MaxK)
	{
		/* printf("Reorderbuffer[%d] full, shift!\n"
		 	,whichBufferShouldIn);*/
		for(int i=0;i<(posOffSet-BlockSize_MaxK + 1);i++)
		{
			int pos=reorderBuffer_[whichBufferShouldIn].head;
			if(reorderBuffer_[whichBufferShouldIn].ifOccupied[pos]!=false)
			{
				reorderBuffer_[whichBufferShouldIn].ifOccupied[pos]=false;
				reorderBuffer_[whichBufferShouldIn].count--;
				packetsCountInReorderBuffer--;
			}
			reorderBuffer_[whichBufferShouldIn].head=(reorderBuffer_[whichBufferShouldIn].head+1)%reorderBuffer_[whichBufferShouldIn].capacity;
			reorderBuffer_[whichBufferShouldIn].headIndex++;
		}
	}
	else if(clb_header->ifParity==1 && posOffSet>(reorderBuffer_[whichBufferShouldIn].blockSize+clb_header->parity_index))
	{
		 /*printf("Reorderbuffer[%d] full due to parity, shift!\n"
		 	,whichBufferShouldIn);*/
		 for(int i=0;i<(posOffSet-(reorderBuffer_[whichBufferShouldIn].blockSize+clb_header->parity_index));i++)
		{
			int pos=reorderBuffer_[whichBufferShouldIn].head;
			if(reorderBuffer_[whichBufferShouldIn].ifOccupied[pos]!=false)
			{
				reorderBuffer_[whichBufferShouldIn].ifOccupied[pos]=false;
				reorderBuffer_[whichBufferShouldIn].count--;
				packetsCountInReorderBuffer--;
			}
			reorderBuffer_[whichBufferShouldIn].head=(reorderBuffer_[whichBufferShouldIn].head+1)%reorderBuffer_[whichBufferShouldIn].capacity;
			reorderBuffer_[whichBufferShouldIn].headIndex++;
		}
	}
#endif

	int insertPos=
		(reorderBuffer_[whichBufferShouldIn].head + 
		(ntohs(clb_header->gen_index) - reorderBuffer_[whichBufferShouldIn].headIndex + MAX_PACKETS_COUNT) 
		% MAX_PACKETS_COUNT)
		%reorderBuffer_[whichBufferShouldIn].capacity;
	if(reorderBuffer_[whichBufferShouldIn].ifOccupied[insertPos]!=false)
	{
		printf("Reorder buffer error! insertPos=%d\n", insertPos);
		parentCLBTunnelAgent->PrintPacket(pkt->m_packet[0], "Receive", 1);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}

	reorderBuffer_[whichBufferShouldIn].ifOccupied[insertPos]=true;
	memcpy(reorderBuffer_[whichBufferShouldIn].buffer[insertPos].GetBuffer()+reorderBuffer_[whichBufferShouldIn].buffer[insertPos].GetPrefixLength(),
		pkt->m_packet[0].GetBuffer()+pkt->m_packet[0].GetPrefixLength(),
		pkt->m_packet[0].GetLength());
	reorderBuffer_[whichBufferShouldIn].buffer[insertPos].SetLength(pkt->m_packet[0].GetLength());

	/*printf("+++++++++++++++inserted reorder buffer Data, length=%lu++++++++++\n"
		, reorderBuffer_[whichBufferShouldIn].buffer[insertPos].GetLength());
	parentCLBTunnelAgent->print_bytes(reorderBuffer_[whichBufferShouldIn].buffer[insertPos].GetBuffer() 
		+ reorderBuffer_[whichBufferShouldIn].buffer[insertPos].GetPrefixLength(),
		reorderBuffer_[whichBufferShouldIn].buffer[insertPos].GetLength());*/

	reorderBuffer_[whichBufferShouldIn].count++;
	packetsCountInReorderBuffer++;

#if 0
	if(clb_header->ifParity==1)
	{
		CLB_PARITY_OPTION* clb_option;
		clb_option = (CLB_PARITY_OPTION*)(pkt->m_packet[0].GetBuffer() + pkt->m_packet[0].GetPrefixLength() + 50);
		int blocksize
			=(ntohs(clb_header->gen_index)
			- clb_header->parity_index
			- ntohs(clb_option->head_gen_index) + MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT;
		reorderBuffer_[whichBufferShouldIn].blockSize=blocksize;
		reorderBuffer_[whichBufferShouldIn].lastGenOriginalIndex=
			(ntohs(clb_header->gen_index) - clb_header->parity_index - 1 + MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT;
	}

///If the third gen come, and the first gen has not been all handed over, then clear first gen.
	if ((clb_header->gen_id - reorderBuffer_[headReorderBuffer].genID + MAX_GENERATION) % MAX_GENERATION
		>= 2)
	{
		/*printf("The third gen come, handOverAndClearGen(%d)!\n",
			headReorderBuffer);*/
		handOverAndClearGen(headReorderBuffer);
		reorderBuffer_[headReorderBuffer].genID = (reorderBuffer_[headReorderBuffer].genID + ReorderGenNumber) % MAX_GENERATION;
		headReorderBuffer=(headReorderBuffer+1)%ReorderGenNumber;
		expectHandOverGenIndex=0;
		expectHandOverGenID++;
	}
#endif

	return true;
}

void DecodeAgent::handOverAndClearGen(int bufferID)
{
	printf("++++++++FLOW %d: handOverAndClearGen!!+++++++++\n", parentCLBTunnelAgent->flowID);

	if(bufferID!=headReorderBuffer)
	{
		printf("ERROR: Should only handOverAndClearGen(headReorderBuffer)!\n");
		exit(0);
	}
	else
	{
		if(expectHandOverGenID!=reorderBuffer_[headReorderBuffer].genID)
		{
			printf("ERROR: expectHandOverGenID(%d)!=reorderBuffer_[headReorderBuffer].genID(%d)\n"
				,expectHandOverGenID
				,reorderBuffer_[headReorderBuffer].genID
				);
			parentCLBTunnelAgent->PrintReorderBuffer(1);
			exit(0);
		}

		for(int i=0;i<reorderBuffer_[bufferID].capacity;i++)
		{
			if (reorderBuffer_[bufferID].count == 0)
			{
				expectHandOverGenIndex = (expectHandOverGenIndex + 1) % MAX_PACKETS_COUNT;
				break;
			}

			expectHandOverGenIndex = (expectHandOverGenIndex + 1) % MAX_PACKETS_COUNT;
			int expectPos = ((expectHandOverGenIndex - reorderBuffer_[bufferID].headIndex + reorderBuffer_[bufferID].head + MAX_PACKETS_COUNT) 
				% MAX_PACKETS_COUNT)
				%reorderBuffer_[bufferID].capacity;
			if(reorderBuffer_[bufferID].ifOccupied[expectPos]!=false)
			{
				
				CLB_HEADER* clb_header;
				clb_header = (CLB_HEADER*)(reorderBuffer_[bufferID].buffer[expectPos].GetBuffer() + reorderBuffer_[bufferID].buffer[expectPos].GetPrefixLength() + 42);
				
				if(clb_header->ifParity==0 && ntohs(clb_header->gen_index)==expectHandOverGenIndex)
				{
					parentCLBTunnelAgent->PrintPacket(reorderBuffer_[bufferID].buffer[expectPos],"Copy to recv_packet");
					///Hand over copy.
					///Do it all together at the end of decode();
					recv_packet->m_packet[recv_packet->m_count]=reorderBuffer_[bufferID].buffer[expectPos];
					recv_packet->m_count++;

					/////If recv_packet is full, hand onver
					if(recv_packet->m_count>=maxRecvCount)
					{
						/*printf("recv_packet overflow! m_count=%ld, maxRecvCount=%d\n",
							recv_packet->m_count,
							maxRecvCount);*/
						for(int j=0;j<recv_packet->m_count;j++)
						{
							parentCLBTunnelAgent->PrintPacket(recv_packet->m_packet[j],"Hand to OS");
							DecapPacket(recv_packet->m_packet[j]);
						}

						LPOVERLAPPED overlapped = new OVERLAPPED;
						memset(overlapped, 0, sizeof(OVERLAPPED));
						overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
						DWORD err;
						err = parentFilter->RecvPacket(recv_packet, recv_size, overlapped);
						if (err != ERROR_SUCCESS &&
							err != ERROR_IO_PENDING) {
							printf("RecvPacket error: %d\n", err);
							exit(0);
						}
						WaitForSingleObject(overlapped->hEvent, INFINITE);
						recv_packet->m_count=0;
					}
				}
				else
				{
					////Do nothing
				}
				reorderBuffer_[bufferID].ifOccupied[expectPos]=false;
				reorderBuffer_[bufferID].count--;
				packetsCountInReorderBuffer--;
			}
		}
	}
	
	if(reorderBuffer_[bufferID].count!=0)
	{
		printf("ERROR: Clear reorderBuffer_[%d].count=%d\n"
			,bufferID
			,reorderBuffer_[bufferID].count);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}
	reorderBuffer_[bufferID].head=0;
	reorderBuffer_[bufferID].count=0;
	reorderBuffer_[bufferID].blockSize=BlockSize_N-BlockSize_MaxP;
	reorderBuffer_[bufferID].headIndex=0;
	reorderBuffer_[bufferID].lastGenOriginalIndex=MAX_PACKETS_COUNT;
}

void DecodeAgent::clearGen(int bufferID)
{
	if(bufferID==headReorderBuffer)
	{
		if(expectHandOverGenID!=reorderBuffer_[headReorderBuffer].genID)
		{
			printf("ERROR: expectHandOverGenID(%d)!=reorderBuffer_[headReorderBuffer].genID(%d)\n"
				,expectHandOverGenID
				,reorderBuffer_[headReorderBuffer].genID
				);
			parentCLBTunnelAgent->PrintReorderBuffer(1);
			exit(0);
		}

		if (reorderBuffer_[bufferID].lastGenOriginalIndex != MAX_PACKETS_COUNT &&
			(expectHandOverGenIndex - reorderBuffer_[bufferID].lastGenOriginalIndex + MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT
			> MAX_PACKETS_COUNT/2)
		{
			printf("ERROR: headReorderBuffer should only be cleared while it has been handed over all!\n");
			parentCLBTunnelAgent->PrintReorderBuffer(1);
			exit(0);
		}
	}
	
	for(int i=0;i<reorderBuffer_[bufferID].capacity;i++)
	{
		if(reorderBuffer_[bufferID].ifOccupied[i]!=false)
		{
			packetsCountInReorderBuffer--;
			reorderBuffer_[bufferID].count--;
			reorderBuffer_[bufferID].ifOccupied[i]=false;
		}
	}
	if(reorderBuffer_[bufferID].count!=0)
	{
		printf("ERROR: Clear reorderBuffer_[%d].count=%d\n"
			,bufferID
			,reorderBuffer_[bufferID].count);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}
	reorderBuffer_[bufferID].head=0;
	reorderBuffer_[bufferID].count=0;
	reorderBuffer_[bufferID].blockSize=BlockSize_N-BlockSize_MaxP;
	reorderBuffer_[bufferID].headIndex=0;
	reorderBuffer_[bufferID].lastGenOriginalIndex=MAX_PACKETS_COUNT;
}

void DecodeAgent::DecapPacket(FILTER_NBL_PACKET pkt)
{
	UDP_HEADER* udp_tunnel_header;
	udp_tunnel_header = (UDP_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength() + 34);
	if (ntohs(udp_tunnel_header->dst_port) != 5217)
	{
		printf("DecapPacket error!\n");
		decodeTimer_.Stop();
		exit(0);
	}

	int extraHeaderLength=0;
	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength() + 42);
	if(clb_header->ifParity==0)
	{
		extraHeaderLength=36;
	}
	else
	{
		extraHeaderLength=38;
	}

	pkt.SetLength(pkt.GetLength() - extraHeaderLength);
	memcpy(pkt.GetBuffer() + pkt.GetPrefixLength() + 14
		, pkt.GetBuffer() + pkt.GetPrefixLength() + 14 +extraHeaderLength
		, pkt.GetLength() - 14);
}

void DecodeAgent::checkReorderBuffer()
{
	// printf("%lf-NODE %d: checking reorder buffer!\n",Scheduler::instance().clock(),ParentClassifier_->nodeID());
	if(packetsCountInReorderBuffer<0)
	{
		printf("ERROR: packetsCountInReorderBuffer<0!\n");
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}

	if(packetsCountInReorderBuffer==0)
	{
		if(decodeTimer_.IsStarted())
		{
			decodeTimer_.Stop();
		}
		return;
	}

	//printf("----------------FLOW %d: There are Out-of-Order Packets!------------\n", parentCLBTunnelAgent->flowID);


	if(expectHandOverGenID!=reorderBuffer_[headReorderBuffer].genID)
	{
		printf("ERROR: expectHandOverGenID(%d)!=reorderBuffer_[headReorderBuffer].genID(%d)\n"
			,expectHandOverGenID
			,reorderBuffer_[headReorderBuffer].genID
			);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}

	int expectPos = ((expectHandOverGenIndex - reorderBuffer_[headReorderBuffer].headIndex + reorderBuffer_[headReorderBuffer].head + MAX_PACKETS_COUNT)
		% MAX_PACKETS_COUNT)
		%reorderBuffer_[headReorderBuffer].capacity;
	if(expectPos<0 || expectPos>=reorderBuffer_[headReorderBuffer].capacity)
	{
		printf("ERROR: expectPos=%d\n",expectPos);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}

	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(reorderBuffer_[headReorderBuffer].buffer[expectPos].GetBuffer() + reorderBuffer_[headReorderBuffer].buffer[expectPos].GetPrefixLength() + 42);


	if(reorderBuffer_[headReorderBuffer].ifOccupied[expectPos]==false)
	{
		if (packetsCountInReorderBuffer != 0) ////For debug
		{
			decodeTimer_.Stop();
			if (!decodeTimer_.Start(DECODE_TIMER_LENGTH))
			{
				printf("decodeTimer_ start failed!\n");
				exit(0);
			}
		}
		
		return;
	}


	if(clb_header->ifParity==0)
	{
		parentCLBTunnelAgent->PrintPacket(reorderBuffer_[headReorderBuffer].buffer[expectPos],"Copy to recv_packet");
		///Hand over copy.
		///Do it all together at the end of decode();
		recv_packet->m_packet[recv_packet->m_count]=reorderBuffer_[headReorderBuffer].buffer[expectPos];
		recv_packet->m_count++;

		/////If recv_packet is full, hand onver
		if(recv_packet->m_count>=maxRecvCount)
		{
			/*printf("recv_packet overflow! m_count=%ld, maxRecvCount=%d\n",
				recv_packet->m_count,
				maxRecvCount);*/
			for(int j=0;j<recv_packet->m_count;j++)
			{
				parentCLBTunnelAgent->PrintPacket(recv_packet->m_packet[j],"Hand to OS");
				DecapPacket(recv_packet->m_packet[j]);
			}

			LPOVERLAPPED overlapped = new OVERLAPPED;
			memset(overlapped, 0, sizeof(OVERLAPPED));
			overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			DWORD err;
			err = parentFilter->RecvPacket(recv_packet, recv_size, overlapped);
			if (err != ERROR_SUCCESS &&
				err != ERROR_IO_PENDING) {
				printf("RecvPacket error: %d\n", err);
				exit(0);
			}
			WaitForSingleObject(overlapped->hEvent, INFINITE);
			recv_packet->m_count=0;
		}
	}
	else
	{
		////Do nothing
	}

	
#if 0
	expectHandOverGenIndex = (expectHandOverGenIndex + 1) % MAX_PACKETS_COUNT;
	if (reorderBuffer_[headReorderBuffer].lastGenOriginalIndex!=MAX_PACKETS_COUNT &&
		(reorderBuffer_[headReorderBuffer].lastGenOriginalIndex - expectHandOverGenIndex +MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT
		> MAX_PACKETS_COUNT/2)
	{
		clearGen(headReorderBuffer);
		expectHandOverGenIndex=0;
		expectHandOverGenID = (expectHandOverGenID + 1) % MAX_GENERATION;
		reorderBuffer_[headReorderBuffer].genID = (reorderBuffer_[headReorderBuffer].genID + ReorderGenNumber) % MAX_GENERATION;
		headReorderBuffer=(headReorderBuffer+1)%ReorderGenNumber;
		checkReorderBuffer();
	}
	else
	{
		checkReorderBuffer();
	}
#endif

	///For debug only
	expectHandOverGenIndex = (expectHandOverGenIndex + 1) % MAX_PACKETS_COUNT;
	reorderBuffer_[headReorderBuffer].ifOccupied[expectPos] = false;
	packetsCountInReorderBuffer--;
	reorderBuffer_[headReorderBuffer].count--;
	reorderBuffer_[headReorderBuffer].head = (reorderBuffer_[headReorderBuffer].head + 1) % reorderBuffer_[headReorderBuffer].capacity;
	reorderBuffer_[headReorderBuffer].headIndex = (reorderBuffer_[headReorderBuffer].headIndex + 1) % MAX_PACKETS_COUNT;
	///For debug only
	
	checkReorderBuffer();
	// printf("%lf-NODE %d: checking reorder buffer finished!\n",Scheduler::instance().clock(),ParentClassifier_->nodeID());
}

void DecodeAgent::decodeReorderBuffer()
{
	// printf("%lf-NODE %d: decoding decodeReorderBuffer!\n",Scheduler::instance().clock(),ParentClassifier_->nodeID());
	if(reorderBuffer_[headReorderBuffer].count<0)
	{
		printf("ERROR: reorderBuffer_[%d].count=%d\n"
			,headReorderBuffer
			,reorderBuffer_[headReorderBuffer].count);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}

	if (reorderBuffer_[headReorderBuffer].lastGenOriginalIndex != MAX_PACKETS_COUNT &&
		(reorderBuffer_[headReorderBuffer].lastGenOriginalIndex - expectHandOverGenIndex + MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT
		> MAX_PACKETS_COUNT/2)
	{
		return;
	}

	if(reorderBuffer_[headReorderBuffer].count>=reorderBuffer_[headReorderBuffer].blockSize)
	{
		FILTER_NBL_PACKET firstParity;
		int flag=0;
		for(int i=0;i<BlockSize_MaxP;i++)
		{
			int pos=(i+reorderBuffer_[headReorderBuffer].head+reorderBuffer_[headReorderBuffer].blockSize)%reorderBuffer_[headReorderBuffer].capacity;
			if(reorderBuffer_[headReorderBuffer].ifOccupied[pos]!=false)
			{
				/*printf("firstParity pos=%d\n", pos);*/
				firstParity = reorderBuffer_[headReorderBuffer].buffer[pos];
				flag=1;
				break;
			}
		}
		if(flag==0)
		{
			return;
		}

		recoverMissingPacketQ(headReorderBuffer, firstParity);

		//// printf("%lf-NODE %d: Able to decode, decode it!\n",Scheduler::instance().clock(),ParentClassifier_->nodeID());
		//for(int i=0;i<reorderBuffer_[headReorderBuffer].blockSize;i++)
		//{
		//	int pos=(i+reorderBuffer_[headReorderBuffer].head)%reorderBuffer_[headReorderBuffer].capacity;
		//	if(reorderBuffer_[headReorderBuffer].ifOccupied[pos]==false)
		//	{
		//		parentCLBTunnelAgent->clb_decode_times++;
		//		printf("FLOW %d: clb_decode_times=%d\n", 
		//			parentCLBTunnelAgent->flowID,
		//			parentCLBTunnelAgent->clb_decode_times);
		//		/*printf("+++++++++++++++firstParity Data, length=%lu++++++++++\n"
		//			, firstParity.GetLength());
		//		parentCLBTunnelAgent->print_bytes(firstParity.GetBuffer()+ firstParity.GetPrefixLength(),firstParity.GetLength());*/
		//		///original XOR decode
		//		recoverMissingPacket(headReorderBuffer, pos, firstParity);
		//		/*printf("+++++++++++++++recovered Packet Data, length=%lu++++++++++\n"
		//			, reorderBuffer_[headReorderBuffer].buffer[pos].GetLength());
		//		parentCLBTunnelAgent->print_bytes(reorderBuffer_[headReorderBuffer].buffer[pos].GetBuffer() 
		//			+ reorderBuffer_[headReorderBuffer].buffer[pos].GetPrefixLength(),
		//			reorderBuffer_[headReorderBuffer].buffer[pos].GetLength());*/
		//		reorderBuffer_[headReorderBuffer].count++;
		//		packetsCountInReorderBuffer++;
		//	}
		//}
	}
}

void DecodeAgent::recoverMissingPacketQ(int bufferID, FILTER_NBL_PACKET firstParity)
{
	ETHERNET_HEADER* old_eth_header;
	old_eth_header = (ETHERNET_HEADER*)(firstParity.GetBuffer() + firstParity.GetPrefixLength());
	int maxLenInCurrentGen = firstParity.GetLength() - 38; /////Rember the length of max possible restored packet

	/////minus the length of ethernet header, and align with sizeof(long)
	int maxDataSize = maxLenInCurrentGen - 14;
	int offset = maxDataSize % sizeof(long);
	if (offset != 0)
	{
		maxDataSize = sizeof(long)-offset + maxLenInCurrentGen - 14;
	}

	/*printf("---------------recoverMissingPacket, maxLenInCurrentGen=%d!-------------\n", maxLenInCurrentGen);*/
	if (maxLenInCurrentGen <= 0 || maxLenInCurrentGen>MAX_PACKET_LENGTH)
	{
		printf("ERROR: maxLenInCurrentGen=%d!\n", maxLenInCurrentGen);
		decodeTimer_.Stop();
		exit(0);
	}

	int * erasures = new int[BlockSize_MaxP + 1];
	for (int i = 0; i < BlockSize_MaxP + 1; i++)
	{
		erasures[i] = -1;
	}
	int erasureCount = 0;

	int blocksize = reorderBuffer_[headReorderBuffer].blockSize;

	if (maxDataSize%sizeof(long) != 0)
	{
		printf("UnAligned Data! maxDataSize=%d, sizeof(long)=%d\n",
			maxDataSize, sizeof(long));
		exit(0);
	}
	unsigned char** data;
	data = new unsigned char*[blocksize];
	for (int i = 0; i < blocksize; i++)
	{
		data[i] = new unsigned char[maxDataSize];
		memset(data[i], 0, maxDataSize*sizeof(unsigned char));
	}

	unsigned char** encodedData;
	encodedData = new unsigned char*[BlockSize_MaxP];
	for (int i = 0; i < BlockSize_MaxP; i++)
	{
		encodedData[i] = new unsigned char[maxDataSize];
		memset(encodedData[i], 0, maxDataSize*sizeof(unsigned char));
	}

	/*printf("aligned maxDataSize=%d, blocksize=%d\n", maxDataSize, blocksize);*/

	for (int i = 0; i<blocksize + BlockSize_MaxP; i++)
	{
		int pos = (i + reorderBuffer_[bufferID].head) % reorderBuffer_[bufferID].capacity;
		if (reorderBuffer_[bufferID].ifOccupied[pos] == false)
		{
			erasures[erasureCount] = i;
			erasureCount++;
			reorderBuffer_[bufferID].ifOccupied[pos] = true;
			reorderBuffer_[bufferID].count++;
			packetsCountInReorderBuffer++;

			if (i < blocksize)
			{
				parentCLBTunnelAgent->clb_decode_times++;
				printf("FLOW %d: clb_decode_times=%d\n",
					parentCLBTunnelAgent->flowID,
					parentCLBTunnelAgent->clb_decode_times);
			}
		}
		else
		{
			if (i < blocksize)
			{
				/*printf("+++++++++++++++orignial packet++++++++++\n");
				parentCLBTunnelAgent->print_bytes(
					reorderBuffer_[bufferID].buffer[pos].GetBuffer() + reorderBuffer_[bufferID].buffer[pos].GetPrefixLength() + 50,
					reorderBuffer_[bufferID].buffer[pos].GetLength() - 50);*/

				memcpy(data[i]
					, (reorderBuffer_[bufferID].buffer[pos].GetBuffer() + reorderBuffer_[bufferID].buffer[pos].GetPrefixLength() + 50)
					, reorderBuffer_[bufferID].buffer[pos].GetLength() - 50);

				/*printf("+++++++++++++++copied data++++++++++\n");
				parentCLBTunnelAgent->print_bytes(data[i], maxDataSize);
				printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");*/
			}
			else
			{
				/*printf("+++++++++++++++orignial packet++++++++++\n");
				parentCLBTunnelAgent->print_bytes(
					reorderBuffer_[bufferID].buffer[pos].GetBuffer() + reorderBuffer_[bufferID].buffer[pos].GetPrefixLength() + 52,
					reorderBuffer_[bufferID].buffer[pos].GetLength() - 52);*/

				memcpy(encodedData[i - blocksize]
					, (reorderBuffer_[bufferID].buffer[pos].GetBuffer() + reorderBuffer_[bufferID].buffer[pos].GetPrefixLength() + 52)
					, reorderBuffer_[bufferID].buffer[pos].GetLength() - 52);

				/*printf("+++++++++++++++copied data++++++++++\n");
				parentCLBTunnelAgent->print_bytes(encodedData[i - blocksize], maxDataSize);
				printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");*/
			}
		}
	}

	/*for (int i = 0; i < BlockSize_MaxP+1; i++)
	{
		printf("erasures[%d]=%d\n", i, erasures[i]);
	}
	printf("===========Before Decoding==============\n");
	for (int i = 0; i < blocksize; i++)
	{
		printf("+++++++++++++++original data++++++++++\n");
		parentCLBTunnelAgent->print_bytes(data[i], maxDataSize);
	}
	for (int i = 0; i < BlockSize_MaxP; i++)
	{
		printf("+++++++++++++++encoded data++++++++++\n");
		parentCLBTunnelAgent->print_bytes(encodedData[i], maxDataSize);
	}*/

	

	int *matrix;
	matrix = reed_sol_r6_coding_matrix(blocksize, 8);
	jerasure_matrix_decode(
		blocksize,
		BlockSize_MaxP,
		8,
		matrix,
		1,
		erasures,
		(char **)data,
		(char **)encodedData,
		maxDataSize);

	/*printf("===========After Decoding==============\n");
	for (int i = 0; i < blocksize; i++)
	{
		printf("+++++++++++++++original data++++++++++\n");
		parentCLBTunnelAgent->print_bytes(data[i], maxDataSize);
	}
	for (int i = 0; i < BlockSize_MaxP; i++)
	{
		printf("+++++++++++++++encoded data++++++++++\n");
		parentCLBTunnelAgent->print_bytes(encodedData[i], maxDataSize);
	}*/


	for (int i = 0; i < BlockSize_MaxP; i++)
	{
		if (erasures[i] == -1)
		{
			break;
		}
		int position = (erasures[i] + reorderBuffer_[bufferID].head) % reorderBuffer_[bufferID].capacity;
		memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength()
			, old_eth_header
			, sizeof(ETHERNET_HEADER));

		if (erasures[i] < blocksize)
		{
			memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 50
				, data[erasures[i]]
				, (maxLenInCurrentGen - 14)*sizeof(unsigned char));
		}
		else
		{
			memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 52
				, encodedData[erasures[i] - blocksize]
				, (maxLenInCurrentGen - 14)*sizeof(unsigned char));
		}
		

		IP_HEADER* ip_header;
		ip_header = (IP_HEADER*)(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 50);

		memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 14
			, ip_header
			, sizeof(IP_HEADER));
		memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 34
			, firstParity.GetBuffer() + firstParity.GetPrefixLength() + 34
			, sizeof(UDP_HEADER)+sizeof(CLB_HEADER));

		CLB_HEADER* clb_header;
		clb_header = (CLB_HEADER*)(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 42);
		clb_header->ifParity = 0;
		clb_header->parity_index = 0;
		clb_header->gen_index = htons(((position - reorderBuffer_[bufferID].head + reorderBuffer_[bufferID].capacity) % reorderBuffer_[bufferID].capacity + reorderBuffer_[bufferID].headIndex) % MAX_PACKETS_COUNT);

		if (erasures[i] >= blocksize)
		{
			clb_header->parity_index = erasures[i] - blocksize;
			memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 50
				, firstParity.GetBuffer() + firstParity.GetPrefixLength() + 50
				, sizeof(CLB_PARITY_OPTION));
		}

		reorderBuffer_[bufferID].buffer[position].SetLength(50 + ntohs(ip_header->ip_total_length));
	}
}

void DecodeAgent::recoverMissingPacket(int bufferID, int position, FILTER_NBL_PACKET firstParity)
{
	ETHERNET_HEADER* old_eth_header;
	old_eth_header = (ETHERNET_HEADER*)(firstParity.GetBuffer() + firstParity.GetPrefixLength());
	int maxLenInCurrentGen = firstParity.GetLength() - 38; /////Rember the length of max possible restored packet
	printf("---------------recoverMissingPacket, maxLenInCurrentGen=%d!-------------\n", maxLenInCurrentGen);
	if (maxLenInCurrentGen <= 0 || maxLenInCurrentGen>MAX_PACKET_LENGTH)
	{
		printf("ERROR: maxLenInCurrentGen=%d!\n", maxLenInCurrentGen);
		decodeTimer_.Stop();
		exit(0);
	}
	unsigned char* data;
	data = new unsigned char[maxLenInCurrentGen - 14];/////minus the length of ethernet header
	memset(data, 0, (maxLenInCurrentGen - 14)*sizeof(unsigned char));
	unsigned char* restoredData;
	restoredData = new unsigned char[maxLenInCurrentGen - 14];/////minus the length of ethernet header
	memset(restoredData, 0, (maxLenInCurrentGen - 14)*sizeof(unsigned char));
	for (int k = 0; k < reorderBuffer_[bufferID].blockSize+BlockSize_MaxP; k++)
	{
		int i=(k+reorderBuffer_[bufferID].head)%reorderBuffer_[bufferID].capacity;
		if (reorderBuffer_[bufferID].ifOccupied[i] == false) /////hit the missing position
		{
			if (position != i)
			{
				printf("recoverMissingPacket error!\n");
				decodeTimer_.Stop();
				exit(0);
			}
			continue;
		}
		CLB_HEADER* clb_header_thisPacket;
		clb_header_thisPacket = (CLB_HEADER*)(reorderBuffer_[bufferID].buffer[i].GetBuffer() + reorderBuffer_[bufferID].buffer[i].GetPrefixLength() + 42);
		if (clb_header_thisPacket->ifParity == 0)
		{
			memcpy(data
			, (reorderBuffer_[bufferID].buffer[i].GetBuffer() + reorderBuffer_[bufferID].buffer[i].GetPrefixLength() + 50)
			, reorderBuffer_[bufferID].buffer[i].GetLength() - 50);
			/*printf("+++++++++++++++packet[%d] data++++++++++\n",i);
			parentCLBTunnelAgent->print_bytes(data, reorderBuffer_[bufferID].buffer[i].GetLength() - 50);*/
		}
		else
		{
			memcpy(data
			, (reorderBuffer_[bufferID].buffer[i].GetBuffer() + reorderBuffer_[bufferID].buffer[i].GetPrefixLength() + 52)
			, reorderBuffer_[bufferID].buffer[i].GetLength() - 52);
			/*printf("+++++++++++++++packet[%d] data++++++++++\n", i);
			parentCLBTunnelAgent->print_bytes(data, reorderBuffer_[bufferID].buffer[i].GetLength() - 52);*/
		}
		
		for (int j = 0; j < maxLenInCurrentGen - 14; j++)
		{
			restoredData[j] = restoredData[j] ^ data[j];
		}
	}
	/*printf("+++++++++++++++restoredData++++++++++\n"); 
	parentCLBTunnelAgent->print_bytes(restoredData,maxLenInCurrentGen-14);*/

	reorderBuffer_[bufferID].ifOccupied[position] = true;

	memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength()
		, old_eth_header
		, sizeof(ETHERNET_HEADER));
	memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 50
		, restoredData
		, (maxLenInCurrentGen - 14)*sizeof(unsigned char));

	IP_HEADER* ip_header;
	ip_header = (IP_HEADER*)(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 50);
	
	memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 14
		, ip_header
		, sizeof(IP_HEADER));
	memcpy(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 34
		, firstParity.GetBuffer() + firstParity.GetPrefixLength() + 34
		, sizeof(UDP_HEADER)+sizeof(CLB_HEADER));

	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(reorderBuffer_[bufferID].buffer[position].GetBuffer() + reorderBuffer_[bufferID].buffer[position].GetPrefixLength() + 42);
	clb_header->ifParity=0;
	clb_header->parity_index=0;
	clb_header->gen_index=htons((position-reorderBuffer_[bufferID].head+reorderBuffer_[bufferID].capacity)%reorderBuffer_[bufferID].capacity+reorderBuffer_[bufferID].headIndex);

	reorderBuffer_[bufferID].buffer[position].SetLength(50 + ntohs(ip_header->ip_total_length));
}

void DecodeAgent::decodeTimeout()
{
	parentCLBTunnelAgent->decodePacket_mutex.lock();
	decodeTimer_.Stop();
	printf("===========FLOW %d: Decode Timeout!!=============\n", parentCLBTunnelAgent->flowID);
	parentCLBTunnelAgent->PrintReorderBuffer(0);

	if (packetsCountInReorderBuffer<0)
	{
		printf("ERROR: packetsCountInReorderBuffer<0!\n");
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}

	if (packetsCountInReorderBuffer == 0)
	{
		return;
	}

#if 0
	decodeReorderBuffer();
#endif

	int expectPos = ((expectHandOverGenIndex - reorderBuffer_[headReorderBuffer].headIndex + reorderBuffer_[headReorderBuffer].head + MAX_PACKETS_COUNT) 
		% MAX_PACKETS_COUNT)
		% reorderBuffer_[headReorderBuffer].capacity;
	if (expectPos<0 || expectPos >= reorderBuffer_[headReorderBuffer].capacity)
	{
		printf("ERROR: expectPos=%d\n", expectPos);
		parentCLBTunnelAgent->PrintReorderBuffer(1);
		exit(0);
	}

	if (reorderBuffer_[headReorderBuffer].ifOccupied[expectPos] != false)
	{
		CLB_HEADER* clb_header;
		clb_header = (CLB_HEADER*)(reorderBuffer_[headReorderBuffer].buffer[expectPos].GetBuffer() + reorderBuffer_[headReorderBuffer].buffer[expectPos].GetPrefixLength() + 42);
		if (ntohs(clb_header->gen_index) == expectHandOverGenIndex)
		{
			checkReorderBuffer();
		}	
	}
	else
	{
		/*printf("FLOW %d: Clear head gen and hand over all packets!\n", parentCLBTunnelAgent->flowID);*/
		parentCLBTunnelAgent->PrintReorderBuffer(0);

		parentCLBTunnelAgent->clb_decode_timeout_times++;

		printf("FLOW %d: clb_decode_timeout_times=%d\n",
			parentCLBTunnelAgent->flowID,
			parentCLBTunnelAgent->clb_decode_timeout_times);

		handOverAndClearGen(headReorderBuffer);
#if 0
		reorderBuffer_[headReorderBuffer].genID = (reorderBuffer_[headReorderBuffer].genID + ReorderGenNumber) % MAX_GENERATION;
		headReorderBuffer = (headReorderBuffer + 1) % ReorderGenNumber;
		expectHandOverGenIndex = 0;
		expectHandOverGenID = (expectHandOverGenID + 1) % MAX_GENERATION;
#endif
		/*printf("FLOW %d: Clear head reorder gen complete!\n", parentCLBTunnelAgent->flowID);*/
		parentCLBTunnelAgent->PrintReorderBuffer(0);

		decodeReorderBuffer();

		checkReorderBuffer();

		parentCLBTunnelAgent->PrintReorderBuffer(0);
	}

	if (recv_packet->m_count>0)
	{
		/*printf("recv_packet hand over! m_count=%ld\n",
			recv_packet->m_count);*/
		for (int j = 0; j<recv_packet->m_count; j++)
		{
			parentCLBTunnelAgent->PrintPacket(recv_packet->m_packet[j], "Hand to OS");
			DecapPacket(recv_packet->m_packet[j]);
		}

		LPOVERLAPPED overlapped = new OVERLAPPED;
		memset(overlapped, 0, sizeof(OVERLAPPED));
		overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		DWORD err;
		err = parentFilter->RecvPacket(recv_packet, recv_size, overlapped);
		if (err != ERROR_SUCCESS &&
			err != ERROR_IO_PENDING) {
			printf("RecvPacket error: %d\n", err);
			exit(0);
		}
		WaitForSingleObject(overlapped->hEvent, INFINITE);
		recv_packet->m_count = 0;
	}

	parentCLBTunnelAgent->decodePacket_mutex.unlock();
	printf("===========FLOW %d: Decode Timeout Finished!!=============\n", parentCLBTunnelAgent->flowID);
	parentCLBTunnelAgent->PrintReorderBuffer(0);
}
