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
#include <packet.h>

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
	z.address = graph_vram_allocate(frames[0].width,frames[0].height,z.zsm, GRAPH_ALIGN_PAGE);
	
	// Set video mode.
	graph_set_mode(GRAPH_MODE_INTERLACED, graph_get_region(), GRAPH_MODE_FIELD, GRAPH_DISABLE);
	graph_set_screen(0, 0, frames[0].width, frames[0].height);
	graph_set_bgcolor(0, 0, 0);
	graph_enable_output();
	// Initialize the screen and tie the first framebuffer to the read circuits.
	graph_set_framebuffer_filtered(frames[0].address, frames[0].width, frames[0].psm, 0, 0);
	graph_enable_output();
	// Create initial setup packet.
	packet_t *packet = packet_init(16,PACKET_NORMAL);
	// This is our generic qword pointer.
	qword_t *q0 = packet->data;
	// This will setup a default drawing environment.
	q0 = draw_setup_environment(q0,0,&frames[0],&z);
	// Now reset the primitive origin to 2048-width/2,2048-height/2.
	q0 = draw_primitive_xyoffset(q0,0,(2048-frames[0].width/2),(2048-frames[0].height/2));
	// Finish setting up the environment.
	q0 = draw_finish(q0);
	// Now send the packet, no need to wait since it's the first.
	dma_channel_send_normal(DMA_CHANNEL_GIF,packet->data,q0 - packet->data, 0, 0);
	dma_wait_fast();
	packet_free(packet);

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
	
	// Init scene avec graphic setup.
	Scene scene(framebuffers[0].width, framebuffers[0].height, &z);
	
	// The data packets for double buffering dma sends.
	packet_t *generalPackets[2];
	packet_t *generalCurrent;
	packet_t *texturePackets[2];
	packet_t *textureCurrent;
	packet_t *flipBuffersPacket;
	
	generalPackets[0] = packet_init(65535,PACKET_NORMAL);
	generalPackets[1] = packet_init(65535,PACKET_NORMAL);
	texturePackets[0] = packet_init(128,PACKET_NORMAL);
	texturePackets[1] = packet_init(128,PACKET_NORMAL);
	flipBuffersPacket = packet_init(8, PACKET_UCAB);
	int context = 0;
	
	for(;;){
		// Read gamepad.
		pad.update();

		// Current packets.
		generalCurrent = generalPackets[context];
		textureCurrent = texturePackets[context];
		// Render.
		scene.update(pad);
		scene.clear(generalCurrent);
		scene.render(generalCurrent, textureCurrent);
		
		// Wait for scene to finish drawing
		draw_wait_finish();
		graph_wait_vsync();
		
		// Prepare framebuffer.
		graph_set_framebuffer_filtered(framebuffers[context].address, framebuffers[context].width, framebuffers[context].psm, 0, 0);
		
		//Switch context.
		context ^= 1;
		
		// Flip framebuffer.
		qword_t * q = flipBuffersPacket->data;
		q++;
		q = draw_framebuffer(q, 0, &(framebuffers[context]));
		q = draw_finish(q);
		dma_wait_fast();
		dma_channel_send_normal_ucab(DMA_CHANNEL_GIF, flipBuffersPacket->data, q - flipBuffersPacket->data, 0);
		draw_wait_finish();
	}

	SleepThread();
	return 0;
}
