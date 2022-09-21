#ifndef CAJ_LEVELS
#define CAJ_LEVELS
#define MAX_MAP_WIDTH 240
#define MAX_SCREENS_HIGH 2
#define MAP_SCREEN_HEIGHT 16

extern SDL_Texture *skybox;
extern SDL_Texture *tiles;

void clamp_cam(int *cx, int *cy);
void load_column(void);
int load_level(int);
int reset_level(void);
/* map: cam_x, cam_y */
void map(int, int);

/*
 * tile information
 */
/* flags: tilenum */
int flags(int);
/* tile_at: x XX.8, y XX.8 */
int tile_at(int, int);

/* if the tile has a thingy, take it! */
void collect(int x, int y);

#endif
