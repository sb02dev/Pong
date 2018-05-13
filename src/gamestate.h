#ifndef __GAMESTATE_H__
#define __GAMESTATE_H__


#include <Arduino.h>

// game state
struct PongGameState {
  uint32_t frameID;
  uint8_t scoreSelf;
  uint8_t scoreOther;
  int32_t posSelf;
  int8_t dirSelf;
  int32_t posOther;
  int8_t dirOther;
  int32_t posBallX;
  int32_t posBallY;
  int32_t speedBallX;
  int32_t speedBallY;
};
typedef struct PongGameState PongGameState;

struct PongDirChangeMsg {
  uint32_t frameID;
  int8_t direction;
};
typedef struct PongDirChangeMsg PongDirChangeMsg;

void initBuffer();
bool bufferEmpty();
bool bufferFull();
uint32_t bufferAdd();

PongGameState* curState();
PongGameState* oldestState();
PongGameState* getState(uint32_t idx);

uint32_t nextState(uint32_t idx);
PongGameState* nextState(PongGameState* state);
uint32_t prevState(uint32_t idx);
PongGameState* prevState(PongGameState* state);
uint32_t getStateIdx(PongGameState* state);
uint32_t getStateIdxWithID(uint32_t frameID);
PongGameState* getStateWithID(uint32_t frameID);

void printGameState(PongGameState *state);
void reverseRoles(PongGameState *state);

PongGameState* copyLatestState();

#endif //__GAMESTATE_H__
