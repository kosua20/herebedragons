/* Here be dragons, by Simon Rodriguez, 2018.
 More details on the overall project at http://simonrodriguez.fr/dragon
 Code for each version of the scene available at https://github.com/kosua20/herebedragons
 */

/* Uncomment to disable the soft min/max operations and use basic
 unions/intersections instead. This will speed up the compilation
 but degrade the look*/
//#define NO_SOFT_OPS

/* Comment to remove the monkey. */
#define SHOW_MONKEY

/* Comment to remove the dragon. */
#define SHOW_DRAGON


/* Comment to disable any texture use. */
#define USE_TEXTURES

/* Comment to remove the cloud noise in the sky. */
#define USE_NOISE

//// PI defines

#define M_PI (3.14159)
#define M_PI_2 (2.0*M_PI)
#define M_PI_O_2 (M_PI/2.0)


//// Transformations

// vector to transform, angle *in radians*
vec2 rotate(vec2 v, float angle){
	float c = cos(angle);
	float s = sin(angle);
	return vec2(
				c * v.x - s * v.y,
				s * v.x + c * v.y
				);
}

// vector to transform, translation to apply, ZYX euler angles *in degrees*
vec3 transform(vec3 p, vec3 t, vec3 r){
	vec3 rP = p - t;
	vec3 angles = M_PI/180.0 * r;
	rP.xy = rotate(rP.xy, angles.x);
	rP.xz = rotate(rP.xz, angles.y);
	rP.yz = rotate(rP.yz, angles.z);
	return rP;
}

//// Noise utilities

float mod289(float x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float permute(float x) {
	return mod289(((x*34.0)+1.0)*x);
}

vec3 interpolation(vec3 x){
	return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

float grad(vec3 corner, vec3 p){
	// Position relative to current corner.
	p = p - corner;
	// "Random" number.
	float t = floor(permute(permute( permute(corner.x) + corner.y) + corner.z));
	// We use 16 gradients.
	vec4 tmods = mod(vec4(t), vec4(16.0, 8.0, 4.0, 2.0));
	// Compute dot(p, gradient)
	float mul0 = 1.0-2.0*tmods.w;
	float mul1 = tmods.z < 2.0 ? 1.0 : -1.0;
	float num0 = tmods.x < 8.0 ? p.x : p.y;
	float num1 = tmods.y < 4.0 ? p.y : p.z;
	return mul0*num0+mul1*num1;
}

float pnoise(vec3 p){
	// Find the cube we are in.
	vec3 base = floor(p);
	vec3 blend = interpolation(p - base);
	vec2 bins = vec2(0.0,1.0);
	// Mix between corner gradients, using the inteprolation weights.
	float res = mix(
					mix(
						mix(grad(base+bins.xxx, p), grad(base+bins.yxx,p), blend.x),
						mix(grad(base+bins.xyx, p), grad(base+bins.yyx,p), blend.x),
						blend.y),
					mix(
						mix(grad(base+bins.xxy,p), grad(base+bins.yxy,p), blend.x),
						mix(grad(base+bins.xyy,p), grad(base+bins.yyy,p), blend.x),
						blend.y),
					blend.z);
	return res * 0.5 + 0.5;
}


float fbm(vec3 pos){
	float n;
	n = pnoise(pos*0.0625)*0.5;
	n += pnoise(pos*0.125)*0.25;
	n += pnoise(pos*0.25)*0.125;
	return n/0.875;
}


//// Primitive signed distance functions.
//// (source: IQ, https://iquilezles.org/www/articles/distfunctions/distfunctions.htm)

float sdSphere(vec3 p, vec3 c, float r){
	return length(p-c) - r*r;
}

float sdBox(vec3 p, vec3 r){
	vec3 d = abs(p) - (r);
	return length(max(d, 0.0)) + min(max(d.x,max(d.y,d.z)),0.0);
}

float sdSmoothBox(vec3 p, vec3 r, float s){
	vec3 d = abs(p) - (r - s);
	return length(max(d, 0.0)) - s + min(max(d.x,max(d.y,d.z)),0.0);
}

float sdPlaneH(vec3 p, float h){
	float dist = p.y + h;
	return max(abs(dist), 0.0);
}

float sdVerticalCapsule(vec3 p, float h, float r){
	p.y -= clamp( p.y, 0.0, h );
	return length( p ) - r;
}

float sdCone(vec3 p, float h, float r1){
	vec2 q = vec2( length(p.xz), p.y );
	
	vec2 k1 = vec2(0.0,h);
	vec2 k2 = vec2(-r1,2.0*h);
	vec2 ca = vec2(q.x-min(q.x,(q.y < 0.0)?r1:0.0), abs(q.y)-h);
	vec2 cb = q - k1 + k2*clamp( dot(k1-q,k2)/dot(k2,k2), 0.0, 1.0 );
	float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
	return s*sqrt( min(dot(ca,ca),dot(cb,cb)) );
}

float sdRoundCone(vec3 p, float r1, float r2, float h){
	vec2 q = vec2( length(p.xz), p.y );
	
	float b = (r1-r2)/h;
	float a = sqrt(1.0-b*b);
	float k = dot(q,vec2(-b,a));
	
	if( k < 0.0 ) return length(q) - r1;
	if( k > a*h ) return length(q-vec2(0.0,h)) - r2;
	
	return dot(q, vec2(a,b) ) - r1;
}

float sdCylinder(vec3 p, vec2 h){
	vec2 d = abs(vec2(length(p.xz),p.y)) - h;
	return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

/* pos, main radius/height, cap radius,  */
float sdRoundedCylinder(vec3 p,  vec2 h, float rb){
	vec2 d = vec2( length(p.xz)-2.0*h.x+rb, abs(p.y) - h.y );
	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rb;
}

float sdTorus(vec3 p, vec2 t){
	vec2 q = vec2(length(p.xz)-t.x,p.y);
	return length(q)-t.y;
}



//// Sharp boolean operators.

// float min(float a, float b);
// float max(float a, float b);

vec2 mini(vec2 a, vec2 b){
	return a.x < b.x ? a : b;
}

vec2 maxi(vec2 a, vec2 b){
	return a.x < b.x ? b : a;
}


//// Soft boolean operators.
//// (source: IQ, https://iquilezles.org/www/articles/smin/smin.htm)

float minSoft(float a, float b, float k){
#ifdef NO_SOFT_OPS
	return min(a,b);
#endif
	float h = max( k-abs(a-b), 0.0 );
	return min( a, b ) - h*h*h/(6.0*k*k);
}

float maxSoft(float a, float b, float k){
#ifdef NO_SOFT_OPS
	return max(a,b);
#endif
	float h = clamp( 0.5 - 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) + k*h*(1.0-h);
}

vec2 miniSoft(vec2 a, vec2 b, float k){
	float s = minSoft(a.x, b.x, k);
	return vec2(s, a.x < b.x ? a.y : b.y);
}

vec2 maxiSoft( vec2 d1, vec2 d2, float k ) {
	float s = maxSoft( d1.x, d2.x, k);
	return vec2(s, d1.x < d2.x ? d2.y : d1.y);
}


//// Helpers
//// Generate a primitive with a custom position and orientation.

float cylinderSmooth(vec3 p, vec3 t, vec3 r, vec2 h){
	vec3 rP = transform(p, t, r);
	return sdRoundedCylinder(rP, h, 0.2);
}

float cylinderFlat(vec3 p, vec3 t, vec3 r, vec2 h){
	vec3 rP = transform(p, t, r);
	return sdCylinder(rP, h);
}

float coneSmooth(vec3 p, vec3 t, vec3 r, vec3 h){
	vec3 rP = transform(p, t, r);
	return sdRoundCone(rP, h.x, h.y, h.z);
}

float coneFlat(vec3 p, vec3 t, vec3 r, vec2 h){
	vec3 rP = transform(p, t, r);
	return sdCone(rP, h.y, h.x);
}

float cubeFlat(vec3 p, vec3 t, vec3 r, vec3 s){
	vec3 rP = transform(p, t, r);
	return sdBox(rP, s);
}

float cubeSmooth(vec3 p, vec3 t, vec3 r, vec3 s, float m){
	vec3 rP = transform(p, t, r);
	return sdSmoothBox(rP, s, m);
}



//// Object signed distance functions.

//// Monkey signed distance function.

vec2 mapMonkey(vec3 p){
	// The head:
	// The main skull shape
	float headBase = sdSphere(p, vec3(0.01, 0.2, 0.124), 0.84);
	// Two rotated boxes for the face around the eyes
	vec3 rP = transform(p, vec3(-0.264, 0.168, -0.574), vec3(30.6, 3.0, 14.8));
	headBase = minSoft(headBase, sdSmoothBox(rP, vec3( 0.489, 0.368, 0.140), 0.25), 0.5);
	rP = transform(p, vec3(0.264, 0.168, -0.574), vec3(-30.6, 177.0, -14.8));
	headBase = minSoft(headBase, sdSmoothBox(rP, vec3( 0.489, 0.368, 0.140), 0.25), 0.5);
	// The chin
	headBase = minSoft(headBase, sdCylinder(p - vec3(0.0, -0.59, -0.569), vec2(0.15, 0.3)), 0.5);
	headBase = minSoft(headBase, sdVerticalCapsule(p.yxz - vec3(-0.819, -0.1,-0.546), 0.25, 0.1), 0.5);
	// The ears
	vec3 earLP = transform(p, vec3(-0.844, 0.15 ,0.151), vec3(-70.7, 0.0, 0.0));
	vec3 earRP = transform(p, vec3(0.844, 0.15, 0.151), vec3(70.7, 180.0, 0.0));
	float ears = min(sdRoundedCylinder(earLP , vec2(0.12, 0.25), 0.26), sdRoundedCylinder(earRP, vec2(0.12, 0.25), 0.26));
	headBase = minSoft(headBase, ears, 0.5);
	// Add the base material
	vec2 monkey = vec2(headBase, 1.0);
	
	// The nose
	float nose = minSoft(sdSphere(p, vec3( -0.05, -0.213, -0.748), 0.3), sdSphere(p, vec3(  0.05, -0.213, -0.748), 0.3), 0.07);
	monkey = miniSoft(monkey, vec2(nose, 7.0), 0.07);
	// The mouth (difference)
	monkey = maxiSoft(monkey, vec2(-sdVerticalCapsule(p.yxz - vec3(-0.8, -0.0,-0.74), 0.1, 0.05), 5.0), 0.05);
	
	// The inside of the ears (difference)
	float innerEars = min(sdRoundedCylinder(earLP + vec3(0.02, -0.15, 0.1) , vec2(0.1,0.04), 0.2), sdRoundedCylinder(earRP + vec3(0.02, -0.15, -0.1) , vec2(0.1, 0.04), 0.2));
	monkey = maxiSoft(monkey, vec2(-innerEars, 6.0), 0.2);
	
	// The eyes, with their pupils and irises
	vec2 leftEye = vec2(sdSphere(p,  vec3( -0.338, 0.228, -0.67), 0.42), 2.0);
	vec2 rightEye = vec2(sdSphere(p,  vec3( 0.338, 0.228, -0.67), 0.42), 2.0);
	vec2 leftEyeI = vec2(sdSphere(p, vec3( -0.338, 0.228, -0.75), 0.34), 3.0);
	vec2 leftEyeP = vec2(sdSphere(p, vec3( -0.338, 0.228, -0.805), 0.26), 4.0);
	vec2 rightEyeI = vec2(sdSphere(p,vec3(  0.338, 0.228, -0.75), 0.34), 3.0);
	vec2 rightEyeP = vec2(sdSphere(p,vec3(  0.338, 0.228, -0.805), 0.26), 4.0);
	
	// Merge everything.
	leftEye = mini(mini(leftEye, leftEyeI), leftEyeP);
	rightEye = mini(mini(rightEye, rightEyeI), rightEyeP);
	monkey = mini(mini(monkey, leftEye), rightEye);
	return monkey;
}

//// Dragon signed distance function.

vec2 mapFoot(vec3 p){
	// The fingers.
	float footFingers = coneSmooth(p, vec3(0.0), vec3(-0.0, 0.0, -90.0), vec3(0.12, 0.08, 0.3));
	footFingers = min(footFingers, coneSmooth(p, vec3(0.0), vec3(-0.0, 30.0, -90.0), vec3(0.12, 0.08, 0.3)));
	footFingers = min(footFingers, coneSmooth(p, vec3(0.0), vec3(-0.0, -30.0, -90.0), vec3(0.12, 0.08, 0.3)));
	// The nails.
	float footNails = coneSmooth(p, vec3(0.15, 0.0, 0.3), vec3(-0.0, 15.0, -90.0), vec3(0.07, 0.01, 0.15));
	footNails = min(footNails, coneSmooth(p, vec3(0.0, 0.0, 0.35), vec3(-0.0, 0.0, -90.0), vec3(0.07, 0.01, 0.15)));
	footNails = min(footNails, coneSmooth(p, vec3(-0.15, 0.0, 0.28), vec3(-0.0, -15.0, -90.0), vec3(0.07, 0.01, 0.15)));
	// Merge and assign the proper material.
	return mini(vec2(footFingers, 8.0), vec2(footNails, 9.0));
}

vec2 mapJaw(vec3 p){
	// Two cylinders for the jaw itself.
	float jawbase = cylinderFlat(p, vec3(-0.012, -0.128, -0.043), vec3(95.2, -188.0, 7.3 ), vec2(0.048, 0.323));
	jawbase = minSoft(jawbase, cylinderFlat(p, vec3(-0.012, -0.128, 0.178), vec3(-86.0, -173.0, 172.0), vec2(0.048, 0.323)), 0.2);
	// Add a half torus at the front.
	vec3 rP = p - vec3(-0.32, -0.12, 0.02);
	float jawFrontBottom = sdTorus(rP, vec2(0.12, 0.04));
	// Cut it in half by intersecting with a box.
	jawFrontBottom = maxSoft(jawFrontBottom, sdBox(rP + vec3(0.5, 0.0, 0.0), vec3(0.5, 1.0, 1.0)), 0.1);
	// Merge, assign material.
	vec2 jawBottom = vec2(minSoft(jawbase, jawFrontBottom, 0.1), 8.0);
	// The tongue.
	float tongue = cubeFlat(p, vec3(-0.029, -0.105, 0.067), vec3(0.0, -98.6, 0.0), vec3(0.02, 0.001,0.3));
	// Merge with the jaw.
	jawBottom = miniSoft(jawBottom, vec2(tongue, 5.0), 0.14) ;
	// The teeth.
	// Two fangs, using cones.
	float toothBottom = coneFlat(p, vec3(-0.32, -0.058, 0.1), vec3(0.0, -25.2, 0.0), vec2(0.04, 0.08));
	toothBottom = min(toothBottom, coneFlat(p, vec3(-0.3, -0.058, -0.05), vec3(0.0, -25.2, 0.0), vec2(0.04, 0.08)));
	return mini(vec2(toothBottom, 6.0), jawBottom);
}

vec2 mapDragonHead(vec3 p){
	// Bottom jaw.
	vec2 jawBottom = mapJaw(p);
	// Add the pointy tongue.
	vec2 spikingTongue = vec2(coneSmooth(p, vec3(-0.30, -0.12, 0.02), vec3(-54.2, -22.9, 6.23), vec3(0.025, 0.005, 0.2)), 5.0);
	jawBottom = miniSoft(jawBottom, spikingTongue, 0.1);
	// Top jaw.
	vec3 rP = p ;
	// Flip the jaw and orient it.
	rP.y = -rP.y;
	rP.xy = rotate(rP.xy, -0.3);
	vec2 jawtop = mapJaw(rP);
	// Add the nose and chin bumps.
	jawtop = miniSoft(jawtop, vec2(sdSphere(p, vec3(-0.3, 0.25, 0.025), 0.22), 8.0), 0.1);
	jawBottom = miniSoft(jawBottom, vec2(sdSphere(p, vec3(-0.4, -0.2, 0.025), 0.15), 8.0), 0.15);
	vec2 combinedHead = mini(jawBottom, jawtop);
	// Merge both jaws with the base head.
	float baseHead = cubeSmooth(p, vec3(0.163, 0.25, 0.1), vec3(9.41, -7.33, 0.0), vec3(0.2, 0.04, 0.15), 0.07);
	combinedHead = miniSoft(combinedHead, vec2(baseHead, 8.0), 0.3);
	
	// Spikes on both sides and the neck.
	// Left spikes: a series of rounded cones.
	float leftSpikes = coneSmooth(p, vec3(0.286, 0.236,-0.04), vec3(60.0, -29.1, -4.2), vec3(0.05, 0.01, 0.32));
	leftSpikes = min(leftSpikes, coneSmooth(p, vec3(0.35,0.16,-0.05), vec3(82.6, -26.9, -5.0), vec3(0.056, 0.01, 0.34)));
	leftSpikes = min(leftSpikes, coneSmooth(p, vec3(0.40,0.064,-0.09), vec3(84.1,-30.5,14.1), vec3(0.056,0.01,0.28)));
	leftSpikes = min(leftSpikes, coneSmooth(p, vec3(0.42,-0.037,-0.12), vec3(105.0,-0.0,15.0), vec3(0.056,0.01,0.28)));
	// Same for the right spikes.
	float rightSpikes = coneSmooth(p, vec3(0.286, 0.236,0.22), vec3(60.0, -29.1, 0.0), vec3(0.05, 0.01, 0.32));
	rightSpikes = min(rightSpikes, coneSmooth(p, vec3(0.35,0.16,0.23), vec3(82.6, -26.9, -15.0), vec3(0.056, 0.01, 0.34)));
	rightSpikes = min(rightSpikes, coneSmooth(p, vec3(0.43,0.064,0.27), vec3(84.1,-20.5,-28.0), vec3(0.056,0.01,0.28)));
	rightSpikes = min(rightSpikes, coneSmooth(p, vec3(0.44,-0.037,0.3), vec3(105.0,-10.0,-35.0), vec3(0.056,0.01,0.28)));
	// And the center spikes.
	float centerSpikes = coneSmooth(p, vec3(0.45,0.2,0.1), vec3(70.0,0.0,-0.0), vec3(0.06,0.02,0.5));
	centerSpikes = min(centerSpikes, coneSmooth(p, vec3(0.58,0.3,0.1), vec3(20.0,0.0,-0.0), vec3(0.04,0.01,0.2)));
	// Merge all spikes.
	vec2 spikes = vec2(min(min(leftSpikes, rightSpikes), centerSpikes), 8.0);
	// Add them to the head.
	combinedHead = miniSoft(combinedHead, spikes, 0.1);
	
	// Finally the eyes.
	float eyes = sdSphere(p, vec3(-0.07, 0.25, 0.12), 0.18);
	eyes = min(eyes, sdSphere(p, vec3(-0.05, 0.25, -0.0), 0.18));
	// Pupils.
	float eyeDots = sdSphere(p, vec3(-0.09, 0.26, 0.115), 0.12);
	eyeDots = min(eyeDots, sdSphere(p, vec3(-0.07, 0.26, -0.0), 0.12));
	// Combine everything.
	vec2 mergedEyes = mini(vec2(eyes, 11.0), vec2(eyeDots, 4.0));
	return mini(combinedHead, mergedEyes);;
}

vec2 mapDragon(vec3 p){
	// Body: combine tori and cylinders.
	// Four tori, each intersected with one (or two) boxes to only keep half of it..
	vec3 rP = p - vec3(-0.35, 0.8, -0.9);
	float topbody = sdTorus(rP.yxz, vec2(0.35, 0.2));
	topbody = maxSoft(topbody, sdBox(rP - vec3(0.0, 0.5, 0.0), vec3(1.0, 0.5, 1.0)), 0.15);
	rP = p - vec3(-0.35, 0.25, -0.1);
	float topbody1 = sdTorus(rP.yxz, vec2(0.35, 0.2));
	topbody1 = maxSoft(topbody1, sdBox(rP + vec3(0.0, 0.5, 0.0), vec3(1.0, 0.5, 1.0)), 0.15);
	rP = p - vec3(-0.55, 0.25, -1.45);
	rP.xz = rotate(rP.xz, 0.6);
	float topbody2 = sdTorus(rP.yxz, vec2(0.34, 0.2));
	topbody2 = maxSoft(topbody2, sdBox(rP + vec3(0.0, 0.5, 0.0), vec3(1.0, 0.5, 1.0)), 0.15);
	rP = p - vec3(-0.35, 0.8, 0.5);
	float topbody3 = sdTorus(rP.yxz, vec2(0.34, 0.2));
	topbody3 = maxSoft(topbody3, sdBox(rP - vec3(0.0, 0.5, 0.0), vec3(1.0, 0.5, 1.0)), 0.15);
	topbody3 = maxSoft(topbody3, sdBox(rP - vec3(0.0, 0.5, -1.0), vec3(1.0, 0.5, 1.0)), 0.15);
	// Merge the tori.
	float dragonBody = topbody3;
	dragonBody = min(topbody1, dragonBody);
	dragonBody = min(topbody, dragonBody);
	dragonBody = min(topbody2, dragonBody);
	// Add junction cylinders.
	dragonBody = minSoft(cylinderSmooth(p, vec3(-0.35,0.51,0.2), vec3(0.0, -0, 10.0), vec2(0.105, 0.25)), dragonBody, 0.1);
	dragonBody = minSoft(cylinderSmooth(p, vec3(-0.35,0.55,-0.51), vec3(0.0, -0, 10.0), vec2(0.105, 0.25)), dragonBody, 0.1);
	dragonBody = minSoft(cylinderSmooth(p, vec3(-0.35,0.51,-1.2), vec3(0.0, -0, 10.0), vec2(0.105, 0.25)), dragonBody, 0.1);
	// Smooth a bit the whole body.
	dragonBody -= 0.025;
	// Add the tail.
	dragonBody = minSoft(coneSmooth(p, vec3(-0.75,0.32,-1.74), vec3(30.0, -50.0, -20.0), vec3(0.18, 0.01, 0.8)), dragonBody, 0.1);
	
	// The end of the tail: two flat boxes rotated by 45° and merged.
	vec3 tP = p - vec3(-0.7, 1.2, -1.6);
	tP.xy = rotate(tP.xy, -0.7);
	tP.yz = rotate(tP.yz, 0.4);
	tP.xz = rotate(tP.xz, 0.4);
	float dragonTail = sdSmoothBox(tP, vec3(0.02,0.18, 0.18), 0.04);
	tP.yz = rotate(tP.yz, 0.6);
	dragonTail = min(dragonTail, sdSmoothBox(tP, vec3(0.02,0.18, 0.18), 0.04));
	// Add the end of the tail to the body.
	float dragonMain = minSoft(dragonBody, dragonTail, 0.25);
	
	// Legs.
	// The left back leg has three components because of the weird knee-spike.
	float leftBackLeg = cylinderSmooth(p, vec3(-0.7, 0.298, -1.15), vec3(15.2, 80.0, 46.9), vec2(0.071, 0.296));
	leftBackLeg = min(leftBackLeg, cylinderSmooth(p, vec3(-0.85, 0.2, -1.2), vec3(-5.0, 15.2, -1.0), vec2(0.071, 0.296)));
	leftBackLeg = minSoft(leftBackLeg, coneSmooth(p, vec3(-0.85, 0.7, -1.2), vec3(20.0, 25.0, -1.0), vec3(0.09, 0.03, 0.3)), 0.1);
	// Merge with the foot.
	vec2 leftBackFoot = mini(mapFoot(p - vec3(-0.85, -0.2, -1.2)), vec2(leftBackLeg, 8.0));
	
	// Right back leg, only one cylinder needed.
	float rightBackLeg = cylinderSmooth(p, vec3(-0.2, -0.13, -1.4), vec3(20.0, 20.0, 80.0), vec2(0.071, 0.2));
	vec2 rightBackFoot = mini(mapFoot(p - vec3(-0.1, -0.2, -1.2)), vec2(rightBackLeg, 8.0));
	
	// Right front leg, two cylinders.
	float rightFrontLeg = cylinderSmooth(p, vec3(-0.05, -0.15, -0.1), vec3(20.0, 20.0, 90.0), vec2(0.071, 0.1));
	rightFrontLeg = min(rightFrontLeg, cylinderSmooth(p, vec3(-0.05, 0.05, -0.05), vec3(-90.0, 40.0, 90.0), vec2(0.071, 0.18)));
	vec2 rightFrontFoot = mini(mapFoot(p - vec3(-0.0, -0.2, 0.1)), vec2(rightFrontLeg, 8.0));
	
	// Left front leg: two cylinders.
	float leftFrontLeg = cylinderSmooth(p, vec3(-0.7, -0.0, -0.1), vec3(10.0, -20.0, 110.0), vec2(0.071, 0.1));
	leftFrontLeg = min(leftFrontLeg, cylinderSmooth(p, vec3(-0.65, 0.05, -0.05), vec3(-90.0, 40.0, 90.0), vec2(0.071, 0.18)));
	// For the left front leg, the foot is custom, to wrap it around the blue stone.
	vec3 lP = p - vec3( -0.8, 0.05, 0.1);
	// Fingers.
	float leftFrontFootBase = coneSmooth(lP, vec3(0.0), vec3(-0.0, 0.0, -90.0), vec3(0.12, 0.08, 0.3));
	leftFrontFootBase = min(leftFrontFootBase, coneSmooth(lP, vec3(0.0), vec3(-0.0, 30.0, -90.0), vec3(0.12, 0.08, 0.3)));
	leftFrontFootBase = min(leftFrontFootBase, coneSmooth(lP, vec3(0.0), vec3(-0.0, -30.0, -100.0), vec3(0.12, 0.08, 0.3)));
	// Nails.
	float leftFrontFootNails = coneSmooth(lP, vec3(0.15, 0.0, 0.3), vec3(-0.0, 15.0, -150.0), vec3(0.07, 0.02, 0.15));
	leftFrontFootNails = min(leftFrontFootNails, coneSmooth(lP, vec3(0.0, 0.0, 0.35), vec3(-0.0, 0.0, -150.0), vec3(0.07, 0.02, 0.15)));
	leftFrontFootNails = min(leftFrontFootNails, coneSmooth(lP, vec3(-0.15, -0.08, 0.28), vec3(-0.0, -15.0, -150.0), vec3(0.07, 0.02, 0.15)));
	// Merge the fingers and nails.
	vec2 leftFrontFoot = vec2(min(leftFrontFootBase, leftFrontFootNails), leftFrontFootBase < leftFrontFootNails ? 8.0 : 9.0);
	// Add the leg.
	leftFrontFoot = miniSoft(leftFrontFoot, vec2(leftFrontLeg, 8.0), 0.1);
	// And add a blue sphere under the foot.
	leftFrontFoot = mini(leftFrontFoot, vec2(sdSphere(p, vec3(-0.75, -0.1, 0.3), 0.45), 10.0));
	// Merge all feet.
	vec2 feetMerged = mini(mini(mini(leftBackFoot, rightBackFoot),rightFrontFoot), leftFrontFoot);
	
	// Finally the head.
	vec3 headPos = p - vec3(-0.45, 1.2, 0.7);
	headPos.xz = rotate(headPos.xz, M_PI_O_2);
	vec2 dragonHead = mapDragonHead(headPos);
	
	// Merge everything.
	return miniSoft(miniSoft(vec2(dragonMain, 8.0), feetMerged, 0.2), dragonHead, 0.2);
}

//// Scene signed distance function.

vec2 map(vec3 p){
	
	// Ground.
	float ground = sdPlaneH(p, 1.01);
	vec2 res = vec2(ground, 0.0);
	
	// Monkey.
#ifdef SHOW_MONKEY
	// Compute position.
	const float monkeyScale = 0.35;
	const vec3 monkeyShift = vec3(-1.25, -0.2, 0.25);
	vec3 monkeyP = p - monkeyShift;
	monkeyP /= monkeyScale;
	// Animate the monkey.
	monkeyP.xz = rotate(monkeyP.xz, mod(iTime, M_PI_2));
	// Basic culling: approximate as a sphere, and use its SDF if far enough.
	float monkeyCull = monkeyScale*(sdSphere(p, monkeyShift, 1.0) );
	if(monkeyCull < 1.0){
		// Get our distance to the monkey.
		vec2 monkey = mapMonkey(monkeyP);
		// Compensate the scaling.
		monkey.x *= monkeyScale;
		// Store the result if closer.
		res = mini(res, monkey);
	} else {
		// Use the sphere SDF instead, adjusted so that isolines align with the full SDF isolines.
		res = mini(res, vec2(2.0*monkeyCull + 0.57));
	}
#endif
	
	// Dragon.
#ifdef SHOW_DRAGON
	// Compute position.
	const vec3 dragonShift = vec3(0.5, -0.75, 0.5);
	vec3 dragonP = p - dragonShift;
	// Basic culling: approximate as a sphere, and use its SDF if far enough.
	float dragonCull = (sdSphere(dragonP, vec3(-0.3,0.5,-0.7), 1.3) );
	if(dragonCull<1.0){
		// Get our distance to the dragon.
		vec2 dragon = mapDragon(dragonP);
		// Store the result if closer.
		res = mini(res, dragon);
	} else {
		// Use the sphere SDF instead, adjusted so that isolines align with the full SDF isolines.
		res = mini(res, vec2(dragonCull));
	}
#endif
	
	return res;
}



///// Geometry helpers.

// Compute the normal to the surface of the scene at a given world point.
/*vec3 normal(vec3 p){
 const vec2 epsilon = vec2(0.02, 0.0); //...bit agressive.
 // Use centered finite differences scheme.
 return normalize(vec3(
 map(p + epsilon.xyy).x - map(p - epsilon.xyy).x,
 map(p + epsilon.yxy).x - map(p - epsilon.yxy).x,
 map(p + epsilon.yyx).x - map(p - epsilon.yyx).x
 ));
 
 }*/

// Compute the normal to the surface of the scene at a given world point.
vec3 normalCheap(vec3 p){
	const vec2 epsilon = vec2(0.02, 0.0); //...bit agressive.
	float dP = map(p).x;
	// Forward differences scheme, cheaper.
	return normalize(vec3(
						  map(p + epsilon.xyy).x - dP,
						  map(p + epsilon.yxy).x - dP,
						  map(p + epsilon.yyx).x - dP
						  ));
}

// Origin and direction of the ray, distance to the hit, last step distance and material id.
bool raymarch(vec3 orig, vec3 dir, out float t, out vec2 res){
	// Reset.
	t = 0.0;
	res = vec2(0.0);
	// Step through the scene.
	for(int i = 0; i < 96; ++i){
		// Current position.
		vec3 pos = orig + t * dir;
		// Query the distance to the closest surface in the scene.
		res = map(pos);
		// Move by this distance.
		t += res.x;
		// If the distance to the scene is small, we have reached the surface.
		if(res.x < 0.01){
			return true;
		}
	}
	return false;
}

const vec3 lightDir = normalize(vec3(-1.0, 1.0, 1.0));
const vec3 groundTint = vec3(0.8, 0.9, 1.0);

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	// Compute ray in view space.
	vec2 uv = 2.0*fragCoord.xy/iResolution.xy-1.0;
	uv.x *= iResolution.x / iResolution.y;
	vec3 dir = normalize(vec3(uv, 1.0));
	vec3 eye = vec3(0.0, 0.0, -2.25);
	// Move camera around center based on cursor position.
	
	float horizAngle = -iMouse.x/iResolution.x * M_PI_2 + M_PI * 1.25 + M_PI;
	float vertAngle = M_PI_O_2 - iMouse.y/iResolution.y * (M_PI_O_2);
	// Hack for when the page is just loaded, so that the default viewpoint is interesting.
	if(iMouse.x == 0.0 && iMouse.y == 0.0){
		horizAngle = M_PI * 1.25;
		vertAngle = 0.0;
	}
	// Move eye and dir to world space.
	eye.yz = rotate(eye.yz, vertAngle);
	dir.yz = rotate(dir.yz, vertAngle);
	eye.xz = rotate(eye.xz, horizAngle);
	dir.xz = rotate(dir.xz, horizAngle);
	
	// Check if we intersect something along the ray.
	float t; vec2 res;
	bool didHit = raymarch(eye, dir, t, res);
	
	// Compute the color of the pixel.
	vec3 outColor = vec3(0.0);
	// If we hit a surface, compute it's appearance.
	if(didHit){
		// Compute position and normal.
		vec3 hit = eye + t * dir;
		vec3 n = normalCheap(hit);
		
		// See if the light is occluded by another surface, by marching in the light direction.
		float tDummy; vec2 resDummy;
		float shadowingFactor = float(raymarch(hit + 0.02*n, lightDir, tDummy, resDummy));
		
		// Diffuse lighting.
		float diffuse = max(dot(n, lightDir), 0.0);
		diffuse *= (1.0-shadowingFactor);
		// Specular lighting.
		float specular = max(dot(reflect(-lightDir, n), -dir), 0.0);
		
		vec3 baseColor = vec3(1.0,0.0,0.0);
		if(res.y < 0.5){
			// Plane texture.
#ifdef USE_TEXTURES
			float stones = texture(iChannel0, fract(hit.xz * 0.4)).r;
#else
			float stones = 0.5;
#endif
			// When reaching the horizon, fade the texture into a unique flat color.
			float fadingHorizon = 0.5 * clamp(length(hit) / 10.0, 0.0, 1.0) + 0.5;
			baseColor = mix(stones, 0.2, fadingHorizon) * groundTint;
			specular = pow(specular, 50.0);
			
		} else if(res.y < 1.5){
			// Monkey skin.
			baseColor = vec3(122.0, 86.0, 65.0)/255.0;
			specular = 0.0;
			
		} else if(res.y < 2.5){
			// Monkey eye.
			baseColor = vec3(1.0);
			specular = pow(specular, 100.0);
			
		} else if(res.y < 3.5){
			// Monkey eye iris.
			baseColor = vec3(0.1, 0.4, 1.0);
			specular = pow(specular, 100.0);
			
		} else if(res.y < 4.5){
			// Monkey eye pupil.
			baseColor = vec3(0.0);
			specular = pow(specular, 100.0);
			
		} else if(res.y < 5.5){
			// Monkey mouth.
			baseColor = vec3(0.29, 0.12, 0.13);
			specular = pow(specular, 80.0);
			
		} else if(res.y < 6.5){
			// Monkey ears.
			baseColor = vec3(227.0, 180.0, 100.0)/255.0;
			specular = 0.0;
			
		} else if(res.y < 7.5){
			// Monkey nose.
			baseColor = 0.9*vec3(0.84, 0.7, 0.69);
			specular = 0.0;
			
		} else if(res.y < 8.5){
			// Dragon skin.
			// Apply some high frequency details.
#ifdef USE_TEXTURES
			float scales = 0.25 * texture(iChannel1, 2.0 * hit.yz).r + 0.25;
#else
			float scales = 0.5;
#endif
			baseColor = scales * vec3(0.2, 0.9, 0.1);
			specular = pow(specular, 100.0);
			
		} else if(res.y < 9.5){
			// Dragon nails.
			baseColor = vec3(0.8, 0.9, 0.5);
			specular = 0.0;
			
		} else if(res.y < 10.5){
			// Dragon blue stone.
			// Apply some high frequency details.
#ifdef USE_TEXTURES
			float scales = texture(iChannel1, n.xy).r;
#else
			float scales = 1.0;
#endif
			baseColor = scales * vec3(0.0, 0.7, 1.0);
			specular = pow(specular, 100.0);
			
		} else if(res.y < 11.5){
			// Dragon eye.
			baseColor = vec3(1.0, 0.15, 0.0);
			specular = pow(specular, 100.0);
		}
		
		//Apply diffuse and ambient shading to the base color.
		outColor = (diffuse + 0.1) * baseColor + specular;
		
	} else {
		// We haven't hit anything, it's the sky.
		// Mix between two colors picked from the "miramar" cubemap.
		// Use FBM noise based on Perlin noise.
#ifdef USE_NOISE
		float scale = fbm(dir*45.0);
#else
		float scale = clamp(dir.y+0.2, 0.0, 1.0);
#endif
		outColor = mix(vec3(0.24, 0.27, 0.33), 1.5*vec3(0.53, 0.67, 0.72), scale*scale);
	}
	
	// Output to screen, apply gamma correction.
	fragColor = vec4(pow(outColor, vec3(1.0/2.2)), 1.0);
}
