#include "clb_main.h"

unsigned __stdcall
recv_thread(
void* context) {
	FILTER_AND_FLOWCLASSIFIER* passParameter = (FILTER_AND_FLOWCLASSIFIER*)context;
	PacketFilter* filter;
	filter = passParameter->filter;
	FlowClassifier* flowClassifier;
	flowClassifier = passParameter->flowClassifier;

	unsigned long count;
	unsigned long fetch_size;
	count = packetsPerOneFetchForRecv;

	PFILTER_NBL_FETCH_PACKET fetch_packet;
	fetch_size = sizeof(FILTER_NBL_FETCH_PACKET)+count * sizeof(FILTER_NBL_PACKET);
	fetch_packet = (PFILTER_NBL_FETCH_PACKET)new char[fetch_size];
	memset(fetch_packet, 0, fetch_size);
	fetch_packet->m_direction = Recv;
	fetch_packet->m_processor_id = GetCurrentProcessorNumber(); // set the processor you want to fetch (RSS)
	printf("GetCurrentProcessorNumber(): %d, recv_thread %d\n", GetCurrentProcessorNumber(), fetch_packet->m_processor_id);

	LPOVERLAPPED overlapped = new OVERLAPPED;
	memset(overlapped, 0, sizeof(OVERLAPPED));
	overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	while(1) {
		/*printf("[%d] Fetching....\n", fetch_packet->m_processor_id);*/

		fetch_packet->m_count = count;
		DWORD err;
		err = filter->FetchPacket(fetch_packet, fetch_size, overlapped);
		if (err != ERROR_SUCCESS &&
			err != ERROR_IO_PENDING) {
			printf("[%d] fetch error: %d\n", fetch_packet->m_processor_id, err);
			break;
		}

		WaitForSingleObject(overlapped->hEvent, INFINITE);
		
		//printf("[%d] fetch a packet\n", fetch_packet->m_processor_id);

		DWORD byteRet;
		if (GetOverlappedResult(filter->Handle(),overlapped,&byteRet,FALSE)) 
		{
			/*///For luo paul use
			if (!filter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)fetch_packet, fetch_size))
			{
				printf("Decode ProcessPacket failed\n");
				exit(0);
			}
			continue;
			///For luo paul use*/


				ETHERNET_HEADER* eth_header;
				eth_header = (ETHERNET_HEADER*)(fetch_packet->m_packet[0].GetBuffer() + fetch_packet->m_packet[0].GetPrefixLength());

				if (0x0800 != ntohs(eth_header->m_type))/////We only deal with IP packet!
				{
					if (!filter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)fetch_packet, fetch_size))
					{
						printf("Decode ProcessPacket failed\n");
						exit(0);
					}
					continue;
				}
			
				IP_HEADER* ip_header;
				ip_header = (IP_HEADER*)(fetch_packet->m_packet[0].GetBuffer() + fetch_packet->m_packet[0].GetPrefixLength() + 14);

				PrintIPPacket(fetch_packet->m_packet[0], "Fectch a",0);

				if (17 == ip_header->ip_protocol)
				{
					UDP_HEADER* udp_tunnel_header;
					udp_tunnel_header = (UDP_HEADER*)(fetch_packet->m_packet[0].GetBuffer() + fetch_packet->m_packet[0].GetPrefixLength() + 34);

					if (5217 == ntohs(udp_tunnel_header->dst_port))
						///Packets will be processed within Decode(fetch_packet, size);
					{

						if (ip_header->ip_srcaddr == inet_addr(localAddress)
							|| ip_header->ip_destaddr != inet_addr(localAddress))
						{
							PrintIPPacket(fetch_packet->m_packet[0],"recv_thread Fectch and Drop",1);
							fetch_packet->m_packet[0].SetAction(Drop);
							if (!filter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)fetch_packet, fetch_size))
							{
								printf("Decode ProcessPacket failed\n");
								exit(0);
							}
							continue;
						}

						flowTable_mutex.lock();
						unsigned int srcaddr = ip_header->ip_srcaddr;
						CLBTunnelAgent* clb_tunnel = flowClassifier->getCLBTunnelAgent(srcaddr);
						flowTable_mutex.unlock();

						clb_tunnel->decodePacket_mutex.lock();
						if (!clb_tunnel->Decode(fetch_packet, fetch_size))
						{
							printf("Decode packet failed\n");
							clb_tunnel->decodePacket_mutex.unlock();
							exit(0);
						}
						clb_tunnel->decodePacket_mutex.unlock();
					}
					else
					{
						if (!filter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)fetch_packet, fetch_size))
						{
							printf("Decode ProcessPacket failed\n");
							exit(0);
						}
						continue;
					}
				}
				else
				{
					if (!filter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)fetch_packet, fetch_size))
					{
						printf("Decode ProcessPacket failed\n");
						exit(0);
					}
					continue;
				}
		}
		else {
			err = GetLastError();
			printf("[%d] fetch error: %d\n", GetCurrentProcessorNumber(), err);
			break;
		}
	}

	printf("recv_thread [%d] exit!\n", fetch_packet->m_processor_id);
	return 0;
}


unsigned __stdcall
send_thread(
void* context) {

	FILTER_AND_FLOWCLASSIFIER* passParameter = (FILTER_AND_FLOWCLASSIFIER*)context;
	PacketFilter* filter;
	filter = passParameter->filter;
	FlowClassifier* flowClassifier;
	flowClassifier = passParameter->flowClassifier;

	unsigned long count;
	unsigned long fetch_size;
	unsigned long recv_size;
	count = packetsPerOneFetchForSend;

	PFILTER_NBL_FETCH_PACKET fetch_packet;
	fetch_size = sizeof(FILTER_NBL_FETCH_PACKET)+count * sizeof(FILTER_NBL_PACKET);
	fetch_packet = (PFILTER_NBL_FETCH_PACKET)new char[fetch_size];
	memset(fetch_packet, 0, fetch_size);
	fetch_packet->m_direction = Send;
	fetch_packet->m_processor_id = GetCurrentProcessorNumber(); // set the processor you want to fetch (RSS)
	printf("GetCurrentProcessorNumber(): %d, send_thread %d\n", GetCurrentProcessorNumber(), fetch_packet->m_processor_id);
	LPOVERLAPPED overlapped = new OVERLAPPED;
	memset(overlapped, 0, sizeof(OVERLAPPED));
	overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	while (1)
	{
		fetch_packet->m_count = count;
		DWORD err;
		err = filter->FetchPacket(fetch_packet, fetch_size, overlapped);
		if (err != ERROR_SUCCESS &&
			err != ERROR_IO_PENDING) {
			printf("[%d] fetch error: %d\n", fetch_packet->m_processor_id, err);
			break;
		}
		WaitForSingleObject(overlapped->hEvent, INFINITE);

		//printf("Thread %d: Send a packet!\n", GetCurrentProcessorNumber());

		DWORD byteRet;
		if (GetOverlappedResult(	filter->Handle(), overlapped, &byteRet, FALSE)) 
		{
			for (unsigned long j = 0; j < fetch_packet->m_count; j++) /////Deal with every packet fetched, bypass the none CLB packet
			{
				/*///For luo paul use
				int a = 1;
				continue;*/

				ETHERNET_HEADER* eth_header;
				eth_header = (ETHERNET_HEADER*)(fetch_packet->m_packet[j].GetBuffer() + fetch_packet->m_packet[j].GetPrefixLength());

				if (0x0800 != ntohs(eth_header->m_type))/////We only deal with IP packet!
				{
					continue;
				}
				IP_HEADER* ip_header;
				ip_header = (IP_HEADER*)(fetch_packet->m_packet[j].GetBuffer() + fetch_packet->m_packet[j].GetPrefixLength() + 14);

				///////For debug, only encode some certain dstIP packets
				//if (ip_header->ip_destaddr != inet_addr(otherEndAddress))
				//{
				//	continue;
				//}

				flowTable_mutex.lock();
				unsigned int destaddr = ip_header->ip_destaddr;
				CLBTunnelAgent* clb_tunnel = flowClassifier->getCLBTunnelAgent(destaddr);
				//printf("Finish flowClassifier!\n");
				flowTable_mutex.unlock();

				clb_tunnel->encodePacket_mutex.lock();
				if (!clb_tunnel->Encode(fetch_packet->m_packet[j]))
				{
					printf("Encode packet failed\n");
					fflush(stdout);
					clb_tunnel->encodePacket_mutex.unlock();
					exit(0);
				}
				//printf("Encode finished!\n");
				clb_tunnel->encodePacket_mutex.unlock();
			}

			////Process all fetched packets at once! Send out them!
			filter->ProcessPacket((PFILTER_NBL_PROCESS_PACKET)fetch_packet, fetch_size);
		}
		else {
			err = GetLastError();
			printf("[%d] fetch error: %d\n", GetCurrentProcessorNumber(), err);
			break;
		}
	}

	printf("send_thread [%d] exit!\n", fetch_packet->m_processor_id);
	return 0;
}