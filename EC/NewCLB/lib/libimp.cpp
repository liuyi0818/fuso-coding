#include "stdafx.h"
#include <windows.h>
#include <devioctl.h>
#include <assert.h>
#include <stdio.h>
#include "libimp.h"

PacketFilter* PacketFilter::NewPacketFilter() {

    return new PacketFilterImp();
}

void PacketFilter::DelPacketFilter(PacketFilter* filter) {

    PacketFilterImp* filterimp;
    filterimp = (PacketFilterImp*)filter;
    if (filterimp) {
        filterimp->Uninitialize();
        delete filter;
    }
}

PacketFilterImp:: PacketFilterImp() : 
    m_state(0),
    m_file(INVALID_HANDLE_VALUE) {
    m_filter.m_value = NULL;
}

PacketFilterImp::~PacketFilterImp() {

    Uninitialize();
}

HANDLE PacketFilterImp::Handle() {

    return m_file;
}

bool PacketFilterImp::Initialize(wchar_t* adapter_name) {

    Uninitialize();
    m_file = CreateFile(
        L"\\\\?\\NDISLWF",
        GENERIC_READ|GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
        NULL);
    if (m_file != INVALID_HANDLE_VALUE) {
        FILTER_NBL_ACQUIRE_HANDLE filter_nbl_acquire_handle = { 0 };
        if (adapter_name)
            wcsncpy_s(filter_nbl_acquire_handle.m_module_name, MAX_FILTER_INSTANCE_NAME_LENGTH, adapter_name, _TRUNCATE);
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        if (DeviceIoControl(m_file, 
            IOCTL_FILTER_NBL_ACQUIRE_HANDLE,
            &filter_nbl_acquire_handle,
            sizeof(FILTER_NBL_ACQUIRE_HANDLE),
            &filter_nbl_acquire_handle,
            sizeof(FILTER_NBL_ACQUIRE_HANDLE),
            &bytereturn,
            &overlapped)) {
            m_filter.m_value = filter_nbl_acquire_handle.m_filter_handle.m_value;
            m_state |= state_initialized;
            return true;
        }
    }
    Uninitialize();
    return false;
}

bool PacketFilterImp::InitializeEx(wchar_t* file_name, wchar_t* adapter_name) {

    Uninitialize();
    m_file = CreateFile(
        file_name,
        GENERIC_READ|GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
        NULL);
    if (m_file != INVALID_HANDLE_VALUE) {
        FILTER_NBL_ACQUIRE_HANDLE filter_nbl_acquire_handle = { 0 };
        if (adapter_name)
            wcsncpy_s(filter_nbl_acquire_handle.m_module_name, MAX_FILTER_INSTANCE_NAME_LENGTH, adapter_name, _TRUNCATE);
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        if (DeviceIoControl(m_file, 
            IOCTL_FILTER_NBL_ACQUIRE_HANDLE,
            &filter_nbl_acquire_handle,
            sizeof(FILTER_NBL_ACQUIRE_HANDLE),
            &filter_nbl_acquire_handle,
            sizeof(FILTER_NBL_ACQUIRE_HANDLE),
            &bytereturn,
            &overlapped)) {
            m_filter.m_value = filter_nbl_acquire_handle.m_filter_handle.m_value;
            m_state |= state_initialized;
            return true;
        }
    }
    Uninitialize();
    return false;
}

void PacketFilterImp::Uninitialize() {

    m_state &= ~state_initialized;

    DisablePacketFiltering();
    if (m_file != INVALID_HANDLE_VALUE) {
        CloseHandle(m_file);
        m_file  = INVALID_HANDLE_VALUE;
    }
    m_filter.m_value = NULL;
}

bool PacketFilterImp::AcquirePacket(PFILTER_NBL_ACQUIRE_PACKET acquire_packet) {

    if (m_state & state_initialized) {
        acquire_packet->m_filter_handle.m_value = m_filter.m_value;
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        if (DeviceIoControl(m_file,
            IOCTL_FILTER_NBL_ACQUIRE_PACKET,
            acquire_packet,
            sizeof(FILTER_NBL_ACQUIRE_PACKET),
            acquire_packet,
            sizeof(FILTER_NBL_ACQUIRE_PACKET),
            &bytereturn,
            &overlapped))
            return true;
    }
    return false;
}

void PacketFilterImp::ReleasePacket(PFILTER_NBL_RELEASE_PACKET release_packet) {

    if (m_state & state_initialized) {
        release_packet->m_filter_handle.m_value = m_filter.m_value;
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        DeviceIoControl(m_file,
            IOCTL_FILTER_NBL_RELEASE_PACKET,
            release_packet,
            sizeof(FILTER_NBL_RELEASE_PACKET),
            NULL,
            0,
            &bytereturn,
            &overlapped);
    }
}

bool PacketFilterImp::EnablePacketFiltering(unsigned long flags, KAFFINITY affinity, unsigned long packet_max_count, unsigned long packet_max_len, unsigned long prefix_len, unsigned long posfix_len, NBL_DESC_ACTION failed_action, LONGLONG moderation_duetime) {

    DisablePacketFiltering();
    if (m_state & state_initialized) {
        FILTER_NBL_ENABLE_PACKET_FILTERING enable_packet_filtering = { 0 };
        enable_packet_filtering.m_filter_handle.m_value = m_filter.m_value;
        enable_packet_filtering.m_flags = flags;
        enable_packet_filtering.m_affinity.m_value = affinity;
        enable_packet_filtering.m_packet_max_count = packet_max_count;
        enable_packet_filtering.m_packet_max_len = packet_max_len;
        enable_packet_filtering.m_prefix_len = prefix_len;
        enable_packet_filtering.m_posfix_len = posfix_len;
        enable_packet_filtering.m_failed_action = failed_action;
        enable_packet_filtering.m_moderation_duetime = moderation_duetime;
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        if (DeviceIoControl(m_file, 
            IOCTL_FILTER_NBL_ENABLE_PACKET_FILTERING,
            &enable_packet_filtering, 
            sizeof(FILTER_NBL_ENABLE_PACKET_FILTERING),
            &enable_packet_filtering, 
            sizeof(FILTER_NBL_ENABLE_PACKET_FILTERING), 
            &bytereturn,
            &overlapped)) {
            m_state |= state_enabled;
            return true;
        }
    }
    return false;
}

void PacketFilterImp::DisablePacketFiltering() {

    m_state &= ~state_enabled;

    if (m_state & state_initialized) {        
        FILTER_NBL_DISABLE_PACKET_FILTERING disable_packet_filtering = { 0 };
        disable_packet_filtering.m_filter_handle.m_value = m_filter.m_value;
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        DeviceIoControl(m_file,
            IOCTL_FILTER_NBL_DISABLE_PACKET_FILTERING,
            &disable_packet_filtering, 
            sizeof(FILTER_NBL_DISABLE_PACKET_FILTERING),
            NULL, 
            0,
            &bytereturn, 
            &overlapped);
    }
}

DWORD PacketFilterImp::FetchPacket(PFILTER_NBL_FETCH_PACKET fetch_packet, unsigned long size, LPOVERLAPPED overlapped) {

    if (m_state & state_enabled) {
        if (overlapped) {
            fetch_packet->m_filter_handle.m_value = m_filter.m_value;
            unsigned long bytereturn;
            if (DeviceIoControl(m_file,
                IOCTL_FILTER_NBL_FETCH_PACKET,
                fetch_packet,
                size,
                fetch_packet,
                size,
                &bytereturn,
                overlapped))
                return ERROR_SUCCESS;
            else
                return GetLastError();
        }
    }
    return ERROR_INVALID_ACCESS;
}

bool PacketFilterImp::ProcessPacket(PFILTER_NBL_PROCESS_PACKET process_packet, unsigned long size) {

    if (m_state & state_enabled) {
        process_packet->m_filter_handle.m_value = m_filter.m_value;
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        if (DeviceIoControl(m_file,
            IOCTL_FILTER_NBL_PROCESS_PACKET,
            process_packet,
            size,
            NULL,
            0,
            &bytereturn,
            &overlapped))
            return true;
    }
    return false;
}

DWORD PacketFilterImp::SendPacket(PFILTER_NBL_SEND_PACKET send_packet, unsigned long size, LPOVERLAPPED overlapped) {

    if (m_state & state_initialized) {
        if (overlapped) {
            send_packet->m_filter_handle.m_value = m_filter.m_value;
            unsigned long bytereturn;
            if (DeviceIoControl(m_file,
                IOCTL_FILTER_NBL_SEND_PACKET,
                send_packet,
                size,
                NULL,
                0,
                &bytereturn,
                overlapped))
                return ERROR_SUCCESS;
            else 
                return GetLastError();
        }
    }
    return ERROR_INVALID_ACCESS;
}

DWORD PacketFilterImp::RecvPacket(PFILTER_NBL_RECV_PACKET recv_packet, unsigned long size, LPOVERLAPPED overlapped) {

    if (m_state & state_initialized) {
        if (overlapped) {
            recv_packet->m_filter_handle.m_value = m_filter.m_value;
            unsigned long bytereturn;
            if (DeviceIoControl(m_file,
                IOCTL_FILTER_NBL_RECV_PACKET,
                recv_packet,
                size,
                NULL,
                0,
                &bytereturn,
                overlapped))
                return ERROR_SUCCESS;
            else 
                return GetLastError();
        }
    }
    return ERROR_INVALID_ACCESS;
}

bool PacketFilterImp::AddFetchRule(PFILTER_NBL_ADD_FETCH_RULE add_fetch_rule, unsigned long size) {

    if (m_state & state_initialized) {
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        add_fetch_rule->m_filter_handle.m_value = m_filter.m_value;
        if (DeviceIoControl(m_file,
            IOCTL_FILTER_NBL_ADD_FETCH_RULE,
            add_fetch_rule,
            size,
            add_fetch_rule,
            size,
            &bytereturn,
            &overlapped))
            return true;
    }
    return false;
}

bool PacketFilterImp::DelFetchRule(PFILTER_NBL_DEL_FETCH_RULE del_fetch_rule) {

    if (m_state & state_initialized) {
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        del_fetch_rule->m_filter_handle.m_value = m_filter.m_value;
        if (DeviceIoControl(m_file,
            IOCTL_FILTER_NBL_DEL_FETCH_RULE,
            del_fetch_rule,
            sizeof(FILTER_NBL_DEL_FETCH_RULE),
            NULL,
            0,
            &bytereturn,
            &overlapped))
            return true;
    }
    return false;
}

DWORD PacketFilterImp::OidRequest(PFILTER_NDIS_OID_REQUEST oid_request, unsigned long size, LPOVERLAPPED overlapped) {

    if (m_state & state_initialized) {
        if (overlapped) {
            oid_request->m_filter_handle.m_value = m_filter.m_value;
            unsigned long bytereturn;
            if (DeviceIoControl(m_file,
                IOCTL_FILTER_NDIS_OID_REQUEST,
                oid_request,
                size,
                oid_request,
                size,
                &bytereturn,
                overlapped))
                return ERROR_SUCCESS;
            else 
                return GetLastError();
        }
    }
    return ERROR_INVALID_ACCESS;
}

bool PacketFilterImp::AddFilterOidRequest(PFILTER_NDIS_ADD_FILTER_OID_REQUEST add_filter_oid_request) {

    if (m_state & state_initialized) {
        add_filter_oid_request->m_filter_handle.m_value = m_filter.m_value;
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        if (DeviceIoControl(m_file,
            IOCTL_FILTER_NDIS_ADD_FILTER_OID_REQUEST,
            add_filter_oid_request,
            sizeof(FILTER_NDIS_ADD_FILTER_OID_REQUEST),
            NULL,
            0,
            &bytereturn,
            &overlapped))
            return true;
    }
    return false;
}

bool PacketFilterImp::DelFilterOidRequest(PFILTER_NDIS_DEL_FILTER_OID_REQUEST del_filter_oid_request) {

    if (m_state & state_initialized) {
        del_filter_oid_request->m_filter_handle.m_value = m_filter.m_value;
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        if (DeviceIoControl(m_file,
            IOCTL_FILTER_NDIS_ADD_FILTER_OID_REQUEST,
            del_filter_oid_request,
            sizeof(FILTER_NDIS_DEL_FILTER_OID_REQUEST),
            NULL,
            0,
            &bytereturn,
            &overlapped))
            return true;
    }
    return false;
}

DWORD PacketFilterImp::FetchFilterOidRequest(PFILTER_NDIS_FETCH_FILTER_OID_REQUEST fetch_filter_oid_request, unsigned long size, LPOVERLAPPED overlapped) {

    if (m_state & state_initialized) {
        if (overlapped) {
            fetch_filter_oid_request->m_filter_handle.m_value = m_filter.m_value;
            unsigned long bytereturn;
            if (DeviceIoControl(m_file,
                IOCTL_FILTER_NDIS_FETCH_FILTER_OID_REQUEST,
                fetch_filter_oid_request,
                size,
                fetch_filter_oid_request,
                size,
                &bytereturn,
                overlapped))
                return ERROR_SUCCESS;
            else 
                return GetLastError();
        }
    }
    return ERROR_INVALID_ACCESS;
}

DWORD PacketFilterImp::ProcessFilterOidRequest(PFILTER_NDIS_PROCESS_FILTER_OID_REQUEST process_filter_oid_request, unsigned long size) {

    if (m_state & state_initialized) {
        process_filter_oid_request->m_filter_handle.m_value = m_filter.m_value;
        unsigned long bytereturn;
        OVERLAPPED overlapped = { 0 };
        if (DeviceIoControl(m_file,
            IOCTL_FILTER_NDIS_PROCESS_FILTER_OID_REQUEST,
            process_filter_oid_request,
            size,
            NULL,
            0,
            &bytereturn,
            &overlapped))
            return ERROR_SUCCESS;
        else 
            return GetLastError();
    }
    return ERROR_INVALID_ACCESS;
}

NBL_DESC_DIRECTION
FILTER_NBL_PACKET::GetDirection() {

    return ((NBL_DESC*)this->m_addr.m_ptr)->m_original_nbl.m_direction;
}

unsigned char* 
FILTER_NBL_PACKET::GetBuffer() {

    return ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_data;
}

unsigned long 
FILTER_NBL_PACKET::GetLength() {

    return ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_mdl_byte_count;
}

unsigned long 
FILTER_NBL_PACKET::GetPrefixLength() {

    return ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_mdl_byte_offset;
}

NBL_DESC_ACTION 
FILTER_NBL_PACKET::GetAction() {

    return ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_action;
}

struct NBL_INFO* 
FILTER_NBL_PACKET::GetInfo() {

    return &((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_nbl_info;
}

x64_wrapper
FILTER_NBL_PACKET::GetHWTimestamp() {

    return ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_hw_timestamp;
}

x64_wrapper
FILTER_NBL_PACKET::GetSWTimestamp() {

    return ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_sw_timestamp;
}

void 
FILTER_NBL_PACKET::SetLength(
    unsigned long length) {

    ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_mdl_byte_count = length;
}

void 
FILTER_NBL_PACKET::SetPrefixLength(
    unsigned long length) {

    ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_mdl_byte_offset = length;
}

void 
FILTER_NBL_PACKET::SetAction(
    NBL_DESC_ACTION action) {

    ((NBL_DESC*)this->m_addr.m_ptr)->m_user.m_action = action;
}
