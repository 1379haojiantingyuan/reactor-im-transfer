#include "../include/file_transfer.h"
#include "../include/protocol.h"
#include "../include/logger.h"
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <vector>

void FileTransfer::handle_file_request(int client_fd, const std::string& filename) {
    std::string full_path = "./file_storage/" + filename;
    
    int file_fd = open(full_path.c_str(), O_RDONLY);
    if (file_fd < 0) {
        LOG_ERROR("Failed to open file: " + full_path);
        // Optional: Send error message to client
        return;
    }

    struct stat stat_buf;
    fstat(file_fd, &stat_buf);
    
    // 1. Send Header indicating File Data is coming
    PacketHeader header;
    header.msg_type = MSG_FILE_DATA;
    header.crc32 = 0;
    // Total len = Header + File Content
    // Note: If file is huge (>2GB), int32_t length might overflow. 
    // For this project, assuming files < 2GB.
    header.total_len = sizeof(PacketHeader) + stat_buf.st_size;

    // Send Header first (Standard write)
    write(client_fd, &header, sizeof(PacketHeader));

    // 2. Zero-Copy Send
    off_t offset = 0;
    size_t remaining = stat_buf.st_size;
    
    LOG_INFO("Starting zero-copy transfer for " + filename + " (" + std::to_string(remaining) + " bytes)");
    
    while (remaining > 0) {
        ssize_t sent = sendfile(client_fd, file_fd, &offset, remaining);
        if (sent <= 0) {
            if (errno == EAGAIN) continue;
            LOG_ERROR("sendfile failed");
            break;
        }
        remaining -= sent;
    }
    
    LOG_INFO("File transfer complete.");
    close(file_fd);
}
