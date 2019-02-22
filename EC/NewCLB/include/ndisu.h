#pragma once

#include <windows.h>
#include <ntddndis.h>

typedef void* NDIS_HANDLE;
typedef void* PNET_BUFFER_LIST;
typedef void* PMDL;

typedef enum _NDIS_PARAMETER_TYPE {

    NdisParameterInteger,
    NdisParameterHexInteger,
    NdisParameterString,
    NdisParameterMultiString,
    NdisParameterBinary
} NDIS_PARAMETER_TYPE, *PNDIS_PARAMETER_TYPE;

typedef struct _NDIS_TCP_IP_CHECKSUM_NET_BUFFER_LIST_INFO {

    union {
        struct {
            ULONG  IsIPv4:1;
            ULONG  IsIPv6:1;
            ULONG  TcpChecksum:1;
            ULONG  UdpChecksum:1;
            ULONG  IpHeaderChecksum:1;
            ULONG  Reserved:11;
            ULONG  TcpHeaderOffset:10;
        } Transmit;
        struct {
            ULONG  TcpChecksumFailed:1;
            ULONG  UdpChecksumFailed:1;
            ULONG  IpChecksumFailed:1;
            ULONG  TcpChecksumSucceeded:1;
            ULONG  UdpChecksumSucceeded:1;
            ULONG  IpChecksumSucceeded:1;
            ULONG  Loopback:1;
        } Receive;
        PVOID  Value;
    };
} NDIS_TCP_IP_CHECKSUM_NET_BUFFER_LIST_INFO, *PNDIS_TCP_IP_CHECKSUM_NET_BUFFER_LIST_INFO;

typedef struct _NDIS_IPSEC_OFFLOAD_V1_NET_BUFFER_LIST_INFO {

    union {
        struct {
            NDIS_HANDLE  OffloadHandle;
        } Transmit;
        struct {
            USHORT  SaDeleteReq:1;
            USHORT  CryptoDone:1;
            USHORT  NextCryptoDone:1;
            USHORT  Pad:13;
            USHORT  CryptoStatus;
        } Receive;
    };
} NDIS_IPSEC_OFFLOAD_V1_NET_BUFFER_LIST_INFO, *PNDIS_IPSEC_OFFLOAD_V1_NET_BUFFER_LIST_INFO;

typedef struct _NDIS_IPSEC_OFFLOAD_V2_NET_BUFFER_LIST_INFO {
  
    union {
        struct {
            PVOID   OffloadHandle;
        } Transmit;
        struct {
            ULONG  SaDeleteReq:1;
            ULONG  CryptoDone:1;
            ULONG  NextCryptoDone:1;
            ULONG  Reserved:13;
            ULONG  CryptoStatus:16;
        } Receive;
    };
} NDIS_IPSEC_OFFLOAD_V2_NET_BUFFER_LIST_INFO, *PNDIS_IPSEC_OFFLOAD_V2_NET_BUFFER_LIST_INFO;

typedef struct _NDIS_TCP_LARGE_SEND_OFFLOAD_NET_BUFFER_LIST_INFO {
  
    union {
        struct {
            ULONG  Unused:30;
            ULONG  Type:1;
            ULONG  Reserved2:1;
        } Transmit;
        struct {
            ULONG  MSS:20;
            ULONG  TcpHeaderOffset:10;
            ULONG  Type:1;
            ULONG  Reserved2:1;
        } LsoV1Transmit;
        struct {
            ULONG  TcpPayload:30;
            ULONG  Type:1;
            ULONG  Reserved2:1;
        } LsoV1TransmitComplete;
        struct {
            ULONG  MSS:20;
            ULONG  TcpHeaderOffset:10;
            ULONG  Type:1;
            ULONG  IPVersion:1;
        } LsoV2Transmit;
        struct {
            ULONG  Reserved:30;
            ULONG  Type:1;
            ULONG  Reserved2:1;
        } LsoV2TransmitComplete;
        PVOID  Value;
    };
} NDIS_TCP_LARGE_SEND_OFFLOAD_NET_BUFFER_LIST_INFO, *PNDIS_TCP_LARGE_SEND_OFFLOAD_NET_BUFFER_LIST_INFO;

typedef struct _NDIS_NET_BUFFER_LIST_8021Q_INFO {
    
    union {
        struct {
            UINT32  UserPriority:3;
            UINT32  CanonicalFormatId:1;
            UINT32  VlanId:12;
            UINT32  Reserved:16;
        } TagHeader;
        struct {
            UINT32  UserPriority:3;
            UINT32  CanonicalFormatId:1;
            UINT32  VlanId:12;
            UINT32  WMMInfo:4;
            UINT32  Reserved:12;
        } WLanTagHeader;
        PVOID  Value;
    };
} NDIS_NET_BUFFER_LIST_8021Q_INFO, *PNDIS_NET_BUFFER_LIST_8021Q_INFO;