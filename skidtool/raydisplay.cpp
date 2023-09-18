#include <iostream>
#include <raylib.h>

int main(){
    std::cout << "raydisplay 0.0" << std::endl;

    InitWindow(768, 580, "ACID500");

    int screens = GetMonitorCount();
    std::cout << "screens:" << screens << std::endl;

    while (!IsKeyPressed(27)) {
        PollInputEvents();
        WaitTime(0.1);


        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
    }
}
