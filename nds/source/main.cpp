
#include <nds.h>
#include <stdio.h>

#define LIGHTX (-0.57)
#define LIGHTY (-0.57)
#define LIGHTZ (-0.57)
#define CUBESHIFT 5.0

#include "plane_and_skybox_geometry.h"
#include "skybox_color.h"
#include "plane_color.h"
#include "dragon_geometry.h"
#include "dragon_color.h"
#include "dragon_shadow_geometry.h"
#include "suzanne_geometry.h"
#include "suzanne_color.h"

static void getKeysDelta( int *dx, int *dy, int *du ) {
	
	static int prev_pen[2] = { 0x7FFFFFFF, 0x7FFFFFFF };
	touchPosition touchXY;
	
	u32 keys = keysHeld();
	
	if( keys & KEY_TOUCH ) {
		
		touchRead(&touchXY);
		
		if( prev_pen[0] != 0x7FFFFFFF ) {
			*dx = (prev_pen[0] - touchXY.rawx);
			*dy = (prev_pen[1] - touchXY.rawy);
		} else {
			*dx = *dy = 0;
		}
		
		prev_pen[0] = touchXY.rawx;
		prev_pen[1] = touchXY.rawy;
	} else {
		prev_pen[0] = prev_pen[1] = 0x7FFFFFFF;
		*dx = *dy = 0;
	}
	
	*du = 0;
	if ( keys & KEY_UP ){
		*du += 1;
	} else if ( keys & KEY_DOWN ){
		*du -= 1;
	}
}

int textureIDSkybox;
int textureIDPlane;
int textureIDSuzanne;
int textureIDDragon;

static void loadTextures(){
	// Skybox.
	glGenTextures(1, &textureIDSkybox);
	glBindTexture(0, textureIDSkybox);
	glTexImage2D(0, 0, GL_RGB256, TEXTURE_SIZE_512 , TEXTURE_SIZE_256, 0, TEXGEN_OFF, (u8*)skybox_color);
	glColorTableEXT( 0, 0, 256, 0, 0, (u16*)skybox_color_palette );
	
	// Plane.
	glGenTextures(1, &textureIDPlane);
	glBindTexture(0, textureIDPlane);
	glTexImage2D(0, 0, GL_RGB16, TEXTURE_SIZE_512 , TEXTURE_SIZE_512, 0, TEXGEN_OFF, (u8*)plane_color);
	glColorTableEXT( 0, 0, 16, 0, 0, (u16*)plane_color_palette );
	
	// Suzanne.
	glGenTextures(1, &textureIDSuzanne);
	glBindTexture(0, textureIDSuzanne);
	glTexImage2D(0, 0, GL_RGB16, TEXTURE_SIZE_512 , TEXTURE_SIZE_512, 0, TEXGEN_OFF, (u8*)suzanne_color);
	glColorTableEXT( 0, 0, 16, 0, 0, (u16*)suzanne_color_palette );
	
	// Dragon.
	glGenTextures(1, &textureIDDragon);
	glBindTexture(0, textureIDDragon);
	glTexImage2D(0, 0, GL_RGB16, TEXTURE_SIZE_512 , TEXTURE_SIZE_512, 0, TEXGEN_OFF, (u8*)dragon_color);
	glColorTableEXT( 0, 0, 16, 0, 0, (u16*)dragon_color_palette );
}


int main(void) {
	
	// Video mode (3D).
	videoSetMode(MODE_0_3D);
	
	// GL init.
	glInit();
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ANTIALIAS);
	glEnable(GL_BLEND);
	glClearColor(0,15,15,31);
	glClearPolyID(63);
	glClearDepth(0x7FFF);
	
	// VRAM banks init.
	vramSetBankA(VRAM_A_TEXTURE_SLOT0);
	vramSetBankB(VRAM_B_TEXTURE_SLOT1);
	vramSetBankC(VRAM_C_TEXTURE_SLOT2);
	vramSetBankD(VRAM_D_TEXTURE_SLOT3);
	vramSetBankF(VRAM_F_TEX_PALETTE_SLOT0);
	vramSetBankG(VRAM_G_TEX_PALETTE_SLOT4);
	
	loadTextures();
	
	// Viewport and projection matrix.
	glViewport(0,0,255,191);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, 256.0 / 192.0, 0.1, 100);
	
	int rotateX = 0;
	int rotateY = 0;
	int translate = -30;
	int timer = 0;
	while(1) {
		
		// Inputs.
		scanKeys();
		int pen_delta[2];
		int du;
		getKeysDelta( &pen_delta[0], &pen_delta[1], &du );
		rotateY -= 2*pen_delta[0];
		rotateX -= 2*pen_delta[1];
		translate += du;
		
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK |  POLY_FORMAT_LIGHT0);
		
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		
		//Light
		glLoadIdentity();
		glRotateXi(rotateX);
		glRotateYi(rotateY);
		glLight(0, RGB15(31,31,31) , floattov10(LIGHTX),floattov10(LIGHTY),floattov10(LIGHTZ));
		glMaterialf(GL_AMBIENT, RGB15(8,8,8));
		glMaterialf(GL_DIFFUSE, RGB15(28,28,28));
		
		// Skybox.
		glLoadIdentity();
		glTranslatef(0.0f,0.0f,0.1*float(translate));
		glRotateXi(rotateX);
		glRotateYi(rotateY);
		glBindTexture(0, textureIDSkybox);
		glCallList((u32*)skybox_commands);
		
		glPushMatrix();
		// Plane.
		glTranslatef(-0.0f,-0.5f,-0.0f);
		glBindTexture(0, textureIDPlane);
		glCallList((u32*)plane_commands);
		
		// Dragon.
		glTranslatef(-0.2f,0.55f,-0.2f);
		glBindTexture(0, textureIDDragon);
		glCallList((u32*)dragon_commands);
		glPopMatrix(1);
		glPushMatrix();
		// Suzanne (animated).
		
		glTranslatef(0.3f,0.25f,0.3f);
		glScalef(0.7,0.7,0.7);
		glPushMatrix();
		glRotateYi(timer);
		glBindTexture(0, textureIDSuzanne);
		glCallList((u32*)suzanne_commands);
		
		//glPopMatrix(1);
		glBindTexture(0, 0);
		glPopMatrix(1);
		glPushMatrix();
		glTranslatef(-0.7f,-1.2f,-0.7f);
		//glRotateXi(-8000);
		//glRotateZi(4000);
		glRotateYi(timer);
		glScalef(1.5,2.0,1.5);
		
		glPolyFmt(POLY_ALPHA(1) | POLY_CULL_NONE | POLY_ID(0) | POLY_SHADOW);
		glCallList((u32*)suzanne_commands);
		glPolyFmt(POLY_ALPHA(15) | POLY_CULL_BACK | POLY_ID(61) | POLY_SHADOW);
		glCallList((u32*)suzanne_commands);
		
		//glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK |  POLY_FORMAT_LIGHT0);

		glPopMatrix(1);
		glPushMatrix();
		glTranslatef(-0.70f,0.0f,-0.70f);
		//glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT |  POLY_FORMAT_LIGHT0);
		//glCallList((u32*)dragon_shadow_commands);
		glPolyFmt(POLY_ALPHA(15) | POLY_CULL_NONE | POLY_ID(0) | POLY_SHADOW);
		glCallList((u32*)dragon_shadow_commands);
		glPolyFmt(POLY_ALPHA(15) | POLY_CULL_BACK | POLY_ID(61) | POLY_SHADOW);
		glCallList((u32*)dragon_shadow_commands);
		glPopMatrix(1);
		
		glFlush(GL_TRANS_MANUALSORT | GL_WBUFFERING);
		
		timer += 40;
	}
	
	return 0;

}
