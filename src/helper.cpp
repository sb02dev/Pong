#include "helper.h"
#include "b2debug.h"
bool checkHCollision(
	int32_t x1, int32_t y, int32_t x2, /* section one -- fixed, horizontal */
	int32_t x3, int32_t y3, int32_t dx3, int32_t dy3, /* section two -- starting at x3,y3 and moving by dx3*dy3 speeds */
	int32_t dt, /* the time lapse to be checked */
	int32_t *dtcoll /* the time the collision happens */
) {
  if (dy3==0) return false; // parallel
	int32_t collisiontime=(y-y3)*1000/dy3; // y3+t*dy3=y ==> t=(y-y3)/dy3 gives t in milliseconds since coordinates are multiplied by 1000
  if (collisiontime<0 || collisiontime>dt) return false; // outside time
	dbgf4(b2DEBUG_COLLISION, "\n\tcollisionline: (%d;%d) --> (%d;%d)",x1, y, x2, y);
	dbgf4(b2DEBUG_COLLISION, "\n\tcollisionvector: (%d;%d) | (%d;%d)",x3,y3,dx3,dy3);
	dbgf3(b2DEBUG_COLLISION, "\n\tcollisiontime: %d\tcollisionpoint: (%d;%d)\n; ", collisiontime, x3+collisiontime*dx3/1000, y);
	if (x3+collisiontime*dx3/1000 < x1 || x3+collisiontime*dx3/1000 > x2) return false; // outside section
  *dtcoll = collisiontime;
	return true;
}

bool checkVCollision(
	int32_t x, int32_t y1, int32_t y2, /* section one -- fixed, vertical */
	int32_t x3, int32_t y3, int32_t dx3, int32_t dy3, /* section two -- starting at x3,y3 and moving by dx3*dy3 speeds */
	int32_t dt, /* the time lapse to be checked */
	int32_t *dtcoll /* the time the collision happens */
) {
  if (dx3==0) return false; // parallel
	int32_t collisiontime=(x-x3)*1000/dx3; // x3+t*dx3=x ==> t=(x-x3)/dx3 gives t in milliseconds since coordinates are multiplied by 1000
  if (collisiontime<0 || collisiontime>dt) return false; // outside time
	dbgf4(b2DEBUG_COLLISION, "\n\tcollisionline: (%d;%d) --> (%d;%d)",x, y1, x, y2);
	dbgf4(b2DEBUG_COLLISION, "\n\tcollisionvector: (%d;%d) | (%d;%d)",x3,y3,dx3,dy3);
	dbgf3(b2DEBUG_COLLISION, "\n\tcollisiontime: %d\tcollisionpoint: (%d;%d)\n; ", collisiontime, x, y3+collisiontime*dy3/1000);
	if (y3+collisiontime*dy3/1000 < y1 || y3+collisiontime*dy3/1000 > y2) return false; // outside section
  *dtcoll = collisiontime;
	return true;
}
