#include "GenerationUtilities.h"
#include <iostream>


void Random::seed(){
	std::random_device rd;
	_seed = rd();
	_mt = std::mt19937(_seed);
	_realDist = std::uniform_real_distribution<float>(0,1);
}

void Random::seed(unsigned int seedValue){
	_seed = seedValue;
	_mt = std::mt19937(_seed);
	_realDist = std::uniform_real_distribution<float>(0,1);
}

int Random::Int(int min, int max){
	return (int)(floor(Float() * (max+1 - min)) + min);
}

float Random::Float(){
	return _realDist(_mt);
}

float Random::Float(float min, float max){
	return _realDist(_mt)*(max-min)+min;
}

unsigned int Random::getSeed(){
	return _seed;
}

std::mt19937 Random::_mt;
std::uniform_real_distribution<float> Random::_realDist;
unsigned int Random::_seed;

