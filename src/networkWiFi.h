#include <Arduino.h>
#include "gamestate.h"

#define CONNECT_TIMEOUT 10000

void displayMsg(const char *line1, const char *line2=NULL, const char *line3=NULL);
bool networkInit();
uint32_t getSendingLatency();
uint32_t getReceivingLatency();

void sendMsg(const char *msg);
bool waitMsg(const char *msg, uint32_t timeout = CONNECT_TIMEOUT);
void sendGameState(PongGameState *state);
void waitGameState(PongGameState *state);
void sendDirChg(PongGameState *state);
bool acceptDirChg(uint32_t *fid, int8_t *dir);
void sendPotentialScore(uint32_t frameID, uint32_t lastFrameReceived, uint32_t lastFrameSent);
bool acceptPotentialScore(uint32_t *frameID, uint32_t *lastFrameHandled, uint32_t *lastFrameShouldReceive);
void sendPotentialScoreAck(uint32_t frameID, uint32_t lastFrameReceived, uint32_t lastFrameSent);
bool acceptPotentialScoreAck(uint32_t *frameID, uint32_t *lastFrameHandled, uint32_t *lastFrameShouldReceive);
void sendFinalScore(uint32_t frameID, int8_t scoring);
bool acceptFinalScore(uint32_t *frameID, int8_t *scoring);
