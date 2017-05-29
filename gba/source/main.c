#include <gba_video.h>
#include <gba_base.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_sprites.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"


// ------------------------
// OAM buffer.
// ------------------------

// Modifying OAM immediatly would stall drawing.
OBJATTR obj_buffer[128];

// Update attributes for object 0 to count-1.
void updateOAM(int count){
	for(int i = 0; i < count * sizeof(OBJATTR) / 4 ; i++){
		((u32*)OAM)[i] = ((u32*)obj_buffer)[i];
	}
}

// Update i-th object position.
void updatePos(int i, int x, int y){
	obj_buffer[i].attr1 &= 0xFE00;
	obj_buffer[i].attr1 |= (x & 0x01FF);
	obj_buffer[i].attr0 &= 0xFF00;
	obj_buffer[i].attr0 |= (y & 0x00FF);
}

// ------------------------
// Setup sprites attributes.
// ------------------------

void setupSprite(int i, int x, int y, int index, int palette){
	obj_buffer[i].attr2 = OBJ_CHAR(index) | OBJ_PALETTE(palette);
	updatePos(i, x, y);
}

void setupSprite64x64(int i, int x, int y, int index, int palette){
	obj_buffer[i].attr0 = OBJ_16_COLOR | OBJ_SQUARE ;
	obj_buffer[i].attr1 = ATTR1_SIZE_64;
	setupSprite(i, x, y, index, palette);
}

void setupSprite64x32(int i, int x, int y, int index, int palette){
	obj_buffer[i].attr0 = OBJ_16_COLOR | OBJ_WIDE ;
	obj_buffer[i].attr1 = ATTR1_SIZE_64;
	setupSprite(i, x, y, index, palette);
}

void setupSprite16x16(int i, int x, int y, int index, int palette){
	obj_buffer[i].attr0 = OBJ_16_COLOR | OBJ_SQUARE ;
	obj_buffer[i].attr1 = ATTR1_SIZE_16;
	setupSprite(i, x, y, index, palette);
}

// ------------------------
// Main function.
// ------------------------

int main(void) {
	
	// Set up the interrupt handlers
	irqInit(); REG_IME = 1;
	// Enable Vblank Interrupt to allow VblankIntrWait
	irqEnable(IRQ_VBLANK);
	
	// General display setup: Mode0, enable objects stored as 1D map, enable backgrounds 0-2.
	REG_DISPCNT = (MODE_0 | OBJ_ON | OBJ_1D_MAP | BG0_ON | BG1_ON  | BG2_ON);
	// Keys repeat sensitivity: activate after one scan, every scan.
	setRepeat(1,1);
	
	// ------------------------
	// Backgrounds.
	// ------------------------
	
	// Setup backgrounds: 16 colors palettes, indicate map and tiles bank, enable wrapping, all of size 256x256.
	REG_BG0CNT = BG_16_COLOR | BG_MAP_BASE(31) | BG_TILE_BASE(0) | BG_WRAP | BG_SIZE_0 | 0; // Ground
	REG_BG1CNT = BG_16_COLOR | BG_MAP_BASE(29) | BG_TILE_BASE(1) | BG_WRAP | BG_SIZE_0 | 1; // Lower sky
	REG_BG2CNT = BG_16_COLOR | BG_MAP_BASE(27) | BG_TILE_BASE(1) | BG_WRAP | BG_SIZE_0 | 2; // Higher sky (high priority : drawn first).
	
	// Store the backgrounds palettes in the VRAM bank.
	u16* bgpal = BG_PALETTE;
	for(int i=0; i<PALETTESIZE; i++) {
		*bgpal++ = ((u16*)bg0Palette)[i];
	}
	for(int i=0; i<PALETTESIZE; i++) {
		*bgpal++ = ((u16*)bg1Palette)[i];
	}
	
	// Store the tiles in tile banks 0 and 1.
	CpuFastSet(bg0Tiles, CHAR_BASE_ADR(0) , COPY32 | TILESBG0LEN);
	CpuFastSet(bg1Tiles, CHAR_BASE_ADR(1) , COPY32 | TILESBG1LEN);
	// Background 2 uses the same tiles and palette as background 1.
	// Store the maps.
	CpuFastSet( bg0Map, MAP_BASE_ADR(31), COPY32 | BGMAPLEN);
	CpuFastSet( bg1Map, MAP_BASE_ADR(29), COPY32 | BGMAPLEN);
	CpuFastSet( bg2Map, MAP_BASE_ADR(27), COPY32 | BGMAPLEN);
	
	// Setup scroll for each background (for movement and parallax effects).
	bg_scroll scr0, scr1, scr2;
	scr0.x = 0; scr0.y = 32;
	scr1.x = 0; scr1.y = 12+48;
	scr2.x = 0; scr2.y = 12+48;
	// Set corresponding offset registers.
	REG_BG0VOFS = scr0.y;
	REG_BG1VOFS = scr1.y;
	REG_BG2VOFS = scr2.y;
	
	// ------------------------
	// Objects setup.
	// ------------------------
	
	// First, hide/disable all objects.
	for(int i = 0; i < 128; i++){
		obj_buffer[i].attr0 = OBJ_DISABLE;
	}
	
	// Store the sprites palettes in the VRAM bank.
	u16 *pal = SPRITE_PALETTE;
	for(int i=0; i<PALETTESIZE; i++) {
		*pal++ = ((u16*)monkeyPalette)[i];
	}
	for(int i=0; i<PALETTESIZE; i++) {
		*pal++ = ((u16*)dragonPalette)[i];
	}
	for(int i=0; i<PALETTESIZE; i++) {
		*pal++ = ((u16*)sunPalette)[i];
	}
	
	// Objects positions.
	int dragonX = 40;
	int dragonY = 38;
	int monkeyX = 120;
	int monkeyY = 78;
	int sunX = 180;
	int sunY = 8;
	
	// Setup sprites positions.
	// The dragon is split in 4 sprites: two of size 64x64, two of size 64x32.
	setupSprite64x64(0, monkeyX, monkeyY, 0, 0);
	setupSprite64x64(1, dragonX, dragonY, 64*8, 1);
	setupSprite64x64(2, dragonX+64, dragonY, 64*9, 1);
	setupSprite64x32(3, dragonX, dragonY+64, 64*10, 1);
	setupSprite64x32(4, dragonX+64, dragonY+64, 64*10+32, 1);
	setupSprite16x16(5, sunX, sunY, 64*10+2*32, 2);
	
	// Store sprites in 4th tiles bank.
	// Monkey sprites (the monkey is animated, using 8 frames.
	CpuFastSet(monkey0Tiles, CHAR_BASE_ADR(4)		 			   , COPY32 | TILES64x64LEN);
	CpuFastSet(monkey1Tiles, CHAR_BASE_ADR(4) +     TILES64x64SIZE , COPY32 | TILES64x64LEN);
	CpuFastSet(monkey2Tiles, CHAR_BASE_ADR(4) + 2 * TILES64x64SIZE , COPY32 | TILES64x64LEN);
	CpuFastSet(monkey3Tiles, CHAR_BASE_ADR(4) + 3 * TILES64x64SIZE , COPY32 | TILES64x64LEN);
	CpuFastSet(monkey4Tiles, CHAR_BASE_ADR(4) + 4 * TILES64x64SIZE , COPY32 | TILES64x64LEN);
	CpuFastSet(monkey5Tiles, CHAR_BASE_ADR(4) + 5 * TILES64x64SIZE , COPY32 | TILES64x64LEN);
	CpuFastSet(monkey6Tiles, CHAR_BASE_ADR(4) + 6 * TILES64x64SIZE , COPY32 | TILES64x64LEN);
	CpuFastSet(monkey7Tiles, CHAR_BASE_ADR(4) + 7 * TILES64x64SIZE , COPY32 | TILES64x64LEN);
	// Dragon sprites.
	CpuFastSet(dragon0Tiles, CHAR_BASE_ADR(4) + 8 * TILES64x64SIZE , COPY32 | TILES64x64LEN);
	CpuFastSet(dragon1Tiles, CHAR_BASE_ADR(4) + 9 * TILES64x64SIZE , COPY32 | TILES64x64LEN);
	CpuFastSet(dragon2Tiles, CHAR_BASE_ADR(4) + 10* TILES64x64SIZE , COPY32 | TILES64x32LEN);
	CpuFastSet(dragon3Tiles, CHAR_BASE_ADR(4) + 10* TILES64x64SIZE + TILES64x32SIZE , COPY32 | TILES64x32LEN);
	// Sun sprite
	CpuFastSet(sun0Tiles,    CHAR_BASE_ADR(4) + 10* TILES64x64SIZE + 2* TILES64x32SIZE , COPY32 | TILES16x16LEN);
	
	// Update all sprites attributes.
	updateOAM(128);
	
	// ------------------------
	// Main loop.
	// ------------------------
	
	int time = 0;
	int monkeySprite = 0;
	
	while(1){
		// Wait for vblank interrupt (at the end of screen drawing)
		VBlankIntrWait();
		
		// Scan keys.
		scanKeys();
		// Get keys that are pressed, even if already pressed at the previous frame.
		u16 keys = keysDownRepeat();
		
		// We use the monkey coordinates to check if we move in the allowed range on both axis.
		// To get a parallax object, objects closer to the viewer move faster.
		// Backgrounds have to be shifted in the opposition idrection of sprites movement.
		// We have 4 layers: sun, background sky, foreground sky, ground + monkey + dragon.
		
		// Move left.
		if(keys & KEY_LEFT && monkeyX < 180){
			scr0.x-=3;
			scr1.x-=2;
			scr2.x-=1+time%2;
			sunX+=1;
			dragonX+=3;
			monkeyX+=3;
		}
		// Move right.
		if(keys & KEY_RIGHT && monkeyX > 70){
			scr0.x+=3;
			scr1.x+=2;
			scr2.x+=1+time%2;
			sunX-=1;
			dragonX-=3;
			monkeyX-=3;
		}
		// Move down.
		if(keys & KEY_DOWN && monkeyY > 40){
			scr0.y+=3;
			scr1.y+=2;
			scr2.y+=1+time%2;
			sunY-=1;
			dragonY-=3;
			monkeyY-=3;
		}
		// Move up.
		if(keys & KEY_UP && monkeyY < 100){
			scr0.y-=3;
			scr1.y-=2;
			scr2.y-=1+time%2;
			sunY+=1;
			dragonY+=3;
			monkeyY+=3;
		}
		
		// If the A button is pressed, reset "camera".
		if(keys & KEY_A){
			scr0.x = 0;		scr0.y = 32;
			scr1.x = 0;		scr1.y = 12+48;
			scr2.x = 0;		scr2.y = 12+48;
			dragonX = 40;	dragonY = 38;
			monkeyX = 120;	monkeyY = 78;
			sunX = 180;		sunY = 8;
		}
		
		// If any of these keys were pressed, update sprites and backgrounds parameters.
		if(keys & (DPAD | KEY_A)){
			// Update backgrounds registers.
			REG_BG0HOFS = scr0.x; REG_BG0VOFS = scr0.y;
			REG_BG1HOFS = scr1.x; REG_BG1VOFS = scr1.y;
			REG_BG2HOFS = scr2.x; REG_BG2VOFS = scr2.y;
			// Update objects positions.
			updatePos(0, monkeyX, monkeyY);
			updatePos(1, dragonX, dragonY);
			updatePos(2, dragonX+64, dragonY);
			updatePos(3, dragonX, dragonY+64);
			updatePos(4, dragonX+64, dragonY+64);
			updatePos(5, sunX, sunY);
			// Update the 7 first objects (monkey, dragon and sun).
			updateOAM(6);
		}
		
		// Update monkey sprite.
		if(time%9 == 0){
			// Compute next frame sprite id.
			monkeySprite = (monkeySprite == 7) ? 0 : (monkeySprite+1);
			// Update attribute with the new frame sprite id.
			obj_buffer[0].attr2 = OBJ_CHAR(64*monkeySprite) | OBJ_PALETTE(0);
			// Update the monkey object (the first one).
			updateOAM(1);
		}
		// Increase time.
		time++;
	}
	
	return 0;
}


