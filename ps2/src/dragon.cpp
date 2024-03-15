#include <kernel.h>
#include <tamtypes.h>
#include <math3d.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <packet2.h>

// C/C++ standard libraries:
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <debug.h>
#include <libpad.h>

#include "Pad.hpp"
#include "Scene.hpp"


void setupContext(framebuffer_t * frames, zbuffer_t & z){
	
	graph_vram_clear();
	
	// Init GIF dma channel.
	dma_channel_initialize(DMA_CHANNEL_GIF,NULL,0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);
	
	// Setup the framebuffer.
	frames[0].mask    = 0;
	frames[0].psm     = GS_PSM_16;
	frames[0].address = graph_vram_allocate(frames[0].width, frames[0].height, frames[0].psm, GRAPH_ALIGN_PAGE);
	
	frames[1].mask    = 0;
	frames[1].psm     = GS_PSM_16;
	frames[1].address = graph_vram_allocate(frames[1].width, frames[1].height, frames[1].psm, GRAPH_ALIGN_PAGE);
	// Setup the zbuffer.
	z.enable = DRAW_ENABLE;
	z.mask = 0;
	z.method = ZTEST_METHOD_GREATER_EQUAL;
	z.zsm = GS_ZBUF_16;
	z.address = graph_vram_allocate(frames[0].width,frames[0].height, z.zsm, GRAPH_ALIGN_PAGE);
	
	// Set video mode.
	graph_set_mode(GRAPH_MODE_INTERLACED, graph_get_region(), GRAPH_MODE_FIELD, GRAPH_DISABLE);
	graph_set_screen(0, 0, frames[0].width, frames[0].height);
	graph_set_bgcolor(0, 0, 0);
	graph_enable_output();
	// Initialize the screen and tie the first framebuffer to the read circuits.
	graph_set_framebuffer_filtered(frames[0].address, frames[0].width, frames[0].psm, 0, 0);
	graph_enable_output();

	// Create initial setup packet.
	packet2_t *p = packet2_create(20, P2_TYPE_NORMAL, P2_MODE_NORMAL, 0);
	// This will setup a default drawing environment.
	packet2_update(p, draw_setup_environment(p->next, 0, &frames[0],&z));
	// Now reset the primitive origin to 2048-width/2,2048-height/2.
	packet2_update(p, draw_primitive_xyoffset(p->next,0,(2048-frames[0].width/2),(2048-frames[0].height/2)));
	// Finish setting up the environment.
	packet2_update(p, draw_finish(p->next));

	// Now send the packet, no need to wait since it's the first.
	dma_channel_send_packet2(p, DMA_CHANNEL_GIF, 0);
	dma_wait_fast();
	packet2_free(p);
}

int main(){
	
	SifInitRpc(0);
	// Init pad immediatly.
	Pad pad;
	if(!pad.setup()){
		printf("Error setting up the pad.\n");
	}
	
	zbuffer_t z;
	framebuffer_t  framebuffers[2];
	framebuffers[0].width = 640;
	framebuffers[0].height = 448;
	framebuffers[1].width = 640;
	framebuffers[1].height = 448;
	
	
	setupContext(framebuffers, z);
	
	// Init scene and graphic setup.
	Scene scene(framebuffers[0].width, framebuffers[0].height);
	
	// The data packets for double buffering dma sends.
	packet2_t *generalPackets[2];
	packet2_t *generalCurrent;
	packet2_t *texturePackets[2];
	packet2_t *textureCurrent;
	packet2_t *flipBuffersPacket;
	
	generalPackets[0] = packet2_create(65535, P2_TYPE_NORMAL, P2_MODE_CHAIN, 0);
	generalPackets[1] = packet2_create(65535, P2_TYPE_NORMAL, P2_MODE_CHAIN, 0);
	texturePackets[0] = packet2_create(1024, P2_TYPE_NORMAL, P2_MODE_CHAIN, 0);
	texturePackets[1] = packet2_create(1024, P2_TYPE_NORMAL, P2_MODE_CHAIN, 0);
	flipBuffersPacket = packet2_create(8, P2_TYPE_UNCACHED_ACCL, P2_MODE_NORMAL, 0);
	int context = 0;
	
	for(;;){
		// Read gamepad.
		pad.update();

		// Current packets.
		generalCurrent = generalPackets[context];
		textureCurrent = texturePackets[context];
		packet2_reset(generalCurrent, 0);
		packet2_reset(textureCurrent, 0);
		packet2_reset(flipBuffersPacket, 0);

		// Render.
		scene.update(pad);
		scene.clear(generalCurrent, &z);
		scene.render(generalCurrent, textureCurrent);
		
		// Wait for scene to finish drawing
		draw_wait_finish();
		graph_wait_vsync();
		
		// Prepare framebuffer.
		graph_set_framebuffer_filtered(framebuffers[context].address, framebuffers[context].width, framebuffers[context].psm, 0, 0);
		
		//Switch context.
		context ^= 1;
		
		// Flip framebuffer.
		// This will setup a default drawing environment.
		packet2_update(flipBuffersPacket, draw_framebuffer(flipBuffersPacket->next, 0, &(framebuffers[context])));
		packet2_update(flipBuffersPacket, draw_finish(flipBuffersPacket->next));
		
		dma_wait_fast();
		dma_channel_send_packet2(flipBuffersPacket, DMA_CHANNEL_GIF, 0);
		draw_wait_finish();
	}

	packet2_free(generalPackets[0]);
	packet2_free(generalPackets[1]);
	packet2_free(texturePackets[0]);
	packet2_free(texturePackets[1]);
	packet2_free(flipBuffersPacket);

	SleepThread();
	return 0;
}
