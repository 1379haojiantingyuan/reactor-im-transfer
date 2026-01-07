#include "ui_ncurses.h"

ClientUI::ClientUI() : msg_win(nullptr), input_win(nullptr) {}

ClientUI::~ClientUI() {
    cleanup();
}

void ClientUI::init() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    getmaxyx(stdscr, height, width);
    
    // Top 80% for messages
    msg_win = newwin(height * 0.8, width, 0, 0);
    scrollok(msg_win, TRUE);
    
    // Bottom 20% for input
    input_win = newwin(height * 0.2, width, height * 0.8, 0);
    scrollok(input_win, TRUE);
    
    refresh();
    wrefresh(msg_win);
    wrefresh(input_win);
}

void ClientUI::cleanup() {
    if (msg_win) delwin(msg_win);
    if (input_win) delwin(input_win);
    endwin();
}

void ClientUI::print_message(const std::string& msg) {
    std::lock_guard<std::mutex> lock(ui_mutex);
    wprintw(msg_win, "%s\n", msg.c_str());
    wrefresh(msg_win);
}

std::string ClientUI::get_input() {
    std::string input;
    int ch;
    
    {
        std::lock_guard<std::mutex> lock(ui_mutex);
        wmove(input_win, 0, 0);
        wprintw(input_win, "> ");
        wrefresh(input_win);
    }
    
    // Simple input loop (ncurses wgetnstr is blocking but assumes no interference, 
    // threading with ncurses is tricky. 
    // Safe approach: Main thread handles ALL input waiting. 
    // And print_message uses mutex to refresh other window.)
    
    char buf[1024];
    echo(); // Enable echo for input
    wgetnstr(input_win, buf, 1023);
    noecho();
    
    input = std::string(buf);
    
    // Clear input line
    {
        std::lock_guard<std::mutex> lock(ui_mutex);
        wclear(input_win);
        wrefresh(input_win);
    }
    
    return input;
}
