; Based on the VU1 sample in PS2SDK.

.syntax new
.name VU1SkyboxProgram
.vu
.init_vf_all
.init_vi_all

--enter
--endenter

	; Load constant data
	
	lq.xyz	vpScale,		0(vi00) 		; X,Y,Z viewport/depth range as floats
	lq.w    rgba,           0(vi00)			; W default alpha
	lq		gifSetTag,		1(vi00) 		; Register GIF: set
	lq		gifLodTag,		2(vi00) 		; Register GIF: lod (same for all textures)

	; Reset the CLIP flags (mandatory)
	fcset	0x000000

	; Toggle double-buffered data offset.
	xtop	iBase

	; Load buffered params
	lq.xyz	rgba,			0(iBase) 		; RGB color
	ilw.w	vertCount,		0(iBase)		; vertex count
	lq		gifTexTag,		1(iBase) 		; Register GIF: texture & CLUT
	lq		gifPrimTag,		2(iBase) 		; Register GIF: primitive setup
	
	; Load transformation matrix
	lq		mvpRow0,		3(iBase)
	lq		mvpRow1,		4(iBase)
	lq		mvpRow2,		5(iBase)
	;lq		mvpRow3,		6(iBase) 		; Not needed (don't translate skybox)

	; Input data pointers
	iaddiu	vertexData,		iBase,			7			; start of vertex data
	iadd	stqData,		vertexData,		vertCount	; pointer to stq

	; Output pointers
	iadd	kickAddress,	stqData,		vertCount	; pointer for XGKICK
	iadd	destAddress,	stqData,		vertCount	; output data
	
	; Write packets
	; Header with GS setup.
	sqi		gifSetTag,		(destAddress++) ; Number of kick packets
	sqi		gifLodTag,		(destAddress++) ; LOD params
	sqi		gifSetTag,		(destAddress++) ; .
	sqi		gifTexTag,		(destAddress++) ; Texture params
	sqi		gifPrimTag,		(destAddress++) ; Primitive params

	; Loop over vertices
	iaddiu	vertCounter,	vertCount,		0	; counter
	vertLoop:
	; {
		; Load vertex attributes
		lq			vertex,			0(vertexData)
		lq			stq,			0(stqData)

		; Vertex transformation
		mul			acc,			mvpRow0,		vertex[x]
		madd		acc,			mvpRow1,		vertex[y]
		madd		acc,			mvpRow2,		vertex[z]
		; No translation for skybox
		madd		vertex,			vf00,			vf00

		; Clipping routine from the sample.
		clipw.xyz	vertex,			vertex ; test 6 clipping planes
		fcand		VI01,			0x3FFFF ; Test the last 3*6 bits, VI01 will be set if at least one non-zero.
		; To clip a vertex, we want to output to XYZ3 instead of XYZ2 (and that won't draw kick)
		; To achieve this, we can set bit 111 of XYZ, ie bit 15 of its W component. 
		; W is otherwise unused. We can write ((1 << 15) - 1) + VI01 to W.
		; This will set bit 15 to 0 if VI01 is zero, and to 1 if VI01 is 1.
		iaddiu		iClipBit,		VI01,			0x7FFF
		; Write to W component of vertex packet.
		isw.w		iClipBit,		2(destAddress)
		
		; Finish vertex transformation
		div			q,				vf00[w],		vertex[w] 	; Compute Q=1/v.w (vf00=0,0,0,1)
		mul.xyz		vertex,			vertex,			q			; Perspective division
		mula.xyz	acc,			vpScale,		vf00[w]		; Store scale in acc
		madd.xyz	vertex,			vertex,			vpScale		; Scale/offset vertex to viewport and depth range.
		ftoi4.xy	vertex,			vertex						; Convert to 12:4 fixed point.
		ftoi0.z		vertex,			vertex						; Convert to integer.
		
		; Finish ST transformation (premultiply by Q, and set Q in Z component).
		mulq		stq,			stq,			Q
		
		; Store results
		sq			stq,			0(destAddress)
		sq			rgba,			1(destAddress)
		sq.xyz		vertex,			2(destAddress)
		
		; Next vertex
		iaddiu		vertexData,		vertexData,		1
		iaddiu		stqData,		stqData,		1
		; Next output
		iaddiu		destAddress,	destAddress,	3

		; Loop
		iaddi		vertCounter,	vertCounter,	-1
		ibgtz		vertCounter,	vertLoop

	; }

	--barrier

	xgkick kickAddress ; kick to rasterizer

--exit
--endexit
