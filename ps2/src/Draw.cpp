
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <packet2.h>
#include <packet2_utils.h>
#include <malloc.h>

#include "Draw.hpp"

void Commands::allocate(){
	for(int i = 0; i < 2; ++i){
		mainPacket[i] = packet2_create(1024, P2_TYPE_NORMAL, P2_MODE_CHAIN, 0);
		texturePacket[i] = packet2_create(1024, P2_TYPE_NORMAL, P2_MODE_CHAIN, 0);
		vu1Packets[i] = packet2_create(1024, P2_TYPE_NORMAL, P2_MODE_CHAIN, 1);
	}
	indirectPacket = packet2_create(32, P2_TYPE_NORMAL, P2_MODE_CHAIN, 1);

	currentFrame = 0;
	currentDraw = 0;
}


void Commands::beginFrame(){
	currentFrame = 1 - currentFrame;
}

packet2_t* Commands::nextGeneral(){
	packet2_reset(mainPacket[currentFrame], 0);
	return mainPacket[currentFrame];
}

packet2_t* Commands::nextTexture(){
	packet2_reset(texturePacket[currentFrame], 0);
	return texturePacket[currentFrame];
}

packet2_t* Commands::nextIndirect(){
	packet2_reset(indirectPacket, 0);
	return indirectPacket;
}

packet2_t* Commands::nextDraw(){
	currentDraw = 1 - currentDraw;
	packet2_reset(vu1Packets[currentDraw], 0);
	return vu1Packets[currentDraw];
}

void Commands::clean(){
	for(int i = 0; i < 2; ++i){
		packet2_free(mainPacket[i]);
		packet2_free(texturePacket[i]);
		packet2_free(vu1Packets[i]);
	}
	packet2_free(indirectPacket);
}

