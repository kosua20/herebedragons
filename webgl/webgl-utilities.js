
function computeTangentsAndBinormals(mesh){
	mesh.tangents = Array(mesh.vertexBuffer.numItems*3).fill(0.0);
	mesh.binormals = Array(mesh.vertexBuffer.numItems*3).fill(0.0);
	for(fid = 0; fid < mesh.indexBuffer.numItems; fid += 3){

		// Get the vertices of the face.
		var v0x = mesh.vertices[mesh.indices[fid]*3];
		var v0y = mesh.vertices[mesh.indices[fid]*3+1];
		var v0z = mesh.vertices[mesh.indices[fid]*3+2];
		var v1x = mesh.vertices[mesh.indices[fid+1]*3];
		var v1y = mesh.vertices[mesh.indices[fid+1]*3+1];
		var v1z = mesh.vertices[mesh.indices[fid+1]*3+2];
		var v2x = mesh.vertices[mesh.indices[fid+2]*3];
		var v2y = mesh.vertices[mesh.indices[fid+2]*3+1];
		var v2z = mesh.vertices[mesh.indices[fid+2]*3+2];
		
		var v0 = vec3.fromValues(v0x, v0y, v0z);
		var v1 = vec3.fromValues(v1x, v1y, v1z);
		var v2 = vec3.fromValues(v2x, v2y, v2z);
		
		
		// Get the uvs of the face.
		var  uv0x = mesh.textures[mesh.indices[fid]*2];
		var  uv0y = mesh.textures[mesh.indices[fid]*2+1];
		var  uv1x = mesh.textures[mesh.indices[fid+1]*2];
		var  uv1y = mesh.textures[mesh.indices[fid+1]*2+1];
		var  uv2x = mesh.textures[mesh.indices[fid+2]*2];
		var  uv2y = mesh.textures[mesh.indices[fid+2]*2+1];

		var uv0 = vec2.fromValues(uv0x, uv0y);
		var uv1 = vec2.fromValues(uv1x, uv1y);
		var uv2 = vec2.fromValues(uv2x, uv2y);
		
		// Delta positions and uvs.
		var deltaPosition1 = vec3.create();
		vec3.sub(deltaPosition1, v1, v0);
		var deltaPosition2 = vec3.create();
		vec3.sub(deltaPosition2, v2, v0);
		var deltaUv1 = vec2.create();
		vec2.sub(deltaUv1, uv1, uv0);
		var deltaUv2 = vec2.create();
		vec2.sub(deltaUv2, uv2, uv0);

		// Compute tangent and binormal for the face.
		var det = 1.0 / (deltaUv1[0] * deltaUv2[1] - deltaUv1[1] * deltaUv2[0]);
		
    	var tangent = vec3.create();
    	var tangent1 = vec3.create();
    	vec3.scale(tangent, deltaPosition1, det* deltaUv2[1]);
    	vec3.scale(tangent1, deltaPosition2, det* deltaUv1[1]);
    	vec3.sub(tangent, tangent, tangent1);
    	 
    	var binormal = vec3.create();
    	var binormal1 = vec3.create();
    	vec3.scale(binormal, deltaPosition2, det *deltaUv1[0] );
    	vec3.scale(binormal1, deltaPosition1, det *deltaUv2[0] )
    	vec3.sub(binormal, binormal, binormal1);

    	// Accumulate them. We don't normalize to get a free weighting based on the size of the face.
    	for(fidshift = 0; fidshift < 3; fidshift++){
    		mesh.tangents[mesh.indices[fid+fidshift]*3] += tangent[0];
   			mesh.tangents[mesh.indices[fid+fidshift]*3+1] += tangent[1];
   			mesh.tangents[mesh.indices[fid+fidshift]*3+2] += tangent[2];

   			mesh.binormals[mesh.indices[fid+fidshift]*3] += binormal[0];
   			mesh.binormals[mesh.indices[fid+fidshift]*3+1] += binormal[1];
   			mesh.binormals[mesh.indices[fid+fidshift]*3+2] += binormal[2];
    	}
   		
 
	}
	
	// Finally, enforce orthogonality and good orientation of the basis.
	for(tid = 0; tid < mesh.vertexBuffer.numItems; tid++){
		var ltangent = vec3.fromValues(mesh.tangents[tid*3],mesh.tangents[tid*3+1],mesh.tangents[tid*3+2]);
		var lnormal = vec3.fromValues(mesh.vertexNormals[tid*3],mesh.vertexNormals[tid*3+1],mesh.vertexNormals[tid*3+2]);
		var lbinormal = vec3.fromValues(mesh.binormals[tid*3],mesh.binormals[tid*3+1],mesh.binormals[tid*3+2]);
		var ltempn = vec3.create(); 
		var ltempt = vec3.create();
		var dott = vec3.dot(lnormal, ltangent);
		vec3.scale(ltempn, lnormal, dott);
		vec3.sub(ltempt, ltangent , ltempn);
		vec3.normalize(ltempt,ltempt);
		vec3.cross(lnormal,lnormal,ltempt);
		if (vec3.dot(lnormal, lbinormal) < 0.0){
			vec3.scale(ltempt, ltempt, -1.0);
		}
		//ltempt good to go
		mesh.tangents[tid*3] = ltempt[0];
		mesh.tangents[tid*3+1] = ltempt[1];
		mesh.tangents[tid*3+2] = ltempt[2];
	}
	

	return mesh;
}


function loadTexture(name){

	var textureId = gl.createTexture();
	var image = new Image();
	image.onload = function(){ 
		gl.bindTexture(gl.TEXTURE_2D, textureId);
		gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_NEAREST);
		gl.generateMipmap(gl.TEXTURE_2D);
		gl.bindTexture(gl.TEXTURE_2D, null);
	 }
	image.src = name;
	return textureId;
}

function loadTextureCubeMap(baseName){
	var texture = gl.createTexture();
	gl.bindTexture(gl.TEXTURE_CUBE_MAP, texture);
	gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
	gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
	gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
	gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

	var faces = [["_r.png", gl.TEXTURE_CUBE_MAP_POSITIVE_X],
				 ["_l.png", gl.TEXTURE_CUBE_MAP_NEGATIVE_X],
				 ["_u.png", gl.TEXTURE_CUBE_MAP_POSITIVE_Y],
				 ["_d.png", gl.TEXTURE_CUBE_MAP_NEGATIVE_Y],
				 ["_b.png", gl.TEXTURE_CUBE_MAP_POSITIVE_Z],
				 ["_f.png", gl.TEXTURE_CUBE_MAP_NEGATIVE_Z]];

	for (var i = 0; i < faces.length; i++) {
		var face = faces[i][1];
		var image = new Image();
		image.onload = function(texture, face, image) {
			return function() {
				gl.bindTexture(gl.TEXTURE_CUBE_MAP, texture);
				gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
				gl.texImage2D(face, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
			}
		} (texture, face, image);
		image.src = baseName + faces[i][0];
	}
	gl.bindTexture(gl.TEXTURE_CUBE_MAP, null);
	return texture;
}


function initShaders(vertexString, fragmentString){
	// Compile shaders.
  	var vertexShader = getShader(gl.VERTEX_SHADER,vertexString);
	var fragmentShader = getShader(gl.FRAGMENT_SHADER,fragmentString);
	// Create program, link shaders.
	var shaderprogram = gl.createProgram();
	gl.attachShader(shaderprogram, vertexShader);
	gl.attachShader(shaderprogram, fragmentShader);
	gl.linkProgram(shaderprogram);
	// Error check.
	if(!gl.getProgramParameter(shaderprogram, gl.LINK_STATUS)){
		alert("Unable to initialize the shader program: " + gl.getProgramInfoLog(shader));
	}
	return shaderprogram;

}

function getShader(shaderType, shaderString){

	var shader = gl.createShader(shaderType);
	gl.shaderSource(shader, shaderString);
	gl.compileShader(shader);
	if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {  
		alert("An error occurred compiling the shaders: " + gl.getShaderInfoLog(shader));  
		return null;  
  	}
  	return shader;

}

function initFramebuffer(width, height){
	var framebuffer = {};
	framebuffer.id = gl.createFramebuffer();
	gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer.id);
    framebuffer.width = width;
    framebuffer.height = height;

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