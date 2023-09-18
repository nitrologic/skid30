#include <iostream>
#include <raylib.h>
#include <string>

// relative to ProjectDir

const char* trackpng = "../../maps/format.png";

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

	Texture2D track=LoadTexture(trackpng);
		
	while (!WindowShouldClose()){
		frameCount++;

		float tx = frameCount * 0.025;
		float ty = frameCount * 0.010;

		int width = GetRenderWidth();
		int height = GetRenderHeight();

		PollInputEvents();

		BeginDrawing();
		ClearBackground({ 0,0,0 });

//		DrawTexture(track, 0, 0, WHITE);
		DrawTextureEx(track, { -tx,-ty },0, 10, WHITE);

		int x = 14;
		int y = 20;
		int h = 48;
		Color c = GOLD;
		DrawText(title, x, y, h, c); y += h+12;
		DrawText(version, x, y, h, c); y += h+12;
		DrawText(splash, x, y, h, c); y += h+12;
		DrawText(start, x, y, h, c); y += h+12;

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

	if(IsWindowFullscreen())
		ToggleFullscreen();

	std::cout << "complete" << std::endl;
}
