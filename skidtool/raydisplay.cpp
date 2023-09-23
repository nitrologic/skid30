#include <iostream>
#include <raylib.h>
#include <string>

#include "loadiff.h"

using S=std::string;

S itos(int i){return std::to_string(i);}
int systemClock = 0;

int readClock() {
	return systemClock++;
}

#define DRAWTEXT

// relative to ProjectDir

const char* trackpng = "../../maps/format.png";
const char* skidmed = "../../MyACID500/skidaf/gfx/skidmarks.med";
const char* engineiff = "../../MyACID500/skidaf/gfx/engine.iff";
const char* clickwav = "../../archive/click.wav";

const char* title = "ACID500";
const char* version = "raydisplay 0.2";
//const char* splash = "f11:fullscreen";
const char* start = "Start when steady";

int frameCount = 0;

extern"C"{
int glfwInit(void); 
}


void DrawBox(int x, int y, int w, int h, Color c) {
	DrawRectangleLines(x, y, w, h, c);
}

int main(){
	std::cout << "raydisplay 0.0" << std::endl;

	glfwInit();

	int hz = 60;
	int screenwidth=0;
	int screenheight=0;
	int screens = GetMonitorCount();
	std::cout << "screens:" << screens << std::endl;
	for (int i = 0; i < screens; i++) {
		int w = GetMonitorWidth(i);
		int h = GetMonitorHeight(i);
		int ww = GetMonitorPhysicalWidth(i);
		int hh = GetMonitorPhysicalHeight(i);
		hz = GetMonitorRefreshRate(i);
		Vector2 xy=GetMonitorPosition(i);
		std::string name(GetMonitorName(i));
		std::cout << name << " " << w << "x" << h << " " << ww << "X" << hh;
		std::cout << " @" << xy.x << "," << xy.y << " " << hz << "hz " << std::endl;
		screenwidth=w;
		screenheight=h;
	}
//	return 0;

	InitAudioDevice();

	SetTargetFPS(hz);
	SetConfigFlags(FLAG_VSYNC_HINT|FLAG_FULLSCREEN_MODE);
//	SetConfigFlags(FLAG_VSYNC_HINT|FLAG_FULLSCREEN_MODE|FLAG_WINDOW_HIGHDPI);
//	InitWindow(768*2, 580*2, title);
//	InitWindow(1920/2, 1080/2, title);
	InitWindow(2040, 1332, title);
//	InitWindow(screenwidth/2, screenheight/2, title);
//	InitWindow(screenwidth, screenheight, title);

	Texture2D track=LoadTexture(trackpng);

#ifdef engine
	Raw bytes=loadSVX(engineiff);
	Wave wave = { bytes.size(), 12000,8,1,bytes.data() };
	Sound engine=LoadSoundFromWave(wave);
	PlaySound(engine);
#endif

	Sound click = LoadSound(clickwav);
	PlaySound(click);

//	Music mod = LoadMusicStream(skidmod);
		
	while (!WindowShouldClose()){
		frameCount++;

		float tx = frameCount * 0.025 * hz;
		float ty = frameCount * 0.010 * hz;

		int width = GetRenderWidth();
		int height = GetRenderHeight();

		PollInputEvents();

		BeginDrawing();
		ClearBackground({ 0,0,0 });

//		DrawTexture(track, 0, 0, WHITE);
		DrawTextureEx(track, { -tx,-ty },0, 10, WHITE);
#ifdef DRAWTEXT
		int x = 14;
		int y = 20;
		int h = 48;
		Color c = GOLD;
		DrawText(title, x, y, h, c); y += h+12;
		DrawText(version, x, y, h, c); y += h+12;
		S splash=S("res:")+itos(width)+","+itos(height);
		DrawText(splash.c_str(), x, y, h, c); y += h+12;
		DrawText(start, x, y, h, c); y += h+12;

		DrawFPS(x, y);

//		int fps = 1.0/GetFrameTime();
//		std::string stats = "fps:" + std::to_string(fps);
//		DrawText(stats.c_str(), x, y, h/2, c); y += h + 12;

		int mx = GetMouseX();
		int my = GetMouseY();
		DrawText("hello", mx, my, 24, { 255,255,255,60 } );
#endif

		EndDrawing();		
				
		if (IsKeyPressed(KEY_F11)) {
			ToggleFullscreen();
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			PlaySound(click);
		}
	}


	if(IsWindowFullscreen())
		ToggleFullscreen();

	std::cout << "complete" << std::endl;
}
