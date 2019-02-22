#include "clb_main.h"

SHORT  ip_checksum(const void *buf, int size)
{
	unsigned short buffer[10];
	memcpy(buffer,buf,10*sizeof(unsigned short));
	unsigned long cksum = 0;
	for(int i=0;i<10;i++)
	{
		cksum=cksum+ntohs(buffer[i]);
	}
	 while ((cksum >> 16) > 0)
	 {
	  cksum = (cksum >>16 ) + (cksum & 0xffff);
	 }
	return (USHORT)(~cksum);
}
