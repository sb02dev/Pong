#include "Arduino.h"

bool checkHCollision(
	int32_t x1, int32_t y, int32_t x2, /* section one -- fixed, horizontal */
	int32_t x3, int32_t y3, int32_t dx3, int32_t dy3, /* section two -- starting at x3,y3 and moving by dx3*dy3 speeds */
	int32_t dt, /* the time lapse to be checked */
	int32_t *dtcoll /* the time the collision happens */
);

bool checkVCollision(
	int32_t x, int32_t y1, int32_t y2, /* section one -- fixed, vertical */
	int32_t x3, int32_t y3, int32_t dx3, int32_t dy3, /* section two -- starting at x3,y3 and moving by dx3*dy3 speeds */
	int32_t dt, /* the time lapse to be checked */
	int32_t *dtcoll /* the time the collision happens */
);