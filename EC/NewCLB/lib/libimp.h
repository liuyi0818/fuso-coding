#pragma once

#include <windows.h>
#include <ntddndis.h>
#include "../include/lib.h"
#include "../include/ndisu.h"
#include "../driver/filteruser.h"

#define state_initialized    (1 << 1)
#define state_enabled        (1 << 2)

class PacketFilterImp : public PacketFilter {

public:
    PacketFilterImp();
    virtual ~PacketFilterImp();

    virtual HANDLE Handle();
    virtual bool Initialize(wchar_t* adapter_name);
    virtual bool InitializeEx(wchar_t* file_name, wchar_t* adapter_name);
    virtual void Uninitialize();
    virtual bool AcquirePacket(PFILTER_NBL_ACQUIRE_PACKET acquire_packet);
    virtual void ReleasePacket(PFILTER_NBL_RELEASE_PACKET release_packet);
    virtual bool EnablePacketFiltering(unsigned long flags, KAFFINITY affinity, unsigned long packet_max_count, unsigned long packet_max_len, unsigned long prefix_len, unsigned long posfix_len, NBL_DESC_ACTION failed_action, LONGLONG moderation_duetime);
    virtual void DisablePacketFiltering();
    virtual DWORD FetchPacket(PFILTER_NBL_FETCH_PACKET fetch_packet, unsigned long size, LPOVERLAPPED overlapped);
    virtual bool ProcessPacket(PFILTER_NBL_PROCESS_PACKET process_packet, unsigned long size);
    virtual DWORD SendPacket(PFILTER_NBL_SEND_PACKET send_packet, unsigned long size, LPOVERLAPPED overlapped);
    virtual DWORD RecvPacket(PFILTER_NBL_RECV_PACKET recv_packet, unsigned long size, LPOVERLAPPED overlapped);
    virtual bool AddFetchRule(PFILTER_NBL_ADD_FETCH_RULE add_fetch_rule, unsigned long size);
    virtual bool DelFetchRule(PFILTER_NBL_DEL_FETCH_RULE del_fetch_rule);
    virtual DWORD OidRequest(PFILTER_NDIS_OID_REQUEST oid_request, unsigned long size, LPOVERLAPPED overlapped);
    virtual bool AddFilterOidRequest(PFILTER_NDIS_ADD_FILTER_OID_REQUEST add_filter_oid_request);
    virtual bool DelFilterOidRequest(PFILTER_NDIS_DEL_FILTER_OID_REQUEST del_filter_oid_request);
    virtual DWORD FetchFilterOidRequest(PFILTER_NDIS_FETCH_FILTER_OID_REQUEST fetch_filter_oid_request, unsigned long size, LPOVERLAPPED overlapped);
    virtual DWORD ProcessFilterOidRequest(PFILTER_NDIS_PROCESS_FILTER_OID_REQUEST process_filter_oid_request, unsigned long size);

protected:
    unsigned long m_state;
    HANDLE m_file;
    x64_wrapper m_filter;
};