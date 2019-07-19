#ifndef GenerationUtilities_h
#define GenerationUtilities_h

#include <random>

class Random {
public:
	
	static void seed();
	
	static void seed(unsigned int seedValue);
	
	static int Int(int min, int max);
	
	static float Float();
	
	static float Float(float min, float max);
	
	static unsigned int getSeed();
	
private:
	
	static unsigned int _seed;
	static std::mt19937 _mt;
	static std::uniform_real_distribution<float> _realDist;
};

#endif
