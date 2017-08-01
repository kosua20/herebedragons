/// Global objects.

var gl, canvas;
var shadows; // Will contain all objects related to the shadow map generation and processing.
var previousClick = { clientX: 0,  clientY: 0 }; // Used for mouse interactions.

// Scene objects.

var cubeMapObject, planeObject, dragonObject, suzanneObject; 

var camera = {
	verticalAngle: 0.4, 
	horizontalAngle: 0.7,
	distance: 2.0,
	view: mat4.create(),
	projection: mat4.create() 
};

var light = { 
	directionWorld: vec4.fromValues(1.0,1.0,1.0,0.0),
	directionView: vec4.fromValues(0.0,0.0,0.0,0.0),
	view: mat4.create(),
	projection: mat4.create()
};


/// Initialization.

function start(meshes){
	
	// Events.
	document.onmousedown = clickDown;
	document.onmouseup = clickUp;
	document.onkeydown = keyDown;
	
	// Canvas size setup.
	canvas = document.getElementById("gl-canvas");
	// Update canvas size using client width.
	canvas.width = canvas.clientWidth;
	canvas.height = canvas.clientHeight;
	
	gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
	if(!gl){ alert("Unable to get GL context."); return;}
	
	// Viewport and options.
	gl.viewport(0, 0, gl.drawingBufferWidth,gl.drawingBufferHeight);
	gl.clearColor(0.0,0.0,0.0,1.0);
	gl.enable(gl.DEPTH_TEST);
	gl.depthFunc(gl.LESS);
	gl.enable(gl.CULL_FACE);
	gl.cullFace(gl.BACK);
	gl.frontFace(gl.CCW);

	// Camera and light setup.
	mat4.perspective(camera.projection, 1.22, canvas.clientWidth/canvas.clientHeight, 0.01, 100.0);
	mat4.ortho(light.projection, -1.5,1.5,-1.5,1.5,0.1,10.0);
	mat4.lookAt(light.view, vec3.fromValues(1.0,1.0,1.0), vec3.fromValues(0.0,0.0,0.0), vec3.fromValues(0.0,1.0,0.0));

	shadows = { width: 512, height: 512};
	// Framebuffers for the shadow map.
	shadows.depthBuffer = initFramebuffer(shadows.width, shadows.height);
	shadows.blurBuffer = initFramebuffer(shadows.width, shadows.height);
	// Full screen quad for the blur pass.
	shadows.blurPass = initBlur(shadows.depthBuffer.textureId);
	shadows.mapId = shadows.blurBuffer.textureId;

	// Objects.
	cubeMapObject = initCubeMap(meshes.cube);
	planeObject = initObject(meshes.plane, "plane", 25.0, shadows.mapId);
	suzanneObject = initObject(meshes.suzanne, "suzanne", 10.0, shadows.mapId);
	dragonObject = initObject(meshes.dragon, "dragon", 128.0, shadows.mapId);

	drawScene(0);
	
}


/// Update and render loop.

function updateScene(time){
	// Start by resizing the canvas and the proj matrix if needed.
	if(canvas.width != canvas.clientWidth || canvas.height != canvas.clientHeight){
		canvas.width = canvas.clientWidth;
		canvas.height = canvas.clientHeight;
		mat4.perspective(camera.projection, 1.22, canvas.clientWidth/canvas.clientHeight, 0.01, 100.0);
	}

	// Update view matrix.
	mat4.lookAt(camera.view, vec3.fromValues(camera.distance*Math.cos(camera.horizontalAngle)*Math.cos(camera.verticalAngle), 
											 camera.distance*Math.sin(camera.verticalAngle), 
											 camera.distance*Math.sin(camera.horizontalAngle)*Math.cos(camera.verticalAngle)),
							vec3.fromValues(0.0,0.0,0.0),
							vec3.fromValues(0.0,1.0,0.0));
	// Update light view space direction.
	vec4.transformMat4(light.directionView, light.directionWorld, camera.view);
	
	// Update models matrices.
	// Dragon.
	mat4.fromScaling(dragonObject.model, vec3.fromValues(0.32,0.32,0.32));
	mat4.translate(dragonObject.model,dragonObject.model,vec3.fromValues(-0.5,1.4,-0.5));
	// Monkey head.
	mat4.fromScaling(suzanneObject.model, vec3.fromValues(0.3,0.3,0.3));
	mat4.translate(suzanneObject.model,suzanneObject.model,vec3.fromValues(0.7,1.2,0.9));
	mat4.rotateY(suzanneObject.model,suzanneObject.model,0.0017*time);
	// The plane has the identity matrix as model matrix.
}

function drawScene(time){
	
	// Update camera and models matrices.
	updateScene(time)

	// Shadow pass.
	gl.viewport(0, 0, shadows.width, shadows.height);
	gl.bindFramebuffer(gl.FRAMEBUFFER, shadows.depthBuffer.id);
	gl.clearColor(1.0,1.0,1.0,1.0);
	gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
	// Render objects to shadow map.
	renderObjectShadow(dragonObject);
	renderObjectShadow(suzanneObject);
	renderObjectShadow(planeObject);
	
	// Blurring (covariance) pass.
	gl.bindFramebuffer(gl.FRAMEBUFFER, shadows.blurBuffer.id);
	// Same viewport as the shadow map itself.
	gl.disable(gl.DEPTH_TEST);
	renderBlur(shadows.blurPass);
	gl.enable(gl.DEPTH_TEST);

	// Final render pass.
	gl.bindFramebuffer(gl.FRAMEBUFFER, null);
	gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
	// Render objects.
	renderObject(dragonObject);
	renderObject(suzanneObject);
	renderObject(planeObject);
	renderCubeMap(cubeMapObject);

	// Request next frame.
  	window.requestAnimationFrame(drawScene);
}


/// Object setup and rendering.

function initObject(mesh, objName, shininess, shadowTexture){

	var object = {};
	object.model = mat4.create();

	// Mesh.
	OBJ.initMeshBuffers(gl, mesh);

	// Shaders.
	object.program = initShaders(vsDefaultString, fsDefaultString);

	// Attributes.
	gl.useProgram(object.program);
	object.vertexAttribLocation = gl.getAttribLocation(object.program, "v");
	object.UVAttribLocation = gl.getAttribLocation(object.program, "u");
	object.normalAttribLocation = gl.getAttribLocation(object.program, "no");
	object.tangentAttribLocation = gl.getAttribLocation(object.program, "ta");
	object.binormalAttribLocation = gl.getAttribLocation(object.program, "bi");

	// Compute and init tangent and binormal buffers.
	mesh = computeTangentsAndBinormals(mesh);

	object.vertexBuffer = mesh.vertexBuffer;
	object.UVBuffer = mesh.textureBuffer;
	object.indexBuffer = mesh.indexBuffer;
	object.normalBuffer = mesh.normalBuffer;

	object.tangentBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, object.tangentBuffer);								
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(mesh.tangents), gl.STATIC_DRAW);

	object.binormalBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, object.binormalBuffer);								
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(mesh.binormals), gl.STATIC_DRAW);
	
	// Textures
	object.textureDiffuseId = loadTexture("textures/" + objName + ".png");
	object.textureNormalId = loadTexture("textures/" + objName + "_normal.png");
	object.textureShadowId = shadowTexture;
	gl.uniform1i(gl.getUniformLocation(object.program, "texture"), 0);
	gl.uniform1i(gl.getUniformLocation(object.program, "textureNormal"), 1);
	gl.uniform1i(gl.getUniformLocation(object.program, "textureShadow"),2);

	// Uniforms locations.
	object.mvpind = gl.getUniformLocation(object.program, "mv");
   	object.pind = gl.getUniformLocation(object.program, "p");
   	object.imvind = gl.getUniformLocation(object.program, "imv");
   	object.lind = gl.getUniformLocation(object.program, "lightDir");
   	object.lmvp = gl.getUniformLocation(object.program, "lightMVP");
   	// Shininess is constant.
	gl.uniform1f(gl.getUniformLocation(object.program, "shininess"), shininess);

	gl.useProgram(null);
	
	// Shadow program.
	object.programShadow = initShaders(vsShadowString, fsShadowString);;
	// Shadow pass uniform locations.
	gl.useProgram(object.programShadow);
	object.vertexAttribLocationShadow = gl.getAttribLocation(object.programShadow, "v");
	object.mvpindShadow = gl.getUniformLocation(object.programShadow, "mvp");
	gl.useProgram(null);
	
	return object;
}

function renderObject(object){

	// Final computations.
	var mv = mat4.create();
	var imv = mat4.create();
	mat4.multiply(mv, camera.view, object.model);
	mat4.invert(imv,mv);
   	mat4.transpose(imv,imv);

	gl.useProgram(object.program);

   	// Uniforms.
   	gl.uniformMatrix4fv(object.mvpind,false,mv);
   	gl.uniformMatrix4fv(object.pind,false, camera.projection);
   	gl.uniformMatrix4fv(object.imvind,false,imv);
   	gl.uniform4fv(object.lind, light.directionView);
   	gl.uniformMatrix4fv(object.lmvp,false, object.lightMVP);

	// Textures.
	gl.activeTexture(gl.TEXTURE0);
	gl.bindTexture(gl.TEXTURE_2D, object.textureDiffuseId);
	gl.activeTexture(gl.TEXTURE1);
	gl.bindTexture(gl.TEXTURE_2D, object.textureNormalId);
	gl.activeTexture(gl.TEXTURE2);
	gl.bindTexture(gl.TEXTURE_2D, object.textureShadowId);

	// Attributes.
	gl.bindBuffer(gl.ARRAY_BUFFER, object.vertexBuffer);
	gl.vertexAttribPointer(object.vertexAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(object.vertexAttribLocation);

	gl.bindBuffer(gl.ARRAY_BUFFER, object.UVBuffer);	
	gl.vertexAttribPointer(object.UVAttribLocation, 2, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(object.UVAttribLocation);

	gl.bindBuffer(gl.ARRAY_BUFFER, object.normalBuffer);	
	gl.vertexAttribPointer(object.normalAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(object.normalAttribLocation);

	gl.bindBuffer(gl.ARRAY_BUFFER, object.tangentBuffer);	
	gl.vertexAttribPointer(object.tangentAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(object.tangentAttribLocation);
	
	gl.bindBuffer(gl.ARRAY_BUFFER, object.binormalBuffer);	
	gl.vertexAttribPointer(object.binormalAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(object.binormalAttribLocation);

	// Draw.
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, object.indexBuffer);
	gl.drawElements(gl.TRIANGLES, object.indexBuffer.numItems, gl.UNSIGNED_SHORT, 0);

	// End.
  	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
  	gl.bindTexture(gl.TEXTURE_2D, null);
  	gl.useProgram(null);
}

function renderObjectShadow(object){
	// Light MVP
	var mvp = mat4.create();
	mat4.multiply(mvp, light.view, object.model);
	mat4.multiply(mvp, light.projection, mvp);
	object.lightMVP = mvp;

	gl.useProgram(object.programShadow);
	
	// Uniforms.
   	gl.uniformMatrix4fv(object.mvpindShadow,false,mvp);
	
	// Attributes.
	gl.bindBuffer(gl.ARRAY_BUFFER, object.vertexBuffer);
	gl.vertexAttribPointer(object.vertexAttribLocationShadow, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(object.vertexAttribLocationShadow);
	
	// Draw.
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, object.indexBuffer);
	gl.drawElements(gl.TRIANGLES, object.indexBuffer.numItems, gl.UNSIGNED_SHORT, 0);
  	
  	// End.
  	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
  	gl.useProgram(null);
}


/// Cubemap setup and rendering.

function initCubeMap(mesh){
	var cubemap = {};

	// Model matrix.
	cubemap.model = mat4.create();
	mat4.fromScaling(cubemap.model, vec3.fromValues(5.0,5.0,5.0));

	// Mesh.
	OBJ.initMeshBuffers(gl, mesh);
	
	//Shaders.
	cubemap.program = initShaders(vsCubemapString, fsCubemapString);
	gl.useProgram(cubemap.program);
	
	// Attributes.
	cubemap.vertexAttribLocation = gl.getAttribLocation(cubemap.program, "v");
	cubemap.vertexBuffer = mesh.vertexBuffer;
	cubemap.indexBuffer = mesh.indexBuffer;
	
	// Texture.
	cubemap.textureId = loadTextureCubeMap("textures/cubemap/cubemap"); 
	
	// Uniforms locations.
	gl.uniform1i(gl.getUniformLocation(cubemap.program, "texture"), 0);
	cubemap.mvpind = gl.getUniformLocation(cubemap.program, "mvp");

	gl.useProgram(null);

	return cubemap;
}

function renderCubeMap(cube){
	// Final computation.
	var mvp = mat4.create();
	mat4.multiply(mvp, camera.view, cube.model);
	mat4.multiply(mvp, camera.projection, mvp);

	gl.useProgram(cube.program);

	// Uniform.
	gl.uniformMatrix4fv(cube.mvpind,false,mvp);
	
	// Texture.
	gl.activeTexture(gl.TEXTURE0);
	gl.bindTexture(gl.TEXTURE_CUBE_MAP, cube.textureId);
	
	// Attributes.
	gl.bindBuffer(gl.ARRAY_BUFFER, cube.vertexBuffer);
	gl.vertexAttribPointer(cube.vertexAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(cube.vertexAttribLocation);
	
	// Draw.
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cube.indexBuffer);
	gl.drawElements(gl.TRIANGLES, cube.indexBuffer.numItems, gl.UNSIGNED_SHORT, 0);
  	
  	// End.
  	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
  	gl.bindTexture(gl.TEXTURE_CUBE_MAP, null);
  	gl.useProgram(null);
}


/// Blur pass setup and rendering.

function initBlur(texId){
	// Full screen quad.
	var vertices = [-1.0, -1.0,  1.0, -1.0, 1.0,  1.0, -1.0,  1.0];
	var indices = [ 0,1,3, 3,1,2];
	var blur = {};
	blur.program = initShaders(vsBlurString, fsBlurString);
	gl.useProgram(blur.program);
	
	blur.vertexAttribLocation = gl.getAttribLocation(blur.program, "v");
	blur.vertexBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, blur.vertexBuffer);								
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);

	blur.indexBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, blur.indexBuffer);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);

	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
	gl.bindBuffer(gl.ARRAY_BUFFER, null);

	gl.uniform1i(gl.getUniformLocation(blur.program, "texture"), 0);
	blur.screenSizeId = gl.getUniformLocation(blur.program, "screenSize");

	gl.useProgram(null);

	blur.textureId = texId;

	return blur;
}

function renderBlur(pass){
	gl.useProgram(pass.program);
	
	// Uniforms.
	gl.uniform2f(pass.screenSizeId, gl.drawingBufferWidth, gl.drawingBufferHeight);

	// Textures.
	gl.activeTexture(gl.TEXTURE0);
	gl.bindTexture(gl.TEXTURE_2D, pass.textureId);

	// Attributes.
	gl.bindBuffer(gl.ARRAY_BUFFER, pass.vertexBuffer);
	gl.vertexAttribPointer(pass.vertexAttribLocation, 2, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(pass.vertexAttribLocation);

	// Draw.
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, pass.indexBuffer);
	gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);
  	
  	// End.
  	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
  	gl.bindTexture(gl.TEXTURE_2D, null);
  	gl.useProgram(null);
}


/// Interaction events.

function keyDown(event){
	if (event.keyCode == 90){
		camera.distance = Math.max(0.1, camera.distance - 0.05);
	} else if (event.keyCode == 83){
		camera.distance += 0.05;
	}
}

function clickDown(event){
	canvas.onmousemove = moveMouse;
	previousClick.clientX = event.clientX;
	previousClick.clientY = event.clientY;
}

function clickUp(event){
	canvas.onmousemove = null;
}

function moveMouse(event){
	camera.horizontalAngle += (event.clientX - previousClick.clientX)*0.01;
	camera.verticalAngle += (event.clientY - previousClick.clientY)*0.01;
	camera.verticalAngle = Math.min(Math.max(camera.verticalAngle, -1.57), 1.57);
	previousClick.clientX = event.clientX;
	previousClick.clientY = event.clientY;
}


/// On load.
// Download meshes and start the init + render loop.

window.onload = function(){
	OBJ.downloadMeshes({ 'dragon': 'models/dragon.obj', 
						 'suzanne': 'models/suzanne.obj',
						 'plane': 'models/plane.obj',
						 'cube': 'models/cube.obj' }, start);
}

