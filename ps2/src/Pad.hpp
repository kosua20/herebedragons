#ifndef PAD_HPP
#define PAD_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>




class Pad {
	
public:
	
	enum Button {
		UP = 0, LEFT = 1, RIGHT = 2, DOWN = 3, CROSS = 4, TRIANGLE = 5, CIRCLE = 6, SQUARE = 7
	};
	
	Pad();
	
	int setup();
	
	void update();
	
	bool pressed(Button b);
	
private:
	
	Pad(const Pad &);
	Pad & operator = (const Pad &);
	
	void waitReady();
	
	bool _buttons[8];
	int _port;
	int _slot;
	u32 _old_pad;
	
};

#endif
