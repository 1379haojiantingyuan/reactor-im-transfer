#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <vector>
#include <string>

// Message Types
enum MsgType : int32_t {
    MSG_LOGIN       = 0x01, // Login
    MSG_CHAT_PUBLIC = 0x02, // Public Chat
    MSG_CHAT_PRIVATE= 0x03, // Private Chat
    MSG_FILE_REQ    = 0x04, // File Download Request
    MSG_FILE_DATA   = 0x05, // File Data Stream
    MSG_HEARTBEAT   = 0x06, // Heartbeat
    
    // Server Responses
    MSG_LOGIN_ACK   = 0x11, 
    MSG_ERROR       = 0xFF
};

// Fixed Header (12 bytes)
struct PacketHeader {
    int32_t total_len;  // Total length (Header + Body)
    int32_t msg_type;   // MsgType
    int32_t crc32;      // CRC32 Checksum (0 for now)
};

// Body Structures (Helpers for serialization)
struct LoginBody {
    char username[32]; 
};

struct ChatBody {
    char target_user[32]; // Empty for public chat
    char content[1024];   // Fixed size for simplicity in Phase 3
};

struct FileReqBody {
    char filename[256];
};

#endif // PROTOCOL_H
