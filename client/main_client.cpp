#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "client.h"
#include "ui_ncurses.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <username> [server_ip]" << std::endl;
        return 1;
    }

    std::string username = argv[1];
    std::string server_ip = (argc > 2) ? argv[2] : "127.0.0.1";
    int port = 8080;

    ChatClient client;
    if (!client.connect_to_server(server_ip, port)) {
        std::cerr << "Failed to connect to server." << std::endl;
        return 1;
    }

    ClientUI ui;
    ui.init();
    
    ui.print_message("Connected to server as " + username);
    ui.print_message("commands: /private <user> <msg>, /download <file>, /quit");

    // Callback to print received messages
    client.set_on_message([&ui](const std::string& msg) {
        ui.print_message(msg);
    });

    client.start_receiver();
    client.login(username);

    // Main Input Loop
    while (true) {
        std::string input = ui.get_input();
        
        if (input == "/quit") {
            break;
        } else if (input.rfind("/private ", 0) == 0) {
            // Parse /private <user> <msg>
            std::stringstream ss(input);
            std::string cmd, target, msg_word;
            ss >> cmd >> target;
            
            std::string msg;
            while (ss >> msg_word) {
                if (!msg.empty()) msg += " ";
                msg += msg_word;
            }
            
            client.send_chat_private(target, msg);
            ui.print_message("[To " + target + "]: " + msg);
        } else if (input.rfind("/download ", 0) == 0) {
            // Parse /download <filename>
            std::stringstream ss(input);
            std::string cmd, filename;
            ss >> cmd >> filename;
            
            if (!filename.empty()) {
                client.request_file(filename);
                ui.print_message("[System]: Requested file " + filename);
            } else {
                ui.print_message("[System]: Usage: /download <filename>");
            }
        } else {
            // Default: Public Chat
            client.send_chat_public(input);
        }
    }

    client.stop();
    ui.cleanup();
    return 0;
}
