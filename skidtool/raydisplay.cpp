#include <iostream>
#include <raylib.h>
#include <string>

const char* title = "ACID500";
const char* version = "raydisplay 0.2";
const char* splash = "f11:fullscreen";
const char* start = "Start when steady";

int frameCount = 0;

void DrawBox(int x, int y, int w, int h, Color c) {
	DrawRectangleLines(x, y, w, h, c);
}

int main(){
	std::cout << "raydisplay 0.0" << std::endl;

//	InitWindow(768*2, 580*2, title);
	InitWindow(1920/2, 1080/2, title);

	int screens = GetMonitorCount();
	std::cout << "screens:" << screens << std::endl;

	while (!WindowShouldClose()){
		frameCount++;

		int width = GetRenderWidth();
		int height = GetRenderHeight();

		PollInputEvents();

		BeginDrawing();
		ClearBackground({ 0,0,0 });
		int x = 14;
		int y = 20;
		int h = 48;
		Color c = GOLD;
		DrawText(title, x, y, h, c); y += h+12;
		DrawText(version, x, y, h, c); y += h+12;
		DrawText(splash, x, y, h, c); y += h+12;
		DrawText(start, x, y, h, c); y += h+12;

		for (int t = 0; t < 50; t++) {
			int f = (frameCount + t*50) % 2500;
			int o = f * f / 320;
			DrawBox(o, o, width - 2*o, height - 2*o,DARKBLUE);
		}

		int fps = 1000/GetFrameTime();
		std::string stats = "fps:" + std::to_string(fps);
		DrawText(stats.c_str(), x, y, h/2, c); y += h + 12;

		int mx = GetMouseX();
		int my = GetMouseY();
		DrawText("hello", mx, my, 24, { 255,255,255,60 } );
		EndDrawing();
				
		if (IsKeyPressed(KEY_F11)) {
			ToggleFullscreen();
		}
	}

	std::cout << "complete" << std::endl;
}
