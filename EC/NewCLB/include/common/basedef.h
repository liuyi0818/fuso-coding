#pragma once

#pragma warning(disable:4200)

#define FILTER_FLAG_CAPTURE_SEND_PACKET    (1 << 0)
#define FILTER_FLAG_CAPTURE_RECV_PACKET    (1 << 1)

enum NBL_DESC_TYPE {

    TypeNone = 0,
    Filtering,
    UserAcquired,
};

enum NBL_DESC_DIRECTION {

    DirectionNone = 0,
    Send,
    Recv,
};

enum NBL_DESC_ACTION {

    ActionNone = 0,
    Forward,
    Drop,
};

enum FETCH_RULE_TYPE {

    RuleTypeNone = 0,
    Capture,
    Passthr,
};

typedef union _x64_wrapper {

    HANDLE                  m_handle;
    void*                   m_ptr;
    __int64                 m_value;
} x64_wrapper, *px64_wrapper;

typedef struct _FILTER_NBL_PACKET
{
    x64_wrapper                    m_handle;
    x64_wrapper                    m_addr;

#ifdef __cplusplus

    NBL_DESC_DIRECTION 
    GetDirection();

    unsigned char* 
    GetBuffer();

    unsigned long 
    GetLength();

    unsigned long 
    GetPrefixLength();

    struct NBL_INFO* 
    GetInfo();

    NBL_DESC_ACTION 
    GetAction();

    x64_wrapper
    GetHWTimestamp();

    x64_wrapper
    GetSWTimestamp();

    void 
    SetLength(unsigned long length);

    void 
    SetPrefixLength(unsigned long length);

    void
    SetAction(NBL_DESC_ACTION action);

#endif

} FILTER_NBL_PACKET, *PFILTER_NBL_PACKET;

typedef struct _FILTER_NBL_ACQUIRE_PACKET
{
    x64_wrapper                 m_filter_handle;
    unsigned long               m_size;
    FILTER_NBL_PACKET           m_packet[1];
} FILTER_NBL_ACQUIRE_PACKET, *PFILTER_NBL_ACQUIRE_PACKET;

typedef struct _FILTER_NBL_RELEASE_PACKET
{
    x64_wrapper                 m_filter_handle;
    FILTER_NBL_PACKET           m_packet[1];
} FILTER_NBL_RELEASE_PACKET, *PFILTER_NBL_RELEASE_PACKET;

typedef struct _FILTER_NBL_FETCH_PACKET
{
    x64_wrapper                 m_filter_handle;
    unsigned long               m_count;
    enum NBL_DESC_DIRECTION     m_direction;
    unsigned long               m_processor_id;
    FILTER_NBL_PACKET           m_packet[0];
} FILTER_NBL_FETCH_PACKET, *PFILTER_NBL_FETCH_PACKET;

typedef struct _FILTER_NBL_PROCESS_PACKET
{
    x64_wrapper                 m_filter_handle;
    unsigned long               m_count;
    enum NBL_DESC_DIRECTION     m_direction;
    unsigned long               m_processor_id;        
    FILTER_NBL_PACKET           m_packet[0];
} FILTER_NBL_PROCESS_PACKET, *PFILTER_NBL_PROCESS_PACKET;

typedef struct _FILTER_NBL_SEND_PACKET
{
    x64_wrapper                 m_filter_handle;
    unsigned long               m_count;
    FILTER_NBL_PACKET           m_packet[0];
} FILTER_NBL_SEND_PACKET, *PFILTER_NBL_SEND_PACKET;

typedef struct _FILTER_NBL_PACKET_RECV
{
    x64_wrapper                 m_filter_handle;
    unsigned long               m_count;
    FILTER_NBL_PACKET           m_packet[0];
} FILTER_NBL_RECV_PACKET, *PFILTER_NBL_RECV_PACKET;

typedef struct _FETCH_RULE {

    unsigned long m_offset;
    unsigned char m_value;
} FETCH_RULE, PFETCH_RULE;

typedef struct _FILTER_NBL_ADD_FETCH_RULE
{
    x64_wrapper                 m_filter_handle;
    x64_wrapper                 m_handle;
    enum FETCH_RULE_TYPE        m_fetch_rule_type;
    unsigned long               m_count;
    FETCH_RULE                  m_fetch_rule[0];
} FILTER_NBL_ADD_FETCH_RULE, *PFILTER_NBL_ADD_FETCH_RULE;

typedef struct _FILTER_NBL_DEL_FETCH_RULE
{
    x64_wrapper                 m_filter_handle;
    x64_wrapper                 m_handle;
} FILTER_NBL_DEL_FETCH_RULE, *PFILTER_NBL_DEL_FETCH_RULE;

typedef struct _FILTER_NDIS_OID_REQUEST
{
    x64_wrapper                 m_filter_handle;
    NDIS_REQUEST_TYPE           m_request_type;
    NDIS_OID                    m_oid;
    unsigned long               m_method_id;
    unsigned long               m_redirect;
    NDIS_STATUS                 m_status;
    unsigned long               m_byte_written;
    unsigned long               m_byte_read;
    unsigned long               m_byte_needed;    
    char                        m_data[0];
} FILTER_NDIS_OID_REQUEST, *PFILTER_NDIS_OID_REQUEST;

typedef struct _FILTER_NDIS_ADD_FILTER_OID_REQUEST 
{
    x64_wrapper                 m_filter_handle;
    NDIS_OID                    m_oid;
} FILTER_NDIS_ADD_FILTER_OID_REQUEST, *PFILTER_NDIS_ADD_FILTER_OID_REQUEST;

typedef struct _FILTER_NDIS_DEL_FILTER_OID_REQUEST 
{
    x64_wrapper                 m_filter_handle;
    NDIS_OID                    m_oid;
} FILTER_NDIS_DEL_FILTER_OID_REQUEST, *PFILTER_NDIS_DEL_FILTER_OID_REQUEST;

typedef struct _FILTER_NDIS_FETCH_FILTER_OID_REQUEST 
{
    x64_wrapper                 m_filter_handle;
    x64_wrapper                 m_handle;
    NDIS_REQUEST_TYPE           m_request_type;
    NDIS_OID                    m_oid;
    unsigned long               m_method_id;
    NDIS_STATUS                 m_status;
    unsigned long               m_input_len;
    unsigned long               m_output_len;
    char                        m_data[0];
} FILTER_NDIS_FETCH_FILTER_OID_REQUEST, *PFILTER_NDIS_FETCH_FILTER_OID_REQUEST;

typedef struct _FILTER_NDIS_PROCESS_FILTER_OID_REQUEST
{
    x64_wrapper                 m_filter_handle;
    x64_wrapper                 m_handle;
    NDIS_STATUS                 m_status;
    unsigned long               m_byte_written;
    unsigned long               m_byte_read;
    unsigned long               m_byte_needed;
    char                        m_data[0];
} FILTER_NDIS_PROCESS_FILTER_OID_REQUEST, *PFILTER_NDIS_PROCESS_FILTER_OID_REQUEST;
