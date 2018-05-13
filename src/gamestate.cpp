#include "gamestate.h"
#include "b2debug.h"

/**********
** Circular buffer for game states
**   
***********/
#define GAMESTATE_BUFFER_SIZE 100 // that is ~3 seconds
PongGameState gameStates[GAMESTATE_BUFFER_SIZE]; // a circular buffer for past game states with static initialization (array of the data itself not pointers)
uint32_t oldestItem;
uint32_t latestItem;
bool empty;

void initBuffer() {
  oldestItem=0;
  latestItem=0;
  empty=true;
}

bool bufferEmpty() { return oldestItem==latestItem && empty; }
bool bufferFull() { return oldestItem==latestItem && !empty; }

uint32_t bufferAdd() { // add new item and drop oldest if needed (don't bother with filling the item with data, only frameID)
  uint32_t fid = empty?0:gameStates[latestItem].frameID+1;
  latestItem=(latestItem+1) % GAMESTATE_BUFFER_SIZE; // advance the "head" pointer
  if (latestItem==oldestItem) oldestItem=(oldestItem+1) % GAMESTATE_BUFFER_SIZE; // if buffer full then advance the "tail" pointer as well
  empty=false; // if we added item, it will always become non-empty
  gameStates[latestItem].frameID=fid;
  return latestItem;
}

PongGameState* curState() { return &gameStates[latestItem]; }
PongGameState* oldestState() { return &gameStates[oldestItem]; }
PongGameState* getState(uint32_t idx) { return (idx>GAMESTATE_BUFFER_SIZE-1)?NULL:&gameStates[idx]; }

uint32_t nextState(uint32_t idx) {
  if (idx==latestItem || idx>GAMESTATE_BUFFER_SIZE-1) return -1; // only cases when it can't go one further
  return (idx+1) % GAMESTATE_BUFFER_SIZE;
}

PongGameState* nextState(PongGameState* state) {
  uint32_t idx=getStateIdx(state);
  idx=nextState(idx);
  if (idx==-1) return NULL;
  return &gameStates[idx];
}

uint32_t prevState(uint32_t idx) {
  if (idx==oldestItem || idx>GAMESTATE_BUFFER_SIZE-1) return -1; // only cases when it can't go one further
  return (idx>0)?(idx-1):(GAMESTATE_BUFFER_SIZE-1);
}

PongGameState* prevState(PongGameState* state) {
  uint32_t idx=getStateIdx(state);
  idx=prevState(idx);
  if (idx==-1) return NULL;
  return &gameStates[idx];
}

uint32_t getStateIdx(PongGameState* state) {
  uint32_t idx=oldestItem;
  do {
    if (&gameStates[idx]==state) return idx;
    idx=nextState(idx);
    if (idx==-1) return -1;
  } while (idx<GAMESTATE_BUFFER_SIZE); // this will always be true because of modulo in nextState
  return -1;
}

uint32_t getStateIdxWithID(uint32_t frameID) {
  uint32_t idx=oldestItem;
  dbgf(b2DEBUG_GAMESTATE, "[[Starting from %d. ", idx);
  do {
    if (gameStates[idx].frameID==frameID) { dbgf(b2DEBUG_GAMESTATE, "RET(%d)]]", idx); return idx; }
    idx=nextState(idx);
    dbgf(b2DEBUG_GAMESTATE, "%d->", idx);
    if (idx==-1) { dbgf(b2DEBUG_GAMESTATE, "RET(%d)]]", -1); return -1; }
  } while (idx<GAMESTATE_BUFFER_SIZE); // this will always be true because of modulo in nextState
  dbgf(b2DEBUG_GAMESTATE, "RET(%d)]]", -1);
  return -1;
}

PongGameState* getStateWithID(uint32_t frameID) {
  uint32_t idx=getStateIdxWithID(frameID);
  dbgf(b2DEBUG_GAMESTATE, "Returning state for idx %d\n", idx);
  if (idx==-1) return NULL;
  return &gameStates[idx];
}


/**********
** Game state related routines
**   
***********/
void printGameState(PongGameState *state) {
  dbg(b2DEBUG_GAMESTATE, "GameState: {");
  dbgf3(b2DEBUG_GAMESTATE, "self: {score: %d, pos: %d, dir: %d}, ", state->scoreSelf, state->posSelf, state->dirSelf);
  dbgf3(b2DEBUG_GAMESTATE, "other: {score: %d, pos: %d, dir: %d}, ", state->scoreOther, state->posOther, state->dirOther);
  dbgf4(b2DEBUG_GAMESTATE, "ball: {pos: {x: %d, y: %d}, speed: {x: %d, y: %d}}", state->posBallX, state->posBallY, state->speedBallX, state->speedBallY);
  dbgln(b2DEBUG_GAMESTATE, "}");
}

#define SWAP(var1, var2, tmp) tmp=var1; var1=var2; var2=tmp;

void reverseRoles(PongGameState *state) {
  // we need to reverse the ball direction
  state->speedBallX*=-1; 
  // we need to exchange self and other
  uint32_t temp;
  SWAP(state->scoreSelf, state->scoreOther, temp);
  SWAP(state->posSelf, state->posOther, temp);
  SWAP(state->dirSelf, state->dirOther, temp);
}

PongGameState* copyLatestState() {
  uint32_t idxOld=latestItem;
  uint32_t idxNew=bufferAdd();
  memcpy(&gameStates[idxNew], &gameStates[idxOld], sizeof(PongGameState));
  gameStates[idxNew].frameID=gameStates[idxOld].frameID+1; // even though bufferrAdd fills it, it gets overwritten on memcpy
  return &gameStates[idxNew];
}