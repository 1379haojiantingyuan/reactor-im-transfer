#ifndef UI_NCURSES_H
#define UI_NCURSES_H

#include <string>
#include <mutex>
#include <ncurses.h>

class ClientUI {
public:
    ClientUI();
    ~ClientUI();

    void init();
    void cleanup();
    
    // Thread-safe print
    void print_message(const std::string& msg);
    
    // Blocking input
    std::string get_input();

private:
    WINDOW* msg_win;
    WINDOW* input_win;
    std::mutex ui_mutex;
    int width, height;
};

#endif // UI_NCURSES_H
