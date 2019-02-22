#pragma once

#include <windows.h>
#include <ntddndis.h>
#include "common/basedef.h"

class PacketFilter {

public:
    static PacketFilter* NewPacketFilter();
    static void DelPacketFilter(PacketFilter* filter);

    virtual HANDLE Handle() = 0;
    virtual bool Initialize(wchar_t* adapter_name) = 0;
    virtual bool InitializeEx(wchar_t* file_name, wchar_t* adapter_name) = 0;
    virtual void Uninitialize() = 0;
    virtual bool AcquirePacket(PFILTER_NBL_ACQUIRE_PACKET acquire_packet) = 0;
    virtual void ReleasePacket(PFILTER_NBL_RELEASE_PACKET release_packet) = 0;
    virtual bool EnablePacketFiltering(unsigned long flags, KAFFINITY affinity, unsigned long packet_max_count, unsigned long packet_max_len, unsigned long prefix_len, unsigned long posfix_len, NBL_DESC_ACTION failed_action, LONGLONG moderation_duetime) = 0;
    virtual void DisablePacketFiltering() = 0;
    virtual DWORD FetchPacket(PFILTER_NBL_FETCH_PACKET fetch_packet, unsigned long size, LPOVERLAPPED overlapped) = 0;
    virtual bool ProcessPacket(PFILTER_NBL_PROCESS_PACKET process_packet, unsigned long size) = 0;
    virtual DWORD SendPacket(PFILTER_NBL_SEND_PACKET send_packet, unsigned long size, LPOVERLAPPED overlapped) = 0;
    virtual DWORD RecvPacket(PFILTER_NBL_RECV_PACKET recv_packet, unsigned long size, LPOVERLAPPED overlapped) = 0;
    virtual bool AddFetchRule(PFILTER_NBL_ADD_FETCH_RULE add_fetch_rule, unsigned long size) = 0;
    virtual bool DelFetchRule(PFILTER_NBL_DEL_FETCH_RULE del_fetch_rule) = 0;
    virtual DWORD OidRequest(PFILTER_NDIS_OID_REQUEST oid_request, unsigned long size, LPOVERLAPPED overlapped) = 0;
    virtual bool AddFilterOidRequest(PFILTER_NDIS_ADD_FILTER_OID_REQUEST add_filter_oid_request) = 0;
    virtual bool DelFilterOidRequest(PFILTER_NDIS_DEL_FILTER_OID_REQUEST del_filter_oid_request) = 0;
    virtual DWORD FetchFilterOidRequest(PFILTER_NDIS_FETCH_FILTER_OID_REQUEST fetch_filter_oid_request, unsigned long size, LPOVERLAPPED overlapped) = 0;
    virtual DWORD ProcessFilterOidRequest(PFILTER_NDIS_PROCESS_FILTER_OID_REQUEST process_filter_oid_request, unsigned long size) = 0;
};