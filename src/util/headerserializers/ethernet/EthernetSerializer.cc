//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include <ethernet/EthernetSerializer.h>

namespace INETFw // load headers into a namespace, to avoid conflicts with platform definitions of the same stuff
{
#include "headers/bsdint.h"
#include "headers/in.h"
#include "headers/in_systm.h"
#include "headers/ethernet.h"
};

#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__CYGWIN__) && !defined(_WIN64)
#include <netinet/in.h>  // htonl, ntohl, ...
#endif

#ifdef WITH_IPv4
#include "IPv4Serializer.h"
#endif

#ifdef WITH_IPv6
#include "IPv6Serializer.h"
#endif

#include "ARPSerializer.h"

using namespace INETFw;

int EthernetSerializer::serialize(const EthernetIIFrame *pkt, unsigned char *buf, unsigned int bufsize)
{
    int packetLength = ETHER_HDR_LEN;

    struct ether_header *etherhdr = (struct ether_header *) (buf);

    pkt->getDest().getAddressBytes(etherhdr->ether_dhost);
    pkt->getSrc().getAddressBytes(etherhdr->ether_shost);
    etherhdr->ether_type = htons(pkt->getEtherType());

    cMessage *encapPacket = pkt->getEncapsulatedPacket();

    switch (pkt->getEtherType())
    {
#ifdef WITH_IPv4
        case ETHERTYPE_IP:
            packetLength += IPv4Serializer().serialize(check_and_cast<IPv4Datagram *>(encapPacket),
                                                               buf+ETHER_HDR_LEN, bufsize-ETHER_HDR_LEN, true);
            break;
#endif

#ifdef WITH_IPv6
        case ETHERTYPE_IPV6:
            packetLength += IPv6Serializer().serialize(check_and_cast<IPv6Datagram *>(encapPacket),
                                                               buf+ETHER_HDR_LEN, bufsize-ETHER_HDR_LEN);
            break;
#endif

#ifdef WITH_IPv4
        case ETHERTYPE_ARP:
            packetLength += ARPSerializer().serialize(check_and_cast<ARPPacket *>(encapPacket),
                                                               buf+ETHER_HDR_LEN, bufsize-ETHER_HDR_LEN);
            break;
#endif

        default:
            throw cRuntimeError("EthernetSerializer: cannot serialize protocol %x", pkt->getEtherType());
    }

    return packetLength;
}

void EthernetSerializer::parse(const unsigned char *buf, unsigned int bufsize, cPacket **pkt)
{
    struct ether_header *etherhdr = (struct ether_header*) buf;
    *pkt = new EthernetIIFrame;
    EthernetIIFrame *etherPacket = (EthernetIIFrame*)*pkt;

    MACAddress temp;
    temp.setAddressBytes(etherhdr->ether_dhost);
    etherPacket->setDest(temp);
    temp.setAddressBytes(etherhdr->ether_shost);
    etherPacket->setSrc(temp);
    etherPacket->setEtherType(ntohs(etherhdr->ether_type));

    cPacket *encapPacket = NULL;

    switch (etherPacket->getEtherType())
    {
#ifdef WITH_IPv4
        case ETHERTYPE_IP:
            encapPacket = new IPv4Datagram("ipv4-from-wire");
            IPv4Serializer().parse(buf+ETHER_HDR_LEN, bufsize-ETHER_HDR_LEN, (IPv4Datagram *)encapPacket);
            break;
#endif

#ifdef WITH_IPv6
        case ETHERTYPE_IPV6:
            encapPacket = new IPv6Datagram("ipv6-from-wire");
            IPv6Serializer().parse(buf+ETHER_HDR_LEN, bufsize-ETHER_HDR_LEN, (IPv6Datagram *)encapPacket);
            break;
#endif

        case ETHERTYPE_ARP:
            encapPacket = new ARPPacket("arp-from-wire");
            ARPSerializer().parse(buf+ETHER_HDR_LEN, bufsize-ETHER_HDR_LEN, (ARPPacket *)encapPacket);
            break;

        default:
            throw cRuntimeError("EthernetSerializer: cannot parse protocol %x", etherPacket->getEtherType());
    }
    ASSERT(encapPacket);
    etherPacket->encapsulate(encapPacket);
    etherPacket->setName(encapPacket->getName());
}


