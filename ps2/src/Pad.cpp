
#include <math3d.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <packet.h>

#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <debug.h>
#include <libpad.h>

#include "Pad.hpp"

Pad::Pad(){
	_buttons[0] = _buttons[1] = _buttons[2] = _buttons[3] = false;
	_buttons[4] = _buttons[5] = _buttons[6] = _buttons[7] = false;
	_port = _slot = _old_pad = 0;
}

int Pad::setup(){
	if (SifLoadModule("rom0:SIO2MAN", 0, NULL) < 0 || SifLoadModule("rom0:PADMAN", 0, NULL) < 0) {
		return 0;
	}
	
	padInit(0);
	
	static char padBuf[256] __attribute__((aligned(64)));
	if(padPortOpen(_port, _slot, padBuf) == 0) {
		return 0;
	}
	waitReady();
	padSetMainMode(_port, _slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
	waitReady();
	// Continuous inputs.
	padEnterPressMode(_port, _slot);
	waitReady();
	return 1;
}

void Pad::waitReady()
{
	int state = padGetState(_port, _slot);
	while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
		if(state == PAD_STATE_DISCONN) {
			printf("Pad(%d, %d) is disconnected\n", _port, _slot);
		}
		state = padGetState(_port, _slot);
	}
}

void Pad::update(){
	
	waitReady();
	
	struct padButtonStatus buttons;
	
	if (padRead(_port, _slot, &buttons) != 0) {
		
		// Useful for discrete presses.
		u32 paddata = 0xffff ^ buttons.btns;
		u32 new_pad = paddata & ~_old_pad;
		_old_pad = paddata;
		
		// Directions
		_buttons[LEFT] = buttons.left_p ? true : false;
		_buttons[DOWN] = buttons.down_p ? true : false;
		_buttons[RIGHT] = buttons.right_p ? true : false;
		_buttons[UP] = buttons.up_p ? true : false;
		
		_buttons[TRIANGLE] = buttons.triangle_p ? true : false;
		_buttons[CROSS] = buttons.cross_p ? true : false;
		_buttons[CIRCLE] = (new_pad & PAD_CIRCLE) ? true : false;
		_buttons[SQUARE] = (new_pad & PAD_SQUARE) ? true : false;
		
	}
}

bool Pad::pressed(Button b){
	return _buttons[b];
}


