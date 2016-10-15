var gl;
var cubeMapProgram, planeProgram, dragonProgram, suzanneProgram, blurProgram;
var shadowFramebuffer, blurFramebuffer;
var projectionMatrix, lightProjectionMatrix;



function start(){
	canvas = document.getElementById("glcanvas");
	initWebGL(canvas);
	if(!gl){ return; }
	
	gl.clearColor(0.0,0.0,0.0,1.0);
	gl.enable(gl.DEPTH_TEST);
	gl.depthFunc(gl.LEQUAL);
	gl.disable(gl.CULL_FACE);
	gl.cullFace(gl.BACK);
	gl.frontFace(gl.CCW);

	projectionMatrix = mat4.create();
	mat4.perspective(projectionMatrix, 1.22, canvas.width / canvas.height, 0.01, 100.0);
	lightProjectionMatrix = mat4.create();
	mat4.ortho(lightProjectionMatrix,-1.5,1.5,-1.5,1.5,0.1,10.0);

	shadowFramebuffer = initFramebuffer();
	blurFramebuffer = initFramebuffer();

	
	initCubeMap();
	initBlur();

	blurProgram.textureId = shadowFramebuffer.textureId;
	
	planeProgram = initObject("plane");
	suzanneProgram = initObject("suzanne");
	dragonProgram = initObject("dragon");


	planeProgram.shadowTexture = blurFramebuffer.textureId;
	suzanneProgram.shadowTexture = blurFramebuffer.textureId;
	dragonProgram.shadowTexture = blurFramebuffer.textureId;

	planeProgram.shininess = 25.0;
	suzanneProgram.shininess = 10.0;
	dragonProgram.shininess = 50.0;


	drawScene(0);
	
}

function initFramebuffer(){
	var framebuffer = gl.createFramebuffer();
	gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);
    framebuffer.width = 512;
    framebuffer.height = 512;

    var localTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, localTexture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
 	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
 	

    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, framebuffer.width, framebuffer.height, 0, gl.RGB, gl.UNSIGNED_BYTE, null);
   	
    var renderbuffer = gl.createRenderbuffer();
    gl.bindRenderbuffer(gl.RENDERBUFFER, renderbuffer);
    gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT16, framebuffer.width, framebuffer.height);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, localTexture, 0);
    gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, renderbuffer);

    gl.bindTexture(gl.TEXTURE_2D, null);
    gl.bindRenderbuffer(gl.RENDERBUFFER, null);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);

    framebuffer.textureId = localTexture;
    return framebuffer;
}

function drawScene(time){

	var viewMatrix = mat4.create();
	mat4.lookAt(viewMatrix,vec3.fromValues(2.0*Math.sin(0.0005*time),1.0,2.0*Math.cos(0.0005*time)),vec3.fromValues(0.0,0.0,0.0),vec3.fromValues(0.0,1.0,0.0));

	var lightDir4 = vec4.fromValues(1.0,1.0,1.0,0.0);
	vec4.transformMat4(lightDir4, lightDir4, viewMatrix);
	var lightDir = vec3.fromValues(lightDir4[0],lightDir4[1],lightDir4[2]);

	var lightViewMatrix = mat4.create();
	mat4.lookAt(lightViewMatrix,vec3.fromValues(1.0,1.0,1.0),vec3.fromValues(0.0,0.0,0.0),vec3.fromValues(0.0,1.0,0.0));

	var modelMatrixDragon = mat4.create();
	mat4.fromScaling(modelMatrixDragon, vec3.fromValues(0.32,0.32,0.32));
	mat4.translate(modelMatrixDragon,modelMatrixDragon,vec3.fromValues(-0.5,1.4,-0.5));

	var modelMatrixSuzanne = mat4.create();
	mat4.fromScaling(modelMatrixSuzanne, vec3.fromValues(0.3,0.3,0.3));
	mat4.translate(modelMatrixSuzanne,modelMatrixSuzanne,vec3.fromValues(0.7,1.2,0.9));
	mat4.rotateY(modelMatrixSuzanne,modelMatrixSuzanne,0.0017*time);

	var modelMatrixPlane = mat4.create();

	gl.bindFramebuffer(gl.FRAMEBUFFER, shadowFramebuffer);
	gl.viewport(0, 0, shadowFramebuffer.width, shadowFramebuffer.height);
	gl.clearColor(1.0,1.0,1.0,1.0);
	gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
	
	renderObjectShadow(dragonProgram, modelMatrixDragon, lightViewMatrix, lightProjectionMatrix);
	renderObjectShadow(suzanneProgram, modelMatrixSuzanne, lightViewMatrix, lightProjectionMatrix);
	renderObjectShadow(planeProgram, modelMatrixPlane, lightViewMatrix, lightProjectionMatrix);
	
	gl.bindFramebuffer(gl.FRAMEBUFFER, blurFramebuffer);
	gl.disable(gl.DEPTH_TEST);
	renderBlur();
	gl.enable(gl.DEPTH_TEST);
	gl.bindFramebuffer(gl.FRAMEBUFFER, null);
	//gl.viewport(0, 0, shadowFramebuffer.width, shadowFramebuffer.height);
	gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
	
	
	renderObject(dragonProgram, modelMatrixDragon, viewMatrix, projectionMatrix,lightDir);
	renderObject(suzanneProgram, modelMatrixSuzanne, viewMatrix, projectionMatrix,lightDir);
	renderObject(planeProgram, modelMatrixPlane, viewMatrix, projectionMatrix,lightDir);

	renderCubeMap(viewMatrix, projectionMatrix);


  	window.requestAnimationFrame(drawScene);
}




function initObject(objName){

	var stringObj = document.getElementById(objName + "Model");
	var mesh = new OBJ.Mesh(stringObj.innerHTML);
	OBJ.initMeshBuffers(gl, mesh);

	//Shaders
	var objectProgram = initShaders("vs-default", "fs-default");

	// Attributes
	gl.useProgram(objectProgram);
	objectProgram.vertexAttribLocation = gl.getAttribLocation(objectProgram, "v");
	objectProgram.UVAttribLocation = gl.getAttribLocation(objectProgram, "u");
	objectProgram.normalAttribLocation = gl.getAttribLocation(objectProgram, "no");
	objectProgram.tangentAttribLocation = gl.getAttribLocation(objectProgram, "ta");
	objectProgram.binormalAttribLocation = gl.getAttribLocation(objectProgram, "bi");

	//Compute and init tangent and binormal buffers.
	mesh = computeTangentsAndBinormals(mesh);

	objectProgram.vertexBuffer = mesh.vertexBuffer;
	objectProgram.UVBuffer = mesh.textureBuffer;
	objectProgram.indexBuffer = mesh.indexBuffer;
	objectProgram.normalBuffer = mesh.normalBuffer;

	objectProgram.tangentBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, objectProgram.tangentBuffer);								
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(mesh.tangents), gl.STATIC_DRAW);

	objectProgram.binormalBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, objectProgram.binormalBuffer);								
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(mesh.binormals), gl.STATIC_DRAW);
	
	// Textures
	objectProgram.textureId = loadTexture("textures/" + objName + ".png");
	objectProgram.textureNormalId = loadTexture("textures/" + objName + "_normal.png");
	
	gl.useProgram(null);

	var shadowProgram = initShaders("vs-shadow", "fs-shadow");

	gl.useProgram(shadowProgram);
	shadowProgram.vertexAttribLocation = gl.getAttribLocation(shadowProgram, "v");
	gl.useProgram(null);

	objectProgram.shadowProgram = shadowProgram;
	return objectProgram;
}

function renderObject(objectProgram, modelMatrix, viewMatrix, projectionMatrix, lightDir){
	gl.useProgram(objectProgram);

	gl.activeTexture(gl.TEXTURE0);
	gl.bindTexture(gl.TEXTURE_2D, objectProgram.textureId);
	gl.uniform1i(gl.getUniformLocation(objectProgram, "texture"), 0);

	gl.activeTexture(gl.TEXTURE1);
	gl.bindTexture(gl.TEXTURE_2D, objectProgram.textureNormalId);
	gl.uniform1i(gl.getUniformLocation(objectProgram, "textureNormal"), 1);

	gl.activeTexture(gl.TEXTURE2);
	gl.bindTexture(gl.TEXTURE_2D, objectProgram.shadowTexture);
	gl.uniform1i(gl.getUniformLocation(objectProgram, "textureShadow"),2);
	
	var mv = mat4.create();
	var imv = mat4.create();
	mat4.multiply(mv, viewMatrix, modelMatrix);
	mat4.invert(imv,mv);
   	mat4.transpose(imv,imv);

	var mvpind = gl.getUniformLocation(objectProgram, "mv");
   	gl.uniformMatrix4fv(mvpind,false,mv);

   	var pind = gl.getUniformLocation(objectProgram, "p");
   	gl.uniformMatrix4fv(pind,false,projectionMatrix);

   	var imvind = gl.getUniformLocation(objectProgram, "imv");
   	gl.uniformMatrix4fv(imvind,false,imv);

   	var lind = gl.getUniformLocation(objectProgram, "lightDir");
   	gl.uniform3fv(lind,lightDir);

   	var lmvp = gl.getUniformLocation(objectProgram, "lightMVP");
   	gl.uniformMatrix4fv(lmvp,false,objectProgram.lightMVP);
	
	var sid = gl.getUniformLocation(objectProgram, "shininess");

	gl.uniform1f(sid,objectProgram.shininess);

	gl.bindBuffer(gl.ARRAY_BUFFER, objectProgram.vertexBuffer);
	gl.vertexAttribPointer(objectProgram.vertexAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(objectProgram.vertexAttribLocation);

	gl.bindBuffer(gl.ARRAY_BUFFER, objectProgram.UVBuffer);	
	gl.vertexAttribPointer(objectProgram.UVAttribLocation, 2, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(objectProgram.UVAttribLocation);

	gl.bindBuffer(gl.ARRAY_BUFFER, objectProgram.normalBuffer);	
	gl.vertexAttribPointer(objectProgram.normalAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(objectProgram.normalAttribLocation);

	gl.bindBuffer(gl.ARRAY_BUFFER, objectProgram.tangentBuffer);	
	gl.vertexAttribPointer(objectProgram.tangentAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(objectProgram.tangentAttribLocation);
	
	gl.bindBuffer(gl.ARRAY_BUFFER, objectProgram.binormalBuffer);	
	gl.vertexAttribPointer(objectProgram.binormalAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(objectProgram.binormalAttribLocation);


	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, objectProgram.indexBuffer);
	gl.drawElements(gl.TRIANGLES, objectProgram.indexBuffer.numItems, gl.UNSIGNED_SHORT, 0);
  	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
  	gl.bindTexture(gl.TEXTURE_2D, null);
  	gl.useProgram(null);
}

function renderObjectShadow(objectProgram, modelMatrix, viewMatrix, projectionMatrix){
	gl.useProgram(objectProgram.shadowProgram);
	
	var mv = mat4.create();
	var mvp = mat4.create();
	
	mat4.multiply(mv, viewMatrix, modelMatrix);
	mat4.multiply(mvp, projectionMatrix, mv);
	
	objectProgram.lightMVP = mvp;

	var mvpind = gl.getUniformLocation(objectProgram.shadowProgram, "mvp");
   	gl.uniformMatrix4fv(mvpind,false,mvp);
	
	gl.bindBuffer(gl.ARRAY_BUFFER, objectProgram.vertexBuffer);
	gl.vertexAttribPointer(objectProgram.shadowProgram.vertexAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(objectProgram.shadowProgram.vertexAttribLocation);

	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, objectProgram.indexBuffer);
	gl.drawElements(gl.TRIANGLES, objectProgram.indexBuffer.numItems, gl.UNSIGNED_SHORT, 0);
  	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
  	
  	gl.useProgram(null);
}

function initBlur(){
	blurProgram = initShaders("vs-blur", "fs-blur");
	gl.useProgram(blurProgram);
	blurProgram.vertexAttribLocation = gl.getAttribLocation(blurProgram, "v");
	var vertices = [-1.0, -1.0,  1.0, -1.0, 1.0,  1.0, -1.0,  1.0];
	var indices = [ 0,3,1, 3,2,1];
	blurProgram.vertexBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, blurProgram.vertexBuffer);								
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);

	blurProgram.indexBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, blurProgram.indexBuffer);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);

	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
	gl.bindBuffer(gl.ARRAY_BUFFER, null);
	gl.useProgram(null);
}

function renderBlur(){
	gl.useProgram(blurProgram);

	gl.activeTexture(gl.TEXTURE0);
	gl.bindTexture(gl.TEXTURE_2D, blurProgram.textureId);
	gl.uniform1i(gl.getUniformLocation(blurProgram, "texture"), 0);

	gl.uniform2f(gl.getUniformLocation(blurProgram, "screenSize"), gl.viewportWidth, gl.viewportHeight);
	gl.bindBuffer(gl.ARRAY_BUFFER, blurProgram.vertexBuffer);
	gl.vertexAttribPointer(blurProgram.vertexAttribLocation, 2, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(blurProgram.vertexAttribLocation);

	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, blurProgram.indexBuffer);
	gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);
  	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);

  	gl.bindTexture(gl.TEXTURE_2D, null);
  	gl.useProgram(null);
	
}

function initCubeMap(){
	//Shaders
	cubeMapProgram = initShaders("vs-cubemap", "fs-cubemap");

	// Attributes
	gl.useProgram(cubeMapProgram);
	cubeMapProgram.vertexAttribLocation = gl.getAttribLocation(cubeMapProgram, "v");
	
	// Data
	var vertices = [-1.0, -1.0, -1.0, // Front
					1.0, -1.0, -1.0, 
					1.0,  1.0, -1.0,
					-1.0,  1.0, -1.0,
					1.0,-1.0,-1.0, // Right
					1.0,-1.0,1.0,
					1.0,1.0,1.0,
					1.0,1.0,-1.0,
					1.0,-1.0,1.0, // Back
					-1.0,-1.0,1.0,
					-1.0,1.0,1.0,
					1.0,1.0,1.0,
					-1.0,-1.0,1.0, // Left
					-1.0,-1.0,-1.0,
					-1.0,1.0,-1.0,
					-1.0,1.0,1.0,
					-1.0,1.0,-1.0, // Top
					1.0,1.0,-1.0,
					1.0,1.0,1.0,
					-1.0,1.0,1.0,
					1.0,-1.0,-1.0, // Bottom
					-1.0,-1.0,-1.0,
					-1.0,-1.0,1.0,
					1.0,-1.0,1.0 ];

	var indices = [ 0,1,3, 3,1,2,
					4,5,7, 7,5,6,
					8,9,11, 11,9,10,
					12,13,15, 15,13,14,
					16,17,19, 19,17,18,
					20,21,23, 23,21,22 ];

	cubeMapProgram.vertexBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, cubeMapProgram.vertexBuffer);								
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);

	cubeMapProgram.indexBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cubeMapProgram.indexBuffer);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);
	
	cubeMapProgram.textureId = loadTextureCubeMap("textures/cubemap/cubemap"); 

	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);
	gl.bindBuffer(gl.ARRAY_BUFFER, null);
	gl.useProgram(null);
}

function renderCubeMap(viewMatrix, projectionMatrix){
	gl.useProgram(cubeMapProgram);

	gl.activeTexture(gl.TEXTURE0);
	gl.bindTexture(gl.TEXTURE_CUBE_MAP, cubeMapProgram.textureId);
	gl.uniform1i(gl.getUniformLocation(cubeMapProgram, "texture"), 0);

	var mvp = mat4.create();
	mat4.fromScaling(mvp, vec3.fromValues(5.0,5.0,5.0));
	mat4.multiply(mvp, viewMatrix, mvp);
	mat4.multiply(mvp, projectionMatrix, mvp);
	var mvpind = gl.getUniformLocation(cubeMapProgram, "mvp");
   	gl.uniformMatrix4fv(mvpind,false,mvp);
	
	gl.bindBuffer(gl.ARRAY_BUFFER, cubeMapProgram.vertexBuffer);
	gl.vertexAttribPointer(cubeMapProgram.vertexAttribLocation, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(cubeMapProgram.vertexAttribLocation);

	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cubeMapProgram.indexBuffer);
	gl.drawElements(gl.TRIANGLES, 36, gl.UNSIGNED_SHORT, 0);
  	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,null);

  	gl.bindTexture(gl.TEXTURE_CUBE_MAP, null);
  	gl.useProgram(null);
}


