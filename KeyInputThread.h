#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <sys/select.h>
#include <unistd.h>

inline void keyInputThread
(
    std::function<bool (char)> onQuitCallback
  , std::function<void ()> onIterationCallback
  , uint16_t timeoutBetweenReminders
)
{
    for(int iC = 0; true ; ++iC)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        
        struct timeval tv;
        tv.tv_sec = iC == 0 ? 1 : timeoutBetweenReminders;  // Timeout in seconds
        tv.tv_usec = 0;

        int ret = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
        
        if (ret == -1) {
            // Handle error, e.g., break or log an error message
            break;
        } else if (ret == 0) {
            onIterationCallback(); 
        } else {
            // Input is available: read and process it
            char input;
            std::cin >> input;
            if(onQuitCallback(input))
            {
                break;
            }
        }
    }
}