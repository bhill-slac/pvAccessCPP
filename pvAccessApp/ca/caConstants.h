/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvAccessCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <pv/pvType.h>

namespace epics {
namespace pvAccess {

    /** CA protocol magic number */
    const epics::pvData::int8 CA_MAGIC = 0xCA;
    
    /** CA protocol major revision (implemented by this library). */
    const epics::pvData::int8 CA_MAJOR_PROTOCOL_REVISION = 5;
    
    /** CA protocol minor revision (implemented by this library). */
    const epics::pvData::int8 CA_MINOR_PROTOCOL_REVISION = 0;
    
    /** Unknown CA protocol minor revision. */
    const epics::pvData::int8 CA_UNKNOWN_MINOR_PROTOCOL_REVISION = 0;
    
    /** CA version signature (e.g. 0x50). */
    const epics::pvData::int8 CA_VERSION = ((uint8_t)CA_MAJOR_PROTOCOL_REVISION<<4)|CA_MINOR_PROTOCOL_REVISION;
    
    /** CA protocol port base. */
    const epics::pvData::int32 CA_PORT_BASE = 5056;
    
    /** Default CA server port. */
    const epics::pvData::int32 CA_SERVER_PORT = CA_PORT_BASE+2*CA_MAJOR_PROTOCOL_REVISION;
    
    /** Default CA beacon port. */
    const epics::pvData::int32 CA_BROADCAST_PORT = CA_SERVER_PORT+1;
    
    /** CA protocol message header size. */
    const epics::pvData::int16 CA_MESSAGE_HEADER_SIZE = 8;
    
    /**
     * UDP maximum send message size.
     * MAX_UDP: 1500 (max of ethernet and 802.{2,3} MTU) - 20/40(IPv4/IPv6)
     * - 8(UDP) - some reserve (the MTU of Ethernet is currently independent
     * of its speed variant)
     */
    const epics::pvData::int32 MAX_UDP_SEND = 1440;
    
    /** UDP maximum receive message size. */
    const epics::pvData::int32 MAX_UDP_RECV = 0xFFFF+16;
    
    /** TCP maximum receive message size. */
    const epics::pvData::int32 MAX_TCP_RECV = 1024*16;
    
    /** Maximum number of search requests in one search message. */
    const epics::pvData::int32 MAX_SEARCH_BATCH_COUNT = 0xFFFF;
    
    /** Default priority (corresponds to POSIX SCHED_OTHER) */
    const epics::pvData::int16 CA_DEFAULT_PRIORITY = 0;
    
    /** Unreasonable channel name length. */
    const epics::pvData::uint32 UNREASONABLE_CHANNEL_NAME_LENGTH = 500;
    
    /** Invalid data type. */
    const epics::pvData::int16 INVALID_DATA_TYPE = 0xFFFF;
    
    /** Invalid IOID. */
    const epics::pvData::int32 INVALID_IOID = 0;
    
    /** All messages must be aligned to 8-bytes (64-bit). */
    const epics::pvData::int32 CA_ALIGNMENT = 8;
    
    /** Default CA provider name. */
    const epics::pvData::String PVACCESS_DEFAULT_PROVIDER = "local";
}
}

#endif /* CONSTANTS_H_ */
