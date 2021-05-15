
#include <vectrex/bios.h>
#include <vectrex/stdlib.h>

// Header
#pragma vx_copyright "2021"
#pragma vx_title_pos 40,-125
#pragma vx_title_size -14, 110
#pragma vx_title "HERE BE DRAGONS"
#pragma vx_music vx_music_1


// ROM content.

// Provides:
// * const uint8_t edgeCounts[]; index: [objectId]
//		contains the edge count for each object
// * for each object (N = objectId)
// 		* const int8_t verticesN[][]; index: [angleId][vertexId] : 
//			two int8_t representing screenspace x,y in -127,127 foreach vertex at a given camera angle in 0,127
// 		* const uint16_t edgesN[]; index: [edgeId]
//			one uint16_t packing the two 8-bits vertexIds for the current edge

#include "resources.h"

#define MAX_EDGES 35
#define MAX_OBJECTS 4

// Helpers

/* Draw list instruction, see packId for explanation  */
void packets(int8_t* list)
{
	asm
	{
		JSR $F1AA
		LDX list
		JSR $F40C
	}
}

// Main program.

int main(){
	
	// Scratch buffers for computations.
	// scale + #edges * { move dy dx, draw dy dx } + end marker
	int8_t packs[1 + MAX_EDGES * 3 * 2 + 1];

	// Pre-fill draw list "action" bytes (alternating 0x00 Y X and 0xFF Y X, see packId below)
	char flip = 0;
	for(uint16_t i = 1; i < sizeof(packs) / sizeof(packs[0]); i += 3){
		packs[i] = flip ? 0xFF : 0x00;
		packs[i+1] = 0x00;
		packs[i+2] = 0x00;
		flip = !flip;
	}
	
	// Input.
	uint8_t angle = 0; 
	uint8_t speed = 1;
	uint8_t scale = 0xAF;
	uint16_t frameId;
	intensity(0x7F);

	// All edges.
	const uint16_t* edges[MAX_OBJECTS] = {&edges0, &edges1, &edges2, &edges3};


	while(1){
		wait_retrace();
		
		// Input handling
		{
#ifndef SKIP_INPUT_HANDLING

			const uint8_t buttons = read_buttons();
			const uint8_t joy = read_joystick(1);

			if(buttons & JOY1_BTN4_MASK){
				speed += 1u;
			} else if((buttons & JOY1_BTN3_MASK) && (speed > 0u)){
				speed -= 1u;
			}

			if(joy & JOY_LEFT_MASK){
				// Clamp to 0,127
				angle = (angle - speed) & 0x7F;

			} else if(joy & JOY_RIGHT_MASK){
				// Clamp to 0,127
				angle = (angle + speed) & 0x7F;

			} else if(joy & JOY_UP_MASK){
				scale += speed;

			} else if(joy & JOY_DOWN_MASK){
				scale -= speed;

			}
#endif
		}

		// Store global scene scale (~ FoV)
		packs[0] = scale;

		int8_t prevy = 0;
		int8_t prevx = 0;

		// Needed by packets() ?
		move(0,0);

		// Per-object projected vertices for the current angle.
		const int8_t* vertices[MAX_OBJECTS] = {&vertices0[angle][0], 
			&vertices1[angle][0], &vertices2[angle][0], &vertices3[angle][0]};
		
		// Frame counter.
#ifdef ONE_OBJECT_PER_FRAME
		frameId = ((uint16_t)(*((uint16_t*)(0xC825))));
		uint8_t oid = (uint8_t)(frameId % MAX_OBJECTS);
#else
		for(uint8_t oid = 0; oid != MAX_OBJECTS; ++oid)
#endif
		{
			// Pack contains a draw list of triplets: action,dy,dx
			// where action is either : 
			// 		* move (0x00)
			// 		* line (0xFF)
			// 		* end of list (0x01)
			int8_t* packId = &packs[1];

			for(uint8_t eId = 0; eId != edgeCounts[oid]; ++eId){
				// Read and unpack the two vertex indices
				const uint16_t id = edges[oid][eId];
				const int8_t id0 = (int8_t)(id >> 8);
				const int8_t id1 = (int8_t)(id & 0xFF);
				// Corresponding screen space coordinates.
				const int8_t p0y = vertices[oid][id0];
				const int8_t p0x = vertices[oid][id0+1];
				const int8_t p1y = vertices[oid][id1];
				const int8_t p1x = vertices[oid][id1+1];
				
				//packs[packId ] = 0x00; // already setup in init
				*(++packId) = p0y - prevy;
				*(++packId) = p0x - prevx;
				++packId;
				//packs[packId + 3] = 0xFF; // already setup in init
				*(++packId) = p1y - p0y;
				*(++packId) = p1x - p0x;
				++packId;

				// Store current beam position for next move.
				prevy = p1y;
				prevx = p1x;
				
			}
			// Mark end of list
			*packId = 0x01;
			// Draw
			packets(packs);
			// Unmark end of list
			*packId = 0x00;
		}
		
	}
	return 0;
}
