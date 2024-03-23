#ifndef DRAW_HPP
#define DRAW_HPP

#include <kernel.h>
#include <tamtypes.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <packet2.h>

#define VU1_BUFFER_SIZE 496
#define BATCH_VERTEX_SIZE 96

// Helpers

struct Memory {
	unsigned int texture;
	unsigned int palette;
	unsigned int programObject;
	unsigned int programSkybox;
};

struct Commands {
	
	void allocate();

	void beginFrame();

	packet2_t* nextGeneral();
	packet2_t* nextTexture();
	packet2_t* nextIndirect();
	packet2_t* nextDraw();

	void clean();
	
private:

	packet2_t* mainPacket[2];
	packet2_t* texturePacket[2]; 
	packet2_t* vu1Packets[2] __attribute__((aligned(64)));
	packet2_t* indirectPacket;
	
	int currentFrame{0};
	int currentDraw{0};
};

#endif
