#include <iostream>
#include <cstdint>
#include "../../Dependencies/Shared/Packet/PacketCommon.h"
#include "../../Dependencies/Shared/Packet/PacketEvent.h"

int main() {
    std::cout << "sizeof(hb::net::PacketHeader) = " << sizeof(hb::net::PacketHeader) << std::endl;
    std::cout << "sizeof(hb::net::PacketEventNearTypeBShort) = " << sizeof(hb::net::PacketEventNearTypeBShort) << std::endl;
    std::cout << "sizeof(hb::net::PacketEventNearTypeBDword) = " << sizeof(hb::net::PacketEventNearTypeBDword) << std::endl;

    std::cout << "\nExpected sizes:" << std::endl;
    std::cout << "PacketHeader should be: 6 bytes (uint32 + uint16)" << std::endl;
    std::cout << "PacketEventNearTypeBShort should be: 18 bytes (6 + 6*2)" << std::endl;
    std::cout << "PacketEventNearTypeBDword should be: 20 bytes (6 + 5*2 + 4)" << std::endl;

    return 0;
}
