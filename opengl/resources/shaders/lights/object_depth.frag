#version 330


// Output: the fragment color
out vec2 fragColor;

void main(){
	
	fragColor = vec2(gl_FragCoord.z,gl_FragCoord.z*gl_FragCoord.z);
	
}
