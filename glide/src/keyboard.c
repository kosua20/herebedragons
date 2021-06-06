#include "keyboard.h"

#include <pc.h>
#include <go32.h> 
#include <dpmi.h>

#include <string.h>

#define KEYBOARD_INT_ID 0x09

unsigned char pressedKeys[0xC0];

_go32_dpmi_seginfo oldKeyboardINT;
_go32_dpmi_seginfo newKeyboardINT;

void keyboardINTHandler(){
	static unsigned char buffer; // For passing info for extended keys.
	
	unsigned char rawCode = inp(0x60); // Read code from controller.
	unsigned char pressed = !(rawCode & 0x80); // Check bit 7.
	int scanCode = rawCode & 0x7F;

	if (buffer == 0xE0) { /* second byte of an extended key */
	    if (scanCode < 0x60) {
	        pressedKeys[KEY_EXT_SHIFT + scanCode] = pressed;
	    }
	    buffer = 0;
	} else if (buffer >= 0xE1 && buffer <= 0xE2) {
	    buffer = 0; /* ingore these extended keys */
	} else if (rawCode >= 0xE0 && rawCode <= 0xE2) {
	    buffer = rawCode; /* first byte of an extended key */
	} else if (scanCode < 0x60) {
	    pressedKeys[scanCode] = pressed;
	}

	// Finish interrupt.
	outportb(0x20, 0x20);
}

void keyboardINTHandlerEnd() { }

void initKeyboard(){
	_go32_dpmi_lock_data(&pressedKeys, sizeof(pressedKeys));
	_go32_dpmi_lock_code(keyboardINTHandler, (long)keyboardINTHandlerEnd - (long)keyboardINTHandler);

	_go32_dpmi_get_protected_mode_interrupt_vector(KEYBOARD_INT_ID, &oldKeyboardINT);
	newKeyboardINT.pm_selector = _go32_my_cs();
	newKeyboardINT.pm_offset = (unsigned long)keyboardINTHandler;
	_go32_dpmi_allocate_iret_wrapper(&newKeyboardINT);
	_go32_dpmi_set_protected_mode_interrupt_vector(KEYBOARD_INT_ID, &newKeyboardINT);

	memset(pressedKeys, 0, sizeof(pressedKeys));
}

void cleanupKeyboard(void) {
	_go32_dpmi_set_protected_mode_interrupt_vector(KEYBOARD_INT_ID, &oldKeyboardINT);
	_go32_dpmi_free_iret_wrapper(&newKeyboardINT);
}

unsigned char isPressed(const int key){
	return pressedKeys[key];
}