#include "EncodeAgent.h"
#include "raid6_all.h"

void EncodeAgent::init()
{
	printf("Intialize encode agent!\n");
	currentFeedbackVP=0;
	currentGenID=0;
	currentGenIndex=0;

	encodeBuffer.buffer = new FILTER_NBL_PACKET[BlockSize_N-BlockSize_MaxP];
	encodeBuffer.ifOccupied = new bool[BlockSize_N-BlockSize_MaxP];
	memset(encodeBuffer.ifOccupied, 0, (BlockSize_N-BlockSize_MaxP)*sizeof(bool));
	encodeBuffer.head=0;
	encodeBuffer.tail=0;
	encodeBuffer.length=0;
	encodeBuffer.capacity=BlockSize_N-BlockSize_MaxP;

	for (int i = 0; i < encodeBuffer.capacity; i++) {
		FILTER_NBL_ACQUIRE_PACKET acquire_packet = { 0 };
		acquire_packet.m_size = MAX_PACKET_LENGTH+ReservedHeaderLength+2;////reserve 2 bytes more for parity option
		if (!parentFilter->AcquirePacket(&acquire_packet)) {
			printf("AcquirePacket failed\n");
			exit(0);
		}
		encodeBuffer.buffer[i] = acquire_packet.m_packet[0];
		encodeBuffer.buffer[i].SetPrefixLength(ReservedHeaderLength+2);////reserve 2 bytes more for parity option
	}

	FILTER_NBL_ACQUIRE_PACKET acquire_packet = { 0 };
	acquire_packet.m_size = MAX_PACKET_LENGTH+ReservedHeaderLength+2;////reserve 2 bytes more for parity option
	if (!parentFilter->AcquirePacket(&acquire_packet)) {
		printf("AcquirePacket failed\n");
		exit(0);
	}
	parityPacket = acquire_packet.m_packet[0];
	parityPacket.SetPrefixLength(ReservedHeaderLength+2);////reserve 2 bytes more for parity option

	send_size = sizeof(FILTER_NBL_FETCH_PACKET)+1*sizeof(FILTER_NBL_PACKET);
	send_packet = (PFILTER_NBL_SEND_PACKET)new char[send_size];
	if (!send_packet) {
		printf("send_packet new failed\n");
		exit(0);
	}
	send_packet->m_count=0;
	maxLenInCurrentGen=0;

	VPSelectCondition = new int [VPNumber];
	memset(VPSelectCondition, 0, VPNumber*sizeof(int));

	currentGeneratedParityNumber=0;
	encodeTimer.SetTimedEvent(this, &EncodeAgent::encodeTimeout);
}

void EncodeAgent::uninitialize()
{
	//printf("Uninitialize encode agent!\n");
	fflush(stdout);

	if (encodeTimer.IsStarted())
	{
		encodeTimer.Stop();
	}
	if (send_packet)
	{
		send_packet = NULL;
	}

	/////Need some code to release acquire packet!/////

	delete[] encodeBuffer.buffer;
	delete[] encodeBuffer.ifOccupied;
	delete[] VPSelectCondition;
}


bool EncodeAgent::Encode(FILTER_NBL_PACKET originalPacket)
{
	/*printf(" Encode packet!\n");*/

#if 0
	parentCLBTunnelAgent->PrintEncodeBuffer(0);
	if(currentGeneratedParityNumber>0)
	{
		/*printf("Free EncodeBuffer! BlockSize_K=%d\n",encodeBuffer.length);*/
		for(int j=0;j<encodeBuffer.length;j++)
		{
			int position=(encodeBuffer.head+j)%encodeBuffer.capacity;
			if(encodeBuffer.ifOccupied[position]==false)
			{
				printf("Free EncodeBuffer[%d] error!\n",position);
				parentCLBTunnelAgent->PrintEncodeBuffer(1);
				exit(0);
			}
			encodeBuffer.ifOccupied[position]=false;
		}
		encodeBuffer.head=0;
		encodeBuffer.tail=0;
		encodeBuffer.length=0;

		currentGeneratedParityNumber=0;
		currentGenID=(currentGenID+1)%MAX_GENERATION;
		currentGenIndex=0;
		maxLenInCurrentGen=0;
		parentCLBTunnelAgent->PrintEncodeBuffer(0);
	}

	if(BlockSize_MaxP>0)
	{
		encodeTimer.Stop();
		if (!encodeTimer.Start(ENCODE_TIMER_LENGTH))
		{
			printf("encodeTimer start failed!\n");
			exit(0);
		}
	}

	if(encodeBuffer.capacity==encodeBuffer.length)////Encode buffer full, shift it
	{
		/*printf("EncodeBuffer Full! Shift it!\n");*/

		///free the head packet
		if(encodeBuffer.ifOccupied[encodeBuffer.head]==false)
		{
			printf("Head is empty. EncodeBuffer[%d] error!\n",encodeBuffer.head);
			parentCLBTunnelAgent->PrintEncodeBuffer(1);
			exit(0);
		}
		encodeBuffer.ifOccupied[encodeBuffer.head]=false;
		encodeBuffer.length--;
		//shift the head polinter
		encodeBuffer.head=(encodeBuffer.head+1)%encodeBuffer.capacity;
	}

	//printf("Original packet!\n");
	//parentCLBTunnelAgent->print_bytes(originalPacket.GetBuffer(), originalPacket.GetPrefixLength()+originalPacket.GetLength());
	//then plug new packet in the tail
	encodeBuffer.ifOccupied[encodeBuffer.tail] = true;
	memcpy(encodeBuffer.buffer[encodeBuffer.tail].GetBuffer() + encodeBuffer.buffer[encodeBuffer.tail].GetPrefixLength(),
		originalPacket.GetBuffer() + originalPacket.GetPrefixLength(),
		originalPacket.GetLength());
	encodeBuffer.buffer[encodeBuffer.tail].SetLength(originalPacket.GetLength());
	encodeBuffer.tail=(encodeBuffer.tail+1)%encodeBuffer.capacity;
	if(originalPacket.GetLength()>maxLenInCurrentGen)
	{
		maxLenInCurrentGen=originalPacket.GetLength();
	}
	encodeBuffer.length++;

#endif

	IP_HEADER* old_ip_header;
	old_ip_header = (IP_HEADER*)(originalPacket.GetBuffer() +originalPacket.GetPrefixLength() + 14);
	setPacket(originalPacket,old_ip_header, false);


	///For debug only!
	/*int rnd = rand() % MAX_PACKETS_COUNT;
	if (currentGenIndex == rnd)
	{
		originalPacket.SetAction(Drop);
		parentCLBTunnelAgent->printIndex(originalPacket, "Explicit drop");
	}*/

	//printf("Encoded packet!\n");
	//parentCLBTunnelAgent->print_bytes(originalPacket.GetBuffer(), originalPacket.GetPrefixLength()+originalPacket.GetLength());
	//parentCLBTunnelAgent->PrintPacket(originalPacket,"Encoded packet");
#if 0
	if(encodeBuffer.length > encodeBuffer.capacity
		|| encodeBuffer.length <= 0)
	{
		printf("ERROR: encodeBuffer_.length > capacity, lenth=%d\n", encodeBuffer.length);
		parentCLBTunnelAgent->PrintEncodeBuffer(1);
		exit(0);
	}
	parentCLBTunnelAgent->PrintEncodeBuffer(0);

	parentCLBTunnelAgent->PrintPacket(originalPacket,"Finish encoding");
#endif

	parentCLBTunnelAgent->PrintPacket(originalPacket, "Finish encoding");
	/*parentCLBTunnelAgent->vp_mutex.lock();
	parentCLBTunnelAgent->PrintVPWindow(0);
	parentCLBTunnelAgent->vp_mutex.unlock();*/
	return true;
}

void EncodeAgent::setPacket(FILTER_NBL_PACKET pkt, IP_HEADER* old_ip_header, bool ifParity)
{
	ETHERNET_HEADER* old_eth_header;
	old_eth_header = (ETHERNET_HEADER*)(pkt.GetBuffer() + pkt.GetPrefixLength());
	memcpy(pkt.GetBuffer(), old_eth_header, sizeof(ETHERNET_HEADER));

	IP_HEADER* new_ip_header;
	new_ip_header = (IP_HEADER*)(pkt.GetBuffer() + 14);
	memcpy(new_ip_header, old_ip_header, sizeof(IP_HEADER));
	new_ip_header->ip_protocol = 17;

	if(ifParity==false)
	{
		new_ip_header->ip_total_length = htons((unsigned short)pkt.GetLength() - 14 + 36);
	}
	else
	{
		new_ip_header->ip_total_length = htons((unsigned short)pkt.GetLength() - 14 + 38);
	}
	new_ip_header->ip_header_len = 5;
	new_ip_header->ip_checksum = htons(0);///Need to initialize checksum field before calculate checksum!
	new_ip_header->ip_checksum = htons(ip_checksum(new_ip_header, 20));


	UDP_HEADER* udp_tunnel_header;
	udp_tunnel_header = (UDP_HEADER*)(pkt.GetBuffer() + 34);
	parentCLBTunnelAgent->vp_mutex.lock();
	udp_tunnel_header->src_port=htons(setVP(pkt));
	parentCLBTunnelAgent->vp_mutex.unlock();
	udp_tunnel_header->dst_port = htons(CLB_DEST_PORT);
	if(ifParity==false)
	{
		udp_tunnel_header->length=htons((unsigned short)pkt.GetLength() - 14 + 16);
	}
	else
	{
		udp_tunnel_header->length=htons((unsigned short)pkt.GetLength() - 14 + 18);
	}
	udp_tunnel_header->checksum=htons(0);

	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(pkt.GetBuffer() + 42);
	if(ifParity==false)
	{
		clb_header->ifParity=0;
		clb_header->parity_index=0;
	}
	else
	{
		clb_header->ifParity=1;
	}
	clb_header->reserved=0;
	clb_header->gen_id = currentGenID;
	clb_header->gen_index=htons(currentGenIndex);
	parentCLBTunnelAgent->vp_mutex.lock();
	clb_header->feedback_vp = htons(findNextFeedbackVP(currentFeedbackVP));
	clb_header->feedback_vp_acked_packets = htons(parentCLBTunnelAgent->FeedbackVPAckedPackets[ntohs(clb_header->feedback_vp)]);
	parentCLBTunnelAgent->vp_mutex.unlock();

	VPSelectCondition[ntohs(udp_tunnel_header->src_port)]++;
	currentFeedbackVP = ntohs(clb_header->feedback_vp);

	pkt.SetPrefixLength(0);
	if(ifParity==false)
	{
		pkt.SetLength(pkt.GetLength() + 36);
	}
	else
	{
		pkt.SetLength(pkt.GetLength() + 38);
	}
	currentGenIndex++;
}

void EncodeAgent::sendParityPacket()
{
	int BlockSize_K=encodeBuffer.length;

	if(BlockSize_K<=0 || BlockSize_K>encodeBuffer.capacity)
	{
		printf("ERROR: FLOW %d: Sending parity packets, but BlockSize_K=%d\n",
			parentCLBTunnelAgent->flowID,
			BlockSize_K);
		printf("encodeBuffer_.tail=%d, head=%d\n",
			encodeBuffer.tail,
			encodeBuffer.head);
		exit(0);
	}
	///Create parity data and assign to parityPacket

	if (currentGeneratedParityNumber == 0)
	{
		createParityPacket(parityPacket);
	}
	else if (currentGeneratedParityNumber == 1)
	{
		createParityPacketQ(parityPacket);
	}
	else
	{
		printf("Can't generate more than 2 pairties at this moment!\n");
		exit(0);
	}
	

	IP_HEADER* old_ip_header;
	int tail=(encodeBuffer.tail-1+encodeBuffer.capacity)%encodeBuffer.capacity;
	old_ip_header = (IP_HEADER*)(encodeBuffer.buffer[tail].GetBuffer() + encodeBuffer.buffer[tail].GetPrefixLength() + 14);
	
	setPacket(parityPacket,old_ip_header,true);

	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(parityPacket.GetBuffer() + parityPacket.GetPrefixLength() + 42);
	clb_header->parity_index=currentGeneratedParityNumber & 0xf;
	CLB_PARITY_OPTION* clb_option;
	clb_option = (CLB_PARITY_OPTION*)(parityPacket.GetBuffer() + parityPacket.GetPrefixLength() + 50);
	clb_option->head_gen_index 
		= htons((ntohs(clb_header->gen_index) 
		- BlockSize_K 
		- clb_header->parity_index 
		+ MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT);

	currentGeneratedParityNumber++;

	/*printf("+++++++++++++++parityPacket data++++++++++\n"); 
	parentCLBTunnelAgent->print_bytes(parityPacket.GetBuffer()+parityPacket.GetPrefixLength(),parityPacket.GetLength());
*/
	int blocksize
		=(ntohs(clb_header->gen_index)
		- clb_header->parity_index
		- ntohs(clb_option->head_gen_index)
		+ MAX_PACKETS_COUNT) % MAX_PACKETS_COUNT;
	if(blocksize<=0 || blocksize>encodeBuffer.capacity)
	{
		printf("ERROR: parityPacket->blockSize setting error!\n");
		printf("gen_index=%u, parity_index=%u, head_gen_index=%u\n",
			ntohs(clb_header->gen_index),
			clb_header->parity_index,
			ntohs(clb_option->head_gen_index));
		exit(0);
	}

	parentCLBTunnelAgent->parity_generated++;
	printf("FLOW %d: parity_generated=%d\n",
		parentCLBTunnelAgent->flowID,
		parentCLBTunnelAgent->parity_generated);

	if(currentGeneratedParityNumber>BlockSize_MaxP)
	{
		printf("ERROR: currentGeneratedParityNumber=%d, > BlockSize_MaxP!\n",currentGeneratedParityNumber);
		exit(0);
	}

	parentCLBTunnelAgent->PrintPacket(parityPacket,"Send parity");
	send_packet->m_packet[0]=parityPacket;
	send_packet->m_count++;
	if(send_packet->m_count != 1)
	{
		printf("ERROR: send_packet->m_count = %lu\n",send_packet->m_count);
		exit(0);
	}
	LPOVERLAPPED overlapped = new OVERLAPPED;
	memset(overlapped, 0, sizeof(OVERLAPPED));
	overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	DWORD err;
	err = parentFilter->SendPacket(send_packet, send_size, overlapped);
	if (err != ERROR_SUCCESS &&
		err != ERROR_IO_PENDING) {
		printf("SendPacket error: %d\n", err);
		exit(0);
	}
	WaitForSingleObject(overlapped->hEvent, INFINITE);
	send_packet->m_count=0;
}

void EncodeAgent::createParityPacket(FILTER_NBL_PACKET pkt)
{
	/*printf("---------------Create parity packet XOR P, maxLenInCurrentGen=%lu!-------------\n",maxLenInCurrentGen); */
	if (maxLenInCurrentGen == 0)
	{
		printf("ERROR: maxLenInCurrentGen=0!\n");
		encodeTimer.Stop();
		exit(0);
	}
	unsigned char* data;
	data = new unsigned char[maxLenInCurrentGen - 14];/////minus the length of ethernet header
	memset(data, 0, (maxLenInCurrentGen - 14)*sizeof(unsigned char));
	unsigned char* encodedData;
	encodedData = new unsigned char[maxLenInCurrentGen - 14];/////minus the length of ethernet header
	memset(encodedData, 0, (maxLenInCurrentGen - 14)*sizeof(unsigned char));

	int blocksize=encodeBuffer.length;
	for (int k = 0; k < blocksize; k++)
	{
		int i=(encodeBuffer.head+k)%encodeBuffer.capacity;
		if (encodeBuffer.ifOccupied[i] == false)
		{
			printf("Encode buffer error. It's empty while generating parity packets!\n");
			encodeTimer.Stop();
			exit(0);
		}
		memcpy(data
			, (encodeBuffer.buffer[i].GetBuffer() + encodeBuffer.buffer[i].GetPrefixLength() + 14)
			, encodeBuffer.buffer[i].GetLength() - 14);
		for (unsigned int j = 0; j < maxLenInCurrentGen - 14; j++)
		{
			encodedData[j] = encodedData[j] ^ data[j];
		}
	}
	/*printf("+++++++++++++++encodedData++++++++++\n"); 
	parentCLBTunnelAgent->print_bytes(encodedData,maxLenInCurrentGen-14);*/

	pkt.SetLength(maxLenInCurrentGen);
	pkt.SetPrefixLength(ReservedHeaderLength + 2);////reserve 2 bytes more for parity option
	ETHERNET_HEADER* eth_header;
	int tail=(encodeBuffer.tail-1+encodeBuffer.capacity)%encodeBuffer.capacity;
	eth_header = (ETHERNET_HEADER*)(encodeBuffer.buffer[tail].GetBuffer() + encodeBuffer.buffer[tail].GetPrefixLength());
	memcpy(pkt.GetBuffer() + pkt.GetPrefixLength()
		, eth_header
		, sizeof(ETHERNET_HEADER));
	memcpy(pkt.GetBuffer() + pkt.GetPrefixLength() + 14
		, encodedData
		, (maxLenInCurrentGen - 14)*sizeof(unsigned char));
}

void EncodeAgent::createParityPacketQ(FILTER_NBL_PACKET pkt)
{
	/*printf("---------------Create parity packet RS Q, maxLenInCurrentGen=%lu!-------------\n", maxLenInCurrentGen);*/
	if (maxLenInCurrentGen == 0)
	{
		printf("ERROR: maxLenInCurrentGen=0!\n");
		encodeTimer.Stop();
		exit(0);
	}
	int blocksize = encodeBuffer.length;

	int maxDataSize = maxLenInCurrentGen - 14;
	int offset = maxDataSize % sizeof(long);
	if (offset != 0)
	{
		maxDataSize = sizeof(long) - offset + maxLenInCurrentGen - 14;
	}
	
	/////minus the length of ethernet header, and align with sizeof(long)
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

	for (int k = 0; k < blocksize; k++)
	{
		int i = (encodeBuffer.head + k) % encodeBuffer.capacity;
		if (encodeBuffer.ifOccupied[i] == false)
		{
			printf("Encode buffer error. It's empty while generating parity packets!\n");
			encodeTimer.Stop();
			exit(0);
		}
		/*printf("+++++++++++++++orignial packet++++++++++\n");
		parentCLBTunnelAgent->print_bytes(
			encodeBuffer.buffer[i].GetBuffer() + encodeBuffer.buffer[i].GetPrefixLength() + 14,
			encodeBuffer.buffer[i].GetLength() - 14);*/
		
		memcpy(data[k]
			, (encodeBuffer.buffer[i].GetBuffer() + encodeBuffer.buffer[i].GetPrefixLength() + 14)
			, encodeBuffer.buffer[i].GetLength() - 14);

		/*printf("+++++++++++++++copied data++++++++++\n");
		parentCLBTunnelAgent->print_bytes(data[k], maxDataSize);
		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");*/
	}

	/*printf("==================================\n");
	for (int i = 0; i < blocksize; i++)
	{
		printf("+++++++++++++++copied data++++++++++\n");
		parentCLBTunnelAgent->print_bytes(data[i], maxDataSize);
	}

	printf("begin reed_sol_r6_encode\n");*/

	reed_sol_r6_encode(blocksize, 8, (char **)data, (char **)encodedData, maxDataSize);

	/*for (int i = 0; i < BlockSize_MaxP; i++)
	{
		printf("+++++++++++++++encodedData++++++++++\n");
		parentCLBTunnelAgent->print_bytes(encodedData[i], maxDataSize);
	}*/

	pkt.SetLength(maxLenInCurrentGen);
	pkt.SetPrefixLength(ReservedHeaderLength + 2);////reserve 2 bytes more for parity option
	ETHERNET_HEADER* eth_header;
	int tail = (encodeBuffer.tail - 1 + encodeBuffer.capacity) % encodeBuffer.capacity;
	eth_header = (ETHERNET_HEADER*)(encodeBuffer.buffer[tail].GetBuffer() + encodeBuffer.buffer[tail].GetPrefixLength());
	memcpy(pkt.GetBuffer() + pkt.GetPrefixLength()
		, eth_header
		, sizeof(ETHERNET_HEADER));
	memcpy(pkt.GetBuffer() + pkt.GetPrefixLength() + 14
		, encodedData[1]
		, maxDataSize);

	/*printf("+++++++++++++++encoded Packet++++++++++\n");
	parentCLBTunnelAgent->print_bytes(pkt.GetBuffer() + pkt.GetPrefixLength(), pkt.GetLength());*/
}

unsigned short EncodeAgent::findBestVP()
{
	unsigned short k = rand() % VPNumber;
	unsigned short  min = k;

	int flag = 0;
	for (int i = 0; i < VPNumber; i++)/////Pick the first used VP
	{
		if (parentCLBTunnelAgent->VPRateEstimation[(k + i) % VPNumber] != 0 && parentCLBTunnelAgent->VPCost[(k + i) % VPNumber] != -1)
		{
			min = (k + i) % VPNumber;
			flag = 1;
			break;
		}
	}
	if (flag == 0)
	{
		printf( "%s\n", "ERROR: findBestVP called, but no active VP!");
		parentCLBTunnelAgent->PrintVPWindow(1);
		exit(0);
	}

	for(int i=0;i<VPNumber;i++)
	{	
		if(parentCLBTunnelAgent->VPRateEstimation[(k+i)%VPNumber]==0 )
		{
			continue;
		}
		if(parentCLBTunnelAgent->VPRateEstimation[min]==0)
		{
			printf( "ERROR: VPRateEstimation[min]==0! min=%d\n",min);
			parentCLBTunnelAgent->PrintVPWindow(1);
			exit(0);
		}

		if(	parentCLBTunnelAgent->VPCost[(k+i)%VPNumber]!=-1 
			&& (parentCLBTunnelAgent->VPCost[(k+i)%VPNumber]+1/parentCLBTunnelAgent->VPRateEstimation[(k+i)%VPNumber])
			<(parentCLBTunnelAgent->VPCost[min]+1/parentCLBTunnelAgent->VPRateEstimation[min]))
		{
			min=(k+i)%VPNumber;
		} 
	}
	return min;
}


int EncodeAgent::createNewVP()
{
	unsigned short k = rand() % VPNumber;
	int newPath = -1;

	for (int i = 0; i < VPNumber; i++)
	{
		if (parentCLBTunnelAgent->VPRateEstimation[(k + i) % VPNumber] == 0 && parentCLBTunnelAgent->VPCost[(k + i) % VPNumber] != -1)
		{
			newPath = (k + i) % VPNumber;
			break;
		}
	}

	return newPath;
}


unsigned short EncodeAgent::setVP(FILTER_NBL_PACKET pkt)
{
	// if(usedVP>countExploredOnceVP && usedVP>MAINTAIN_VP_UPHOLD)
	// {
	// 	clearWorstVP();
	// 	// printf("clearWorstVP\n");
	// 	// PrintVPWindow(1);
	// }

	int VP = 0;
	CLB_HEADER* clb_header;
	clb_header = (CLB_HEADER*)(pkt.GetBuffer() + 42);

	if((BlockSize_MaxP>0 && clb_header->ifParity==0)/////for coded LB
	|| (BlockSize_MaxP==0)) /////for no code LB
	{
		if(parentCLBTunnelAgent->usedVP<INITIAL_PATH_NUMBER)
		{
			VP=createNewVP();
			if(VP!=-1)////New VP created			
			{
				parentCLBTunnelAgent->VPLastUpdateTime[VP]=clock();
				parentCLBTunnelAgent->VPCost[VP]=-1;/////-1 means has been explored once!
				parentCLBTunnelAgent->countExploredOnceVP++;
				parentCLBTunnelAgent->usedVP++;
			}
			else///// all VP has been explored once!
			{
				VP=rand()%VPNumber;
				/*printf("Can't create new VP, randomly select %d\n",VP);*/
				if(parentCLBTunnelAgent->usedVP!=VPNumber)
				{
					printf("All VP should be explored once at this moment\n");
					parentCLBTunnelAgent->PrintVPWindow(1);
					exit(0);
				}
				if(MAINTAIN_VP_UPHOLD<VPNumber)
				{
					printf("ERROR: No way to fail creating new VP\n");
					parentCLBTunnelAgent->PrintVPWindow(1);
					exit(0);
				}
			}
		}
		else if(parentCLBTunnelAgent->usedVP>parentCLBTunnelAgent->countExploredOnceVP)
		{
			VP=findBestVP();
		}
		else if(parentCLBTunnelAgent->usedVP==parentCLBTunnelAgent->countExploredOnceVP)
		{
			VP=rand()%VPNumber;
			if(parentCLBTunnelAgent->VPCost[VP]!=-1)
			{
				parentCLBTunnelAgent->VPLastUpdateTime[VP]=clock();	
				parentCLBTunnelAgent->VPCost[VP]=-1;
				parentCLBTunnelAgent->countExploredOnceVP++;
				parentCLBTunnelAgent->usedVP++;
			}
		}
		else
		{
			printf("VP select ERROR!\n");
			parentCLBTunnelAgent->PrintVPWindow(1);
			exit(0);
		}
	}
	else /////////Parity packets
	{
		VP=createNewVP();
		if(VP!=-1)////New VP created	
		{
			parentCLBTunnelAgent->VPLastUpdateTime[VP]=clock();
			parentCLBTunnelAgent->VPCost[VP]=-1;/////-1 means has been explored once!
			parentCLBTunnelAgent->countExploredOnceVP++;
			parentCLBTunnelAgent->usedVP++;
		}
		else
		{
			if(parentCLBTunnelAgent->usedVP!=VPNumber)
			{
				printf("All VP should be explored at this moment\n");
				parentCLBTunnelAgent->PrintVPWindow(1);
				exit(0);
			}

			if(parentCLBTunnelAgent->usedVP>parentCLBTunnelAgent->countExploredOnceVP)
			{
				VP=findBestVP();
			}
			else////All explored once
			{
				VP=rand()%VPNumber;
			}
		}
	}

	if(parentCLBTunnelAgent->VPPacketsSent[VP]+1>=MAX_PACKETS_COUNT)
	{
		IP_HEADER out_ip_header;
		memcpy(&out_ip_header,pkt.GetBuffer() + pkt.GetPrefixLength() + 14,sizeof(IP_HEADER));
		in_addr outSrc;
		outSrc.S_un.S_addr = out_ip_header.ip_srcaddr;
		in_addr outDst;
		outDst.S_un.S_addr = out_ip_header.ip_destaddr;
		char strOutSrc[256];
		sprintf_s(strOutSrc,"%s", inet_ntoa(outSrc));
		char strOutDst[256];
		sprintf_s(strOutDst,"%s", inet_ntoa(outDst));
		printf("VPPacketsSent Overflow!\n");
		printf("[thread %d]: Flow%d(%s-%s) VPPacketsSent[%d]=%d Overflow!\n"
			,GetCurrentProcessorNumber()
			,parentCLBTunnelAgent->flowID
			,strOutSrc
			,strOutDst
			,VP
			,parentCLBTunnelAgent->VPPacketsSent[VP]);
		// PrintVPWindow(1);
		// exit(0);
	}
	parentCLBTunnelAgent->VPPacketsSent[VP]=(parentCLBTunnelAgent->VPPacketsSent[VP]+1)%MAX_PACKETS_COUNT;
	parentCLBTunnelAgent->VPPacketsOnFly[VP]++;
	if(parentCLBTunnelAgent->VPPacketsOnFly[VP]>=MAX_PACKETS_COUNT)
	{
		IP_HEADER out_ip_header;
		memcpy(&out_ip_header,pkt.GetBuffer() + pkt.GetPrefixLength() + 14,sizeof(IP_HEADER));
		in_addr outSrc;
		outSrc.S_un.S_addr = out_ip_header.ip_srcaddr;
		in_addr outDst;
		outDst.S_un.S_addr = out_ip_header.ip_destaddr;
		char strOutSrc[256];
		sprintf_s(strOutSrc,"%s", inet_ntoa(outSrc));
		char strOutDst[256];
		sprintf_s(strOutDst,"%s", inet_ntoa(outDst));
		printf("VPPacketsOnFly Overflow!\n");
		printf("[thread %d]: Flow%d(%s-%s) VPPacketsOnFly[%d]=%d Overflow!\n"
			,GetCurrentProcessorNumber()
			,parentCLBTunnelAgent->flowID
			,strOutSrc
			,strOutDst
			,VP
			,parentCLBTunnelAgent->VPPacketsOnFly[VP]);
		parentCLBTunnelAgent->PrintVPWindow(1);
		exit(0);
	}

	if(parentCLBTunnelAgent->VPCost[VP]!=-1)/////update VPCost!!
	{
		if(parentCLBTunnelAgent->VPRateEstimation[VP]==0)
		{
			printf("ERROR: select vp=%d, cost and rate not match!\n",VP);
			parentCLBTunnelAgent->PrintVPWindow(1);
			exit(0);
		}
		parentCLBTunnelAgent->VPCost[VP]=parentCLBTunnelAgent->VPPacketsOnFly[VP]/parentCLBTunnelAgent->VPRateEstimation[VP];
	}

	return VP;
}

unsigned short EncodeAgent::findNextFeedbackVP(unsigned short vp)
{
	unsigned short nextVP = (vp + 1) % VPNumber;

	for (unsigned short i = 0; i < VPNumber; i++)
	{
		if (parentCLBTunnelAgent->FeedbackVPAckedPackets[(nextVP + i) % VPNumber] != 0)
		{
			nextVP = (nextVP + i) % VPNumber;
			break;
		}
	}

	return nextVP;
}

void EncodeAgent::encodeTimeout()
{
	parentCLBTunnelAgent->encodePacket_mutex.lock();
	/*printf("===========Encode Timeout!!=============\n");*/
	if(currentGeneratedParityNumber<BlockSize_MaxP)
	{
		parentCLBTunnelAgent->PrintEncodeBuffer(0);
		encodeTimer.Stop();
		if (!encodeTimer.Start(ENCODE_TIMER_LENGTH))
		{
			printf("encodeTimer start failed!\n");
			exit(0);
		}
		sendParityPacket();
	}
	else
	{
		encodeTimer.Stop();
	}
	parentCLBTunnelAgent->encodePacket_mutex.unlock();
}