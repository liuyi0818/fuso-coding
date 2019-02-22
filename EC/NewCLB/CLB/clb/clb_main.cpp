#include "clb_main.h"

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#include <vector>
#include <sstream>
#include <string>
#include <iostream>


std::mutex flowTable_mutex;

//wchar_t* adapter_name = L"Microsoft Hyper-V Network Adapter";
//wchar_t* adapter_name = L"Intel(R) PRO/1000 MT Network Connection";
wchar_t* adapter_name;

//HANDLE exitEvent;

char otherEndAddress[128];

char localAddress[128];

void initCLB()
{
	if (BlockSize_MaxP > 2)
	{
		printf("BlockSize_MaxP should be 0|1|2 !\n");
		exit(0);
	}

	if(packetsPerOneFetchForRecv!=1)
	{
		printf("packetsPerOneFetchForRecv should be  1!\n");
		exit(0);
	}
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

int main(int argc, char** argv) {

	PIP_ADAPTER_INFO pAdapterInfo;
	pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
	ULONG buflen = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(buflen);
	}

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
		while (pAdapter) {
			std::vector<std::string> ip = split(pAdapter->IpAddressList.IpAddress.String, '.');

			if (ip.front() == "10")
			{
				sprintf_s(localAddress, "%s", pAdapter->IpAddressList.IpAddress.String);
				
				DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, pAdapter->Description, -1, NULL, 0);
				adapter_name = new wchar_t[dwNum];
				if (!adapter_name)
				{
					delete[]adapter_name;
				}
				MultiByteToWideChar(CP_ACP, 0, pAdapter->Description, -1, adapter_name, dwNum);

				std::wcout << "\tAdapter Desc : \t" << adapter_name << std::endl;
				printf("\tIP Address: \t%s\n", localAddress);
				break;
			}
			pAdapter = pAdapter->Next;
		}
	}
	else {
		printf("Call to GetAdaptersInfo failed.\n");
	}


	/*memset(otherEndAddress, 0, 128 * sizeof(char));
	if (argc == 2)
	{
		strcpy_s(otherEndAddress, argv[1]);
		printf("otherEndAddress = %s\n", otherEndAddress);
	}
	else
	{
		printf("usage: clb.exe otherEndAddress\n");
		exit(0);
	}*/

	initCLB();

    // new a PacketFilter
    PacketFilter* filter = nullptr;
    filter = PacketFilter::NewPacketFilter();
    if (!filter) {
        printf("new PacketFilter failed\n");
        goto error_exit;
    }
    printf("new PacketFilter\n");

    if (!filter->Initialize(adapter_name)) {
        printf("PacketFilter initialize failed\n");
        goto error_exit;
    }
    printf("PacketFilter initialized\n");

    DWORD pros = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);


    // rss is set to core 0 ~ 15, so we enable packet filtering on these cores
    KAFFINITY affinity = 0;
    for(unsigned i=0; i < 16; i++)
        affinity |= (KAFFINITY)1 << i;
    printf("affinity: 0x%I64x\n", affinity);

    if (!filter->EnablePacketFiltering(
		FILTER_FLAG_CAPTURE_RECV_PACKET | FILTER_FLAG_CAPTURE_SEND_PACKET,
        affinity,
        1024,
        MAX_PACKET_LENGTH,
		ReservedHeaderLength,
        0,
        Drop,
        /* don't indicate packet by packet, to reduce the indication overhead for better performance, every 2500 * 100-nanosecond indicate once. */
        /* KeSetTimer for detail */
        0)) {
        printf("PacketFilter enable failed\n");
        goto error_exit;
    }
    printf("PacketFilter enabled, packetsPerOneFetchForSend=%d, packetsPerOneFetchForRecv=%d\n",
		packetsPerOneFetchForSend,
		packetsPerOneFetchForRecv);

	FlowClassifier* flowClassifier;
	flowClassifier=new FlowClassifier(filter);
	FILTER_AND_FLOWCLASSIFIER* passParameter=new FILTER_AND_FLOWCLASSIFIER [1];
	passParameter->filter=filter;
	passParameter->flowClassifier=flowClassifier;


	for (unsigned i = 0; i < pros; i++) {
        HANDLE thread;
        thread = (HANDLE)_beginthreadex(NULL, 0, recv_thread, passParameter, CREATE_SUSPENDED, NULL);
        if (!SetThreadAffinityMask(thread, 1 << i))
            printf("SetThreadAffinityMask failed: %d\n", GetLastError());
        ResumeThread(thread);
    }

	HANDLE threadSend;
	threadSend = (HANDLE)_beginthreadex(
		NULL,
		0,
		send_thread,
		passParameter,
		0,
		NULL);
	if (!threadSend) {
		printf("threadSend _beginthreadex failed\n");
		goto error_exit;
	}



    printf("press any key to exit...\n");
    getchar();

    filter->DisablePacketFiltering();

error_exit:

    return 0;
}