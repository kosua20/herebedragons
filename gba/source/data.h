#ifndef __DATA_H__
#define __DATA_H__

#define TILES64x64LEN 512
#define TILES64x32LEN 256
#define TILES16x16LEN 32

#define TILES64x64SIZE 2048
#define TILES64x32SIZE 1024
#define TILES16x16SIZE 128

#define TILESBG0LEN 256
#define TILESBG1LEN 632

#define BGMAPLEN 512

#define PALETTESIZE 16
#define PALETTELEN 8

// Dragon.
extern const unsigned int dragonPalette[PALETTELEN];
extern const unsigned int dragon0Tiles[TILES64x64LEN];
extern const unsigned int dragon1Tiles[TILES64x64LEN];
extern const unsigned int dragon2Tiles[TILES64x32LEN];
extern const unsigned int dragon3Tiles[TILES64x32LEN];

// Monkey.
extern const unsigned int monkeyPalette[PALETTELEN];
extern const unsigned int monkey0Tiles[TILES64x64LEN];
extern const unsigned int monkey1Tiles[TILES64x64LEN];
extern const unsigned int monkey2Tiles[TILES64x64LEN];
extern const unsigned int monkey3Tiles[TILES64x64LEN];
extern const unsigned int monkey4Tiles[TILES64x64LEN];
extern const unsigned int monkey5Tiles[TILES64x64LEN];
extern const unsigned int monkey6Tiles[TILES64x64LEN];
extern const unsigned int monkey7Tiles[TILES64x64LEN];

// Sun.
extern const unsigned int sunPalette[PALETTELEN];
extern const unsigned int sun0Tiles[TILES16x16LEN];

// Ground.
extern const unsigned int bg0Palette[PALETTELEN];
extern const unsigned int bg0Tiles[TILESBG0LEN];
extern const unsigned int bg0Map[BGMAPLEN];

// Sky: close and far.
extern const unsigned int bg1Palette[PALETTELEN];
extern const unsigned int bg1Tiles[TILESBG1LEN];
extern const unsigned int bg1Map[BGMAPLEN];
extern const unsigned int bg2Map[BGMAPLEN];

#endif // __DATA_H__
