#pragma once

void initNative();
int millis();
void dayminticks(int *dmt);
void sleep(int ms);

int getch2();
int getch3();

void initConsole();
void screenSize(int& columns, int& rows);
