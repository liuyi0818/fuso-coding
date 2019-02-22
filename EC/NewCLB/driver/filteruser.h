#ifndef __FILTERUSER_H__
#define __FILTERUSER_H__

#pragma warning(disable:4200)

#include "../include/common/basedef.h"

//
// Temp file to test filter
//

#define _NDIS_CONTROL_CODE(request,method) \
    CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, request, method, FILE_ANY_ACCESS)

#define IOCTL_FILTER_RESTART_ALL                            _NDIS_CONTROL_CODE(0, METHOD_BUFFERED)
#define IOCTL_FILTER_RESTART_ONE_INSTANCE                   _NDIS_CONTROL_CODE(1, METHOD_BUFFERED)
#define IOCTL_FILTER_ENUERATE_ALL_INSTANCES                 _NDIS_CONTROL_CODE(2, METHOD_BUFFERED)
#define IOCTL_FILTER_QUERY_ALL_STAT                         _NDIS_CONTROL_CODE(3, METHOD_BUFFERED)
#define IOCTL_FILTER_CLEAR_ALL_STAT                         _NDIS_CONTROL_CODE(4, METHOD_BUFFERED)
#define IOCTL_FILTER_SET_OID_VALUE                          _NDIS_CONTROL_CODE(5, METHOD_BUFFERED)
#define IOCTL_FILTER_QUERY_OID_VALUE                        _NDIS_CONTROL_CODE(6, METHOD_BUFFERED)
#define IOCTL_FILTER_CANCEL_REQUEST                         _NDIS_CONTROL_CODE(7, METHOD_BUFFERED)
#define IOCTL_FILTER_READ_DRIVER_CONFIG                     _NDIS_CONTROL_CODE(8, METHOD_BUFFERED)
#define IOCTL_FILTER_WRITE_DRIVER_CONFIG                    _NDIS_CONTROL_CODE(9, METHOD_BUFFERED)
#define IOCTL_FILTER_READ_ADAPTER_CONFIG                    _NDIS_CONTROL_CODE(10, METHOD_BUFFERED)
#define IOCTL_FILTER_WRITE_ADAPTER_CONFIG                   _NDIS_CONTROL_CODE(11, METHOD_BUFFERED)
#define IOCTL_FILTER_READ_INSTANCE_CONFIG                   _NDIS_CONTROL_CODE(12, METHOD_BUFFERED)
#define IOCTL_FILTER_WRITE_INSTANCE_CONFIG                  _NDIS_CONTROL_CODE(13, METHOD_BUFFERED)

#define IOCTL_FILTER_NBL_ACQUIRE_HANDLE                     _NDIS_CONTROL_CODE(20, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_ACQUIRE_PACKET                     _NDIS_CONTROL_CODE(21, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_RELEASE_PACKET                     _NDIS_CONTROL_CODE(22, METHOD_BUFFERED)

#define IOCTL_FILTER_NBL_ENABLE_PACKET_FILTERING            _NDIS_CONTROL_CODE(30, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_DISABLE_PACKET_FILTERING           _NDIS_CONTROL_CODE(31, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_FETCH_PACKET                       _NDIS_CONTROL_CODE(32, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_PROCESS_PACKET                     _NDIS_CONTROL_CODE(33, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_SEND_PACKET                        _NDIS_CONTROL_CODE(34, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_RECV_PACKET                        _NDIS_CONTROL_CODE(35, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_ADD_FETCH_RULE                     _NDIS_CONTROL_CODE(36, METHOD_BUFFERED)
#define IOCTL_FILTER_NBL_DEL_FETCH_RULE                     _NDIS_CONTROL_CODE(37, METHOD_BUFFERED)

#define IOCTL_FILTER_NDIS_OID_REQUEST                       _NDIS_CONTROL_CODE(40, METHOD_BUFFERED)

#define IOCTL_FILTER_NDIS_ADD_FILTER_OID_REQUEST            _NDIS_CONTROL_CODE(50, METHOD_BUFFERED)
#define IOCTL_FILTER_NDIS_DEL_FILTER_OID_REQUEST            _NDIS_CONTROL_CODE(51, METHOD_BUFFERED)
#define IOCTL_FILTER_NDIS_FETCH_FILTER_OID_REQUEST          _NDIS_CONTROL_CODE(52, METHOD_BUFFERED)
#define IOCTL_FILTER_NDIS_PROCESS_FILTER_OID_REQUEST        _NDIS_CONTROL_CODE(53, METHOD_BUFFERED)

#define MAX_FILTER_INSTANCE_NAME_LENGTH     256
#define MAX_FILTER_CONFIG_KEYWORD_LENGTH    256
typedef struct _FILTER_DRIVER_ALL_STAT
{
    ULONG          AttachCount;
    ULONG          DetachCount;
    ULONG          ExternalRequestFailedCount;
    ULONG          ExternalRequestSuccessCount;
    ULONG          InternalRequestFailedCount;
} FILTER_DRIVER_ALL_STAT, * PFILTER_DRIVER_ALL_STAT;


typedef struct _FILTER_SET_OID
{
    WCHAR           InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
    ULONG           InstanceNameLength;
    NDIS_OID        Oid;
    NDIS_STATUS     Status;
    UCHAR           Data[sizeof(ULONG)];

}FILTER_SET_OID, *PFILTER_SET_OID;

typedef struct _FILTER_QUERY_OID
{
    WCHAR           InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
    ULONG           InstanceNameLength;
    NDIS_OID        Oid;
    NDIS_STATUS     Status;
    UCHAR           Data[sizeof(ULONG)];

}FILTER_QUERY_OID, *PFILTER_QUERY_OID;

typedef struct _FILTER_READ_CONFIG
{
    __field_bcount_part(MAX_FILTER_INSTANCE_NAME_LENGTH,InstanceNameLength) 
    WCHAR                   InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
    ULONG                   InstanceNameLength;
    __field_bcount_part(MAX_FILTER_CONFIG_KEYWORD_LENGTH,KeywordLength) 
    WCHAR                   Keyword[MAX_FILTER_CONFIG_KEYWORD_LENGTH];
    ULONG                   KeywordLength;
    NDIS_PARAMETER_TYPE     ParameterType;
    NDIS_STATUS             Status;
    UCHAR                   Data[sizeof(ULONG)];
}FILTER_READ_CONFIG, *PFILTER_READ_CONFIG;

typedef struct _FILTER_WRITE_CONFIG
{
    __field_bcount_part(MAX_FILTER_INSTANCE_NAME_LENGTH,InstanceNameLength) 
    WCHAR                   InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
    ULONG                   InstanceNameLength;
    __field_bcount_part(MAX_FILTER_CONFIG_KEYWORD_LENGTH,KeywordLength) 
    WCHAR                   Keyword[MAX_FILTER_CONFIG_KEYWORD_LENGTH];
    ULONG                   KeywordLength;
    NDIS_PARAMETER_TYPE     ParameterType;
    NDIS_STATUS             Status;
    UCHAR                   Data[sizeof(ULONG)];
}FILTER_WRITE_CONFIG, *PFILTER_WRITE_CONFIG;

typedef struct _FILTER_NBL_ACQUIRE_HANDLE
{
    wchar_t                        m_module_name[MAX_FILTER_INSTANCE_NAME_LENGTH];
    x64_wrapper                    m_filter_handle;
} FILTER_NBL_ACQUIRE_HANDLE, *PFILTER_NBL_ACQUIRE_HANDLE;

typedef struct _FILTER_NBL_ENABLE_PACKET_FILTERING
{
    x64_wrapper                    m_filter_handle;
    unsigned long                  m_flags;
    x64_wrapper                    m_affinity;
    unsigned long                  m_packet_max_count;
    unsigned long                  m_packet_max_len;
    unsigned long                  m_prefix_len;
    unsigned long                  m_posfix_len;
    enum NBL_DESC_ACTION           m_failed_action;
    LONGLONG                       m_moderation_duetime;
} FILTER_NBL_ENABLE_PACKET_FILTERING, *PFILTER_NBL_ENABLE_PACKET_FILTERING;

typedef struct _FILTER_NBL_DISABLE_PACKET_FILTERING
{
    x64_wrapper                    m_filter_handle;
} FILTER_NBL_DISABLE_PACKET_FILTERING, *PFILTER_NBL_DISABLE_PACKET_FILTERING;

struct NBL_INFO {

    union {
        NDIS_TCP_IP_CHECKSUM_NET_BUFFER_LIST_INFO m_tcp_ip_checksum_net_buffer_list_info;
        unsigned long m_tcp_offload_bytes_transferred;
        x64_wrapper m_tcp_ip_checksum_net_buffer_list_info_x64_wrapper;
    };
    union {
        NDIS_IPSEC_OFFLOAD_V1_NET_BUFFER_LIST_INFO m_ipsec_offload_v1_net_buffer_list_info;
        NDIS_IPSEC_OFFLOAD_V2_NET_BUFFER_LIST_INFO m_ipsec_offload_v2_net_buffer_list_info;
        x64_wrapper m_ipsec_offload_vx_net_buffer_list_info_x64_wrapper;
    };
    union {
        NDIS_TCP_LARGE_SEND_OFFLOAD_NET_BUFFER_LIST_INFO m_tcp_large_send_offload_net_buffer_list_info;
        BOOLEAN m_tcp_receive_no_push;
        x64_wrapper m_tcp_large_send_offload_net_buffer_list_info_x64_wrapper;
    };
    union {
        NDIS_NET_BUFFER_LIST_8021Q_INFO m_net_buffer_list_8021q_info;
        x64_wrapper m_net_buffer_list_8021q_info_x64_wrapper;
    };
    union {
        unsigned long m_net_buffer_list_hash_value;
        x64_wrapper m_net_buffer_list_hash_value_x64_wrapper;
    };
    union {
        unsigned long m_net_buffer_list_hash_info;
        x64_wrapper m_net_buffer_list_hash_info_x64_wrapper;
    };
};

struct MAP_MEM_K;

struct NBL_DESC {

    union {
        LIST_ENTRY m_link;
        struct {
            x64_wrapper m_link_Flink_x64_wrapper;
            x64_wrapper m_link_Blink_x64_wrapper;
        } m_link_x64_wrapper;
    };
    union {
        LIST_ENTRY m_track_link;
        struct {
            x64_wrapper m_link_Flink_x64_wrapper;
            x64_wrapper m_link_Blink_x64_wrapper;
        } m_track_link_x64_wrapper;
    };
    union NBL_STATE_DESC {
        long m_state;
    } m_state_desc;
    union {
        struct MAP_MEM_K* m_map_mem_k;
        x64_wrapper m_map_mem_k_x64_wrapper;
    };
    union {
        void*       m_addr;
        x64_wrapper m_addr_x64_wrapper;
    };
    union {
        PMDL        m_partial_mdl;
        x64_wrapper m_partial_mdl_x64_wrapper;
    };
    union {
        PNET_BUFFER_LIST m_nbl;
        x64_wrapper      m_nbl_x64_wrapper;
    };    
    struct {
        enum NBL_DESC_DIRECTION m_direction;
        union {
            NDIS_HANDLE m_filter_module_context;
            x64_wrapper m_filter_module_context_x64_wrapper;
        };
        union {
            PNET_BUFFER_LIST m_nbl;
            x64_wrapper      m_nbl_x64_wrapper;
        };
        unsigned long m_len;
        NDIS_PORT_NUMBER m_port_number;
        ULONG m_flags;
    } m_original_nbl;
    struct {
        unsigned long m_mdl_byte_offset;
        unsigned long m_mdl_byte_count;
        struct NBL_INFO m_nbl_info;
        NDIS_PORT_NUMBER m_port_number;
        ULONG m_flags;
        x64_wrapper m_hw_timestamp;
        x64_wrapper m_sw_timestamp;
        enum NBL_DESC_ACTION m_action;        
        unsigned char m_data[0];
    } m_user;
};

#endif //__FILTERUSER_H__