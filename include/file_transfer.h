#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <string>

class FileTransfer {
public:
    // Handles the file request: Checks existence, sends header, streams content via sendfile
    static void handle_file_request(int client_fd, const std::string& filename);
};

#endif // FILE_TRANSFER_H
