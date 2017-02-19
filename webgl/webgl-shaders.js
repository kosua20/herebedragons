
var vsDefaultString = `
	precision mediump float;
	attribute vec3 v;
	attribute vec2 u;
	attribute vec3 no;
	attribute vec3 ta;
	attribute vec3 bi;

	uniform mat4 p;
	uniform mat4 mv;
	uniform mat4 imv;
	uniform mat4 lightMVP;

	varying mediump vec2 uv;
	varying mediump vec3 pos;
	varying mediump mat3 tbn;
	varying mediump vec3 lsp;

	void main(void) {
		vec4 fp = mv * vec4(v,1.0);
		gl_Position = p * fp;
		pos = fp.xyz;
		uv = u;
		vec3 T = normalize(mat3(imv) * ta);
		vec3 B = normalize(mat3(imv) * bi);
		vec3 N = normalize(mat3(imv) * no);
		tbn = mat3(T, B, N);

		lsp = 0.5*(lightMVP * vec4(v,1.0)).xyz + 0.5;

	}`

var fsDefaultString = `
	precision mediump float;
	varying mediump vec2 uv;
	varying mediump mat3 tbn;
	varying mediump vec3 pos;
	varying mediump vec3 lsp;
	uniform vec3 lightDir;
	uniform sampler2D texture;
	uniform sampler2D textureNormal;
	uniform sampler2D textureShadow;
	uniform float shininess;

	float shadow(vec3 lightSpacePosition){
		float probabilityMax = 1.0;
		if (lightSpacePosition.z < 1.0){
			// Read first and second moment from shadow map.
			vec2 moments = texture2D(textureShadow, lightSpacePosition.xy).rg;
			// Initial probability of light.
			float probability = float(lightSpacePosition.z <= moments.x);
			// Compute variance.
			float variance = moments.y - (moments.x * moments.x);
			variance = max(variance, 0.00008);
			// Delta of depth.
			float d = lightSpacePosition.z - moments.x;
			/*if(lightSpacePosition.z - moments.x > 0.005){
				probabilityMax = 1.0;
			}*/
			// Use Chebyshev to estimate bound on probability.
			probabilityMax = variance / (variance + d*d);
			probabilityMax = max(probability, probabilityMax);
			// Limit light bleeding by rescaling and clamping the probability factor.
			probabilityMax = clamp( (probabilityMax - 0.1) / (1.0 - 0.1), 0.0, 1.0);
		}
		return probabilityMax;
	}


	void main(void) {
		vec3 n = texture2D(textureNormal,uv).rgb;
		n = normalize(n * 2.0 - 1.0);
		n = normalize(tbn * n);
		vec3 l = normalize(lightDir);
		float diffuse = max(0.0, dot(n,l));
		float specular = 0.0;
		if(diffuse > 0.0){
			vec3 v = normalize(-pos);
			vec3 r = reflect(-l,n);
			specular = pow(max(dot(r,v),0.0),shininess);
		}
		vec3 albedo = texture2D(texture, uv).rgb;
		gl_FragColor.rgb = 0.2 * albedo + shadow(lsp) * (diffuse * albedo + specular);
		
		gl_FragColor.a = 1.0;
	}`

var vsShadowString = `
	precision mediump float;
	attribute vec3 v;
	
	uniform mat4 mvp;
	
	void main(void) {
		gl_Position = mvp * vec4(v, 1.0);
	}`

var fsShadowString = `
	precision mediump float;

	void main(void) {
		gl_FragColor.a = 1.0;
		gl_FragColor.rgb = vec3(gl_FragCoord.z,gl_FragCoord.z*gl_FragCoord.z,1.0);
	}`

var vsBlurString = `
	precision mediump float;
	attribute vec2 v;
	
	varying mediump vec2 uv;
	void main(void) {
		gl_Position = vec4(v,0.0, 1.0);
		uv = 0.5*v + 0.5;
	}`

var fsBlurString = `
	precision mediump float;
	varying mediump vec2 uv;
	uniform sampler2D texture;
	uniform vec2 screenSize;
	void main(void) {
		gl_FragColor.a = 1.0;
		gl_FragColor.b = 1.0;
		vec2 offset = 1.0/screenSize;
		vec2 color = vec2(0.0);
		for(int i = -2; i < 3; i++){
			for(int j = -2; j < 3; j++){
				color += texture2D(texture, uv + vec2(i,j)*offset).rg;
			}
		}
		
		gl_FragColor.rg = color / 25.0;
	}`

var vsCubemapString = `
	attribute vec3 v;
	uniform mat4 mvp;

	varying mediump vec3 uv;
	void main(void) {
		gl_Position = mvp * vec4(v,1.0);
		uv = v;
	}`
var fsCubemapString = `
	precision mediump float;
	varying mediump vec3 uv;
	uniform samplerCube texture;

	void main(void) {
		gl_FragColor.rgb = textureCube(texture, uv).rgb;
		gl_FragColor.a = 1.0;
	}`
