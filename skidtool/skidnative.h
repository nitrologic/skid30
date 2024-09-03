#pragma once

void initNative();
int millis();
void dayminticks(int *dmt);
void sleep(int ms);

int waitChar();
int getChar();

void initConsole();
void screenSize(int& columns, int& rows);
