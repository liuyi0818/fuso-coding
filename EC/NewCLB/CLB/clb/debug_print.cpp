#include "clb_main.h"

int gettimeofday(struct timeval *tp, void *tzp);

void PrintIPPacket(FILTER_NBL_PACKET pkt, char SendorReceive[128], int force)
{
	if (!IF_PRINT_DEBUG && force == 0)
	{
		return;
	}
	
	IP_HEADER out_ip_header;
	memcpy(&out_ip_header,pkt.GetBuffer() + pkt.GetPrefixLength() + 14,sizeof(IP_HEADER));

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
    outSrc.S_un.S_addr = out_ip_header.ip_srcaddr;
    in_addr outDst;
    outDst.S_un.S_addr = out_ip_header.ip_destaddr;
	char strOutSrc[256];
	sprintf_s(strOutSrc,"%s", inet_ntoa(outSrc));
	char strOutDst[256];
	sprintf_s(strOutDst,"%s", inet_ntoa(outDst));
	
	printf("%s-----[%d]: %s packet IP[%s-%s], out_ip_ID:%u\n",
		buf,
		GetCurrentProcessorNumber(),
		SendorReceive,
		strOutSrc,
		strOutDst,
		ntohs(out_ip_header.ip_id)
		);
}
