#ifndef clb_main_h
#define clb_main_h

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <mutex>
#include <winsock.h>
#pragma comment(lib, "WS2_32")
#include "../../include/lib.h"

#define CLB_DEST_PORT 5217
#define packetsPerOneFetchForSend 10 //////
#define packetsPerOneFetchForRecv 1 //////Should only be 1!!
#define ReservedHeaderLength 36
#define MAX_PACKET_LENGTH 1600


#define BlockSize_N 202
#define BlockSize_MaxP 0///////Should be 0 or 1
#define BlockSize_MaxK (BlockSize_N-BlockSize_MaxP)

#define IF_PRINT_DEBUG 0
#define IF_PRINT_SEQTIMELINE 0
#define IF_PRINT_VPWINDOWTIMELINE 0

#define ENCODE_TIMER_LENGTH 50 ////millisecond
#define DECODE_TIMER_LENGTH 150    //((BlockSize_MaxP+1)*ENCODE_TIMER_LENGTH)
#define ALPHA 0.5
#define VPNumber 8
#define ReorderGenNumber 3
#define MAX_PACKETS_COUNT 65536
#define INITIAL_PATH_NUMBER 8
#define MAX_GENERATION 65536

#define MAINTAIN_VP_UPHOLD 30
#define MAINTAIN_VP_LOWHOLD 20

//extern HANDLE exitEvent;

#ifdef _DEBUG
#ifdef _WIN64
#pragma comment(lib, "../x64/debug/pfilter.lib")
#else
#pragma comment(lib, "../../lib/debug/pfilter.lib")
#endif
#else
#ifdef _WIN64
#pragma comment(lib, "../x64/release/pfilter.lib")
#else
#pragma comment(lib, "../../lib/release/pfilter.lib")
#endif
#endif

struct ETHERNET_HEADER {
	unsigned char m_dst_mac[6];
	unsigned char m_src_mac[6];
	unsigned short m_type;
};

struct IP_HEADER{
	unsigned char ip_header_len : 4; // 4-bit header length (in 32-bit words) normally=5 (Means 20 Bytes may be 24 also)
	unsigned char ip_version : 4; // 4-bit IPv4 version
	unsigned char ip_tos; // IP type of service
	unsigned short ip_total_length; // Total length
	unsigned short ip_id; // Unique identifier
	unsigned char ip_frag_offset : 5; // Fragment offset field
	unsigned char ip_more_fragment : 1;
	unsigned char ip_dont_fragment : 1;
	unsigned char ip_reserved_zero : 1;
	unsigned char ip_frag_offset1; //fragment offset
	unsigned char ip_ttl; // Time to live
	unsigned char ip_protocol; // Protocol(TCP,UDP etc)
	unsigned short ip_checksum; // IP checksum
	unsigned int ip_srcaddr; // Source address
	unsigned int ip_destaddr; // Source address
};

typedef struct tcp_header
{
	unsigned short source_port;   // source port
	unsigned short dest_port;     // destination port
	unsigned int sequence;        // sequence number - 32 bits
	unsigned int acknowledge;     // acknowledgement number - 32 bits

	unsigned char ns : 1;          //Nonce Sum Flag Added in RFC 3540.
	unsigned char reserved_part1 : 3; //according to rfc
	unsigned char data_offset : 4;    /*The number of 32-bit words in the TCP header.
									  This indicates where the data begins.
									  The length of the TCP header
									  is always a multiple
									  of 32 bits.*/

	unsigned char fin : 1; //Finish Flag
	unsigned char syn : 1; //Synchronise Flag
	unsigned char rst : 1; //Reset Flag
	unsigned char psh : 1; //Push Flag
	unsigned char ack : 1; //Acknowledgement Flag
	unsigned char urg : 1; //Urgent Flag

	unsigned char ecn : 1; //ECN-Echo Flag
	unsigned char cwr : 1; //Congestion Window Reduced Flag

	////////////////////////////////

	unsigned short window; // window
	unsigned short checksum; // checksum
	unsigned short urgent_pointer; // urgent pointer
} TCP_HEADER;

typedef struct udp_header
{
	unsigned short src_port;
	unsigned short dst_port;
	unsigned short length;
	unsigned short checksum;
} UDP_HEADER;


struct CLB_HEADER{
	unsigned char ifParity : 1; //Parity flag
	unsigned char reserved : 3;
	unsigned char parity_index : 4; //Parity index
	unsigned char gen_id;
	unsigned short gen_index;
	unsigned short feedback_vp;
	unsigned short feedback_vp_acked_packets;
};

struct CLB_PARITY_OPTION{
	unsigned short head_gen_index;
};

#include "flow_classify.h"

class FlowClassifier;

struct FILTER_AND_FLOWCLASSIFIER
{
	PacketFilter* filter;
	FlowClassifier* flowClassifier;
};

unsigned __stdcall recv_thread(void* context);
unsigned __stdcall send_thread(void* context);


unsigned __stdcall fetch_thread(void* context);

void PrintIPPacket(FILTER_NBL_PACKET pkt, char SendorReceive[128], int force);

extern char otherEndAddress[128];

extern char localAddress[128];

extern std::mutex flowTable_mutex;

SHORT  ip_checksum(const void *buf, int size);

#endif