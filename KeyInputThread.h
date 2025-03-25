#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <sys/select.h>
#include <unistd.h>

inline void keyInputThread(std::function<void()> onQuitCallback, std::function<void()> onIterationCallback, std::atomic<bool>& running, uint16_t timeoutBetweenReminders) {
    std::cout << "Press 'q' and Enter to quit\n";
    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        
        struct timeval tv;
        tv.tv_sec = timeoutBetweenReminders;  // Timeout in seconds
        tv.tv_usec = 0;

        int ret = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
        
        if (ret == -1) {
            // Handle error, e.g., break or log an error message
            break;
        } else if (ret == 0) {
            onIterationCallback(); 
            // Timeout occurred: print the reminder
            std::cout << "Press 'q' and Enter to quit\n";
        } else {
            // Input is available: read and process it
            char input;
            std::cin >> input;
            if (input == 'q' || input == 'Q') {
                onQuitCallback();
                running = false;
                break;
            }
        }
    }
}