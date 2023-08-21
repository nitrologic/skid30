#include <iostream>
#include <windows.h>
#include <conio.h>
#include <fstream>
#include <string>
#include <vector>


int mainDelay=80;
int counter=0;
int key = 0;
int monsterCount = 0;


int keyboardKey() {
	if (_kbhit()) {
		return getch();
	}
	return -1;
}

enum blockTypes {
	WALL=1,
	MONSTER=2
};

char tileChar[] = {' ', '#', 'M'};

enum keyCodes {
	KEY_ESC=27
};

std::string title;



struct monstermap {
	int rows, cols;
	std::vector<int> data;

	monstermap(int rows0, int cols0) {
		rows = rows0;
		cols = cols0;
		int size = rows * cols;
		data.resize(size);
	}

	void set(int x, int y, int m) {
		data[x + y * cols] = m;
	}

	int get(int x, int y) {
		return data[x + y * cols];
	}

	std::string getRow(int row) {
		std::vector<char> chars;
		for (int col = 0; col < cols; col++) {
			int tile = data[col + row * cols];
			chars.push_back(tileChar[tile]);
		}
		return std::string(chars.data(),cols);
	}


};

monstermap* map0;

struct monster {
	int dir;
	int x, y;
	int delay;
	
	monster(int x0, int y0) {
		x = x0;
		y = y0;
		int count = monsterCount++;
		dir = count & 3;
		delay = 10;
	}

	void move() {
		if (delay) {
			delay--;
			return;
		}
		int x0 = x; int y0 = y;

		switch (dir&3) {
		case 0:y0--; break;
		case 1:x0++; break;
		case 2:y0++; break;
		case 3:x0--; break;
		}

		int tile=map0->get(x0, y0);
		if (tile == 0) {			
			map0->set(x, y, 0);
			x = x0;
			y = y0;
			map0->set(x, y, 2);
		}
		else {
			delay = 10;
			dir--;
		}
	}
};


std::vector<monster*> monsters;

void addMonster(int x, int y) {
	monsters.push_back(new monster(x, y));
}

int maprows = 0;
int mapcols = 0;

void moveMonsters() {
	for (auto m : monsters) {
		m->move();
	}
}



int main() {

	SetConsoleOutputCP(CP_UTF8);

// READ LEVEL FROM FILE

	std::ifstream file("gamelevel.txt");
	std::string str;
	std::vector<std::string> strings;
	int linenumber = 0;
	while (std::getline(file, str))
	{
		linenumber++;
		if (str == "") break;
		if (linenumber == 1) {
			title = str;
			continue;
		}
		int n = str.size();
		if (n > mapcols) mapcols = n;
		strings.push_back(str);
	}
	maprows = strings.size();

	map0=new monstermap(maprows, mapcols);

	for (int i = 0; i < maprows; i++) {
		std::string s=strings[i];
		for (int j = 0; j < s.size(); j++) {
			char c = s[j];

			if (c == 32) continue;

			if (c == '*') {
				map0->set(j, i, WALL);				
			}

			if (c == 'M') {
				map0->set(j, i, MONSTER);
				addMonster(j, i);
			}
		}

	}

// MAIN LOOP	

	while (true) {
		// home the cursor
		std::cout << "\033[0;0f" << std::flush;

		std::cout << title << std::endl << std::endl;

		for (int i = 0; i < maprows; i++) {
			std::string s = map0->getRow(i);
			std::cout << s << std::endl;
		}

		moveMonsters();
		
		std::cout << counter << std::endl;

		std::cout << "key:" << key << "  " << std::endl;
		
		Sleep(mainDelay);

        counter++;

		int keycode = keyboardKey();

		if (key == KEY_ESC) break;

		if (keycode > -1) key = keycode;

	}

	std::cout << "mob game" << std::endl;
}
