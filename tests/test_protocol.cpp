#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>
#include "../include/protocol.h"

// Manual mock of the parsing logic found in network_epoll.cpp
// In a real scenario, parsing logic should be in a separate class (e.g. ProtocolParser).
// Here we replicate the logic to verify correctness.

void test_packet_parsing() {
    std::cout << "[Test] Protocol Parsing: Starting..." << std::endl;

    std::vector<char> read_buffer;

    // 1. Create a dummy login packet
    PacketHeader hdr;
    hdr.msg_type = MSG_LOGIN;
    hdr.crc32 = 0;
    
    LoginBody body;
    strcpy(body.username, "TestUser");
    
    hdr.total_len = sizeof(PacketHeader) + sizeof(LoginBody);

    // 2. Serialize to buffer
    std::vector<char> packet(hdr.total_len);
    memcpy(packet.data(), &hdr, sizeof(PacketHeader));
    memcpy(packet.data() + sizeof(PacketHeader), &body, sizeof(LoginBody));

    // 3. Simulate Sticky Packet: Push 1 full packet + 0.5 packet
    read_buffer.insert(read_buffer.end(), packet.begin(), packet.end());
    read_buffer.insert(read_buffer.end(), packet.begin(), packet.begin() + sizeof(PacketHeader)); // Partial

    // 4. Test Extraction
    // Check Header
    assert(read_buffer.size() >= sizeof(PacketHeader));
    PacketHeader extracted_hdr;
    memcpy(&extracted_hdr, read_buffer.data(), sizeof(PacketHeader));
    
    assert(extracted_hdr.msg_type == MSG_LOGIN);
    assert(extracted_hdr.total_len == hdr.total_len);

    // Check Body availability
    assert((int)read_buffer.size() >= extracted_hdr.total_len);
    
    // "Process" it (Remove from buffer)
    read_buffer.erase(read_buffer.begin(), read_buffer.begin() + extracted_hdr.total_len);

    // 5. Verify remaining data is incomplete (the 0.5 packet)
    if (read_buffer.size() >= sizeof(PacketHeader)) {
         PacketHeader partial_hdr;
         memcpy(&partial_hdr, read_buffer.data(), sizeof(PacketHeader));
         // Should fail check for full body
         assert((int)read_buffer.size() < partial_hdr.total_len);
    }

    std::cout << "[Test] Protocol Parsing: Passed." << std::endl;
}

int main() {
    test_packet_parsing();
    return 0;
}
