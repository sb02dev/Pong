/**
* Pong
* A remake of the game "Pong" to play on two ESP32 over a WiFi network
* One ESP32 is the master and WiFi AP, the other connects to it.
* Control is through touch interface
* For display we use OLED
*/
#include "Arduino.h"
#include "b2debug.h"

#include "helper.h"

// display
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "img/win.h"
#include "img/lose.h"
SSD1306  display(0x3c, 5, 4);
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
uint centerX = SCREEN_WIDTH/2;
#define PADDLE_WIDTH 3
#define PADDLE_HEIGHT 8
#define BALL_RADIUS 2
#define FRAME_TIME 33333 // 30 fps

// networking
#define SERVERID 514108976
bool isServer;
bool isNetworked;
#include "networkWiFi.h"
#include <queue>

// controls
#define MOVE_SPEED (SCREEN_HEIGHT/1)
#define TOUCHPIN_UP T6
#define TOUCHPIN_DOWN T2
#define TOUCH_SENSITIVITY 80

// game params
#define SCORE_MAX 5
#define SCORE_MINDIFF 2
int8_t scoringSituation=0;
uint32_t scoreCheckingStartFrame=0;
#include "gamestate.h"

// AI state
bool hasPred = false;
int32_t predElapsed = 0;
int32_t predSpeedX;
int32_t predSpeedY;
int32_t predExactX;
int32_t predExactY;
int32_t predPosY;
int32_t aiError = 80; // error factor (depends on how far the ball is)
int32_t aiReaction = 500000; // half of a second "to estimate"
int32_t aiForesee = 1000000; // one second foresee capability

void drawFrame(PongGameState *state) {
	display.clear();

  // draw scores
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(centerX-5,0,String(state->scoreSelf));
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(centerX+5,0,String(state->scoreOther));

  // draw paddles
	display.fillRect(0,state->posSelf/1000-PADDLE_HEIGHT/2,PADDLE_WIDTH,PADDLE_HEIGHT);
	display.fillRect(SCREEN_WIDTH-PADDLE_WIDTH,state->posOther/1000-PADDLE_HEIGHT/2,PADDLE_WIDTH,PADDLE_HEIGHT);

	// draw ball
	display.fillCircle(state->posBallX/1000, state->posBallY/1000, BALL_RADIUS);

	display.display();
}

void getControls(PongGameState* state) {
	state->dirSelf=0;
	uint16_t t1, t2, i;
	t1=0; i=0;
	do {
		t1=std::max(touchRead(TOUCHPIN_UP), t1);
		i++;
		dbgf(b2DEBUG_CONTROLS, "touch1: %d;", t1);
	} while (/*t1<10 ||*/ i<5); // below 10 its measurement error (we do not deal with that, otherwise it can be blocking the game), we measure the maximum of 5 measurements
	t2=0; i=0;
	do {
		t2=std::max(touchRead(TOUCHPIN_DOWN), t2);
		i++;
		dbgf(b2DEBUG_CONTROLS, "touch2: %d;", t2);
	} while (/*t2<10 ||*/ i<5); // below 10 its measurement error (we do not deal with that, otherwise it can be blocking the game), we use the maximum of 5 measurements
	if (t2<TOUCH_SENSITIVITY) { state->dirSelf++; }
	if (t1<TOUCH_SENSITIVITY) { state->dirSelf--; }
}

void moveBall(PongGameState* state, PongGameState* pState, int32_t deltaTime) {
	dbgf(b2DEBUG_MOVEBALL, "moveBall: dT=%d\n", deltaTime);
	// collision detection
  int32_t collisionTime = deltaTime, nextSpeedBallX = pState->speedBallX, nextSpeedBallY = pState->speedBallY;
	if (pState->speedBallY < 0 && checkHCollision(0, BALL_RADIUS*1000, SCREEN_WIDTH*1000,
	                                    pState->posBallX, pState->posBallY, pState->speedBallX, pState->speedBallY,
																			deltaTime, &collisionTime)) { // collide with top
    nextSpeedBallY=-pState->speedBallY;
	} else if (pState->speedBallY > 0 && checkHCollision(0, (SCREEN_HEIGHT-BALL_RADIUS)*1000, SCREEN_WIDTH*1000,
	                                    pState->posBallX, pState->posBallY, pState->speedBallX, pState->speedBallY,
																			deltaTime, &collisionTime)) { // collide with bottom
    nextSpeedBallY=-pState->speedBallY;
	} else if (pState->speedBallX < 0 && checkVCollision((PADDLE_WIDTH+BALL_RADIUS)*1000, pState->posSelf-PADDLE_HEIGHT*500-BALL_RADIUS*1000, pState->posSelf+PADDLE_HEIGHT*500+BALL_RADIUS*1000,
	                                             pState->posBallX, pState->posBallY, pState->speedBallX, pState->speedBallY,
																							 deltaTime, &collisionTime)) { // collide with left paddle
    nextSpeedBallX=-pState->speedBallX; 
		// adjust for moving paddle
		if (state->dirSelf>0)
				nextSpeedBallY = nextSpeedBallY * (nextSpeedBallY < 0 ? 0.5 : 1.5);
		else if (state->dirSelf<0)
          nextSpeedBallY = nextSpeedBallY * (nextSpeedBallY > 0 ? 0.5 : 1.5);
	} else if (pState->speedBallX > 0 && checkVCollision((SCREEN_WIDTH-PADDLE_WIDTH-BALL_RADIUS)*1000, pState->posOther-PADDLE_HEIGHT*500-BALL_RADIUS*1000, pState->posOther+PADDLE_HEIGHT*500+BALL_RADIUS*1000,
	                                             pState->posBallX, pState->posBallY, pState->speedBallX, pState->speedBallY,
																							 deltaTime, &collisionTime)) { // collide with right paddle
    nextSpeedBallX=-pState->speedBallX; 
		// adjust for moving paddle
    if (state->dirOther>0)
      nextSpeedBallY = nextSpeedBallY * (nextSpeedBallY < 0 ? 0.5 : 1.5);
    else if (state->dirOther<0)
      nextSpeedBallY = nextSpeedBallY * (nextSpeedBallY > 0 ? 0.5 : 1.5);
	}
	// move1
	state->posBallX = pState->posBallX + pState->speedBallX * collisionTime / 1000;
	state->posBallY = pState->posBallY + pState->speedBallY * collisionTime / 1000;
	dbg(b2DEBUG_MOVEBALL, "move1:[speed=("); dbg(b2DEBUG_MOVEBALL, state->speedBallX); dbg(b2DEBUG_MOVEBALL, ";"); dbg(b2DEBUG_MOVEBALL, state->speedBallY); dbg(b2DEBUG_MOVEBALL, ") pos=("); dbg(b2DEBUG_MOVEBALL, state->posBallX); dbg(b2DEBUG_MOVEBALL, ";"); dbg(b2DEBUG_MOVEBALL, state->posBallY); dbg(b2DEBUG_MOVEBALL, ")] ");
	// update ball speed according to collision 
	state->speedBallX=nextSpeedBallX;
	state->speedBallY=nextSpeedBallY;
	// move2 (finish the move if collision happens mid-frame)
	state->posBallX += state->speedBallX * (deltaTime-collisionTime) / 1000;
	state->posBallY += state->speedBallY * (deltaTime-collisionTime) / 1000;
	dbg(b2DEBUG_MOVEBALL, "move2:[speed=("); dbg(b2DEBUG_MOVEBALL, state->speedBallX); dbg(b2DEBUG_MOVEBALL, ";"); dbg(b2DEBUG_MOVEBALL, state->speedBallY); dbg(b2DEBUG_MOVEBALL, ") pos=("); dbg(b2DEBUG_MOVEBALL, state->posBallX); dbg(b2DEBUG_MOVEBALL, ";"); dbg(b2DEBUG_MOVEBALL, state->posBallY); dbgln(b2DEBUG_MOVEBALL, ")]");
}

void recalcFrame(PongGameState* curState, PongGameState* pState) {
	int32_t move;
	// default frame time, we don't handle differences
	int32_t deltaTime = FRAME_TIME; 
	// move self
	move = deltaTime * MOVE_SPEED / 1000;
	dbgf(b2DEBUG_RECALCFRAME, "self move amount in this frame: %d\n", move);
	curState->posSelf = pState->posSelf + curState->dirSelf * move; 
	if (curState->posSelf<(PADDLE_HEIGHT*1000/2)) curState->posSelf=(PADDLE_HEIGHT*1000/2);
	if (curState->posSelf>(SCREEN_HEIGHT-PADDLE_HEIGHT/2)*1000) curState->posSelf=(SCREEN_HEIGHT-PADDLE_HEIGHT/2)*1000;
	dbgf(b2DEBUG_RECALCFRAME, "recalculated self position: %d\n", curState->posSelf);
	// move other
	move = deltaTime * MOVE_SPEED / 1000;
	dbgf(b2DEBUG_RECALCFRAME, "other move amount in this frame: %d\n", move);
	curState->posOther = pState->posOther + curState->dirOther * move; 
	if (curState->posOther<(PADDLE_HEIGHT*1000/2)) curState->posOther=(PADDLE_HEIGHT*1000/2);
	if (curState->posOther>(SCREEN_HEIGHT-PADDLE_HEIGHT/2)*1000) curState->posOther=(SCREEN_HEIGHT-PADDLE_HEIGHT/2)*1000;
	dbgf(b2DEBUG_RECALCFRAME, "recalculated other position: %d\n", curState->posOther);
	// move the ball
	moveBall(curState, pState, deltaTime);
}

int8_t checkScoreSituation(PongGameState *state) { // returns 1: we won a point, 0: no scoring, -1: we lost a point
	if (state->posBallX>(SCREEN_WIDTH+BALL_RADIUS)*1000) {
		return 1;
	} else if (state->posBallX<-BALL_RADIUS) {
		return -1;
	} else {
		return 0;
	}
}

std::queue<PongDirChangeMsg> futureMsgs;
void commNetwork(PongGameState *state, PongGameState *pState) {
	static uint32_t lastFrameSent = 0, lastFrameReceived = 0;
	// send frameid + self direction if changed 
	if (state->dirSelf != pState->dirSelf) {
		sendDirChg(state);
		lastFrameSent = state->frameID;
	}
	// see if have buffered (future) frames we should handle already
	while (futureMsgs.front().frameID<=state->frameID) {
		PongDirChangeMsg msg = futureMsgs.front();
		PongGameState *st = getStateWithID(msg.frameID);
		if (!st) { // this can happen in only one case: message arrived that late that it is out of buffer now
			for (int idx=0; idx<100/*GAMESTATE_BUFFER_SIZE*/; idx++) dbgf(b2DEBUG_WIFI, "\t%d", getState(idx)->frameID);
			// TODO: no calculation is possible on this side, request full game state
			// TODO: do we do this on both sides or only client? -> server should have authority
		} 
		PongGameState *pSt = prevState(st);
		while (st) {
			dbgf4(b2DEBUG_WIFI, "State pointer: %p (frameID: %d), prev state pointer: %p (frameID: %d). ", st, st?st->frameID:0, pSt, pSt?pSt->frameID:0);
			st->dirOther = msg.direction; // we recalculate all frames until current (because we are using TCP connection which guarantees the order of packets, which in turn guarantees that we have not received any messages from a later frame than the current message)
			recalcFrame(st, pSt);
			dbgf2(b2DEBUG_WIFI, "New posself: %d, posother: %d. ", st->posSelf, st->posOther);
			pSt = st;
			st = nextState(st);
		}
		dbgf2(b2DEBUG_WIFI, "Final new posself: %d, posother: %d. ", state->posSelf, state->posOther);
		dbgln(b2DEBUG_WIFI, "");
		futureMsgs.pop();
	}
	// see if received other direction
	uint32_t fid; int8_t dir;
	if (acceptDirChg(&fid, &dir)) {
		lastFrameReceived = fid;
		if (fid>state->frameID) {
			// buffer future frames
			PongDirChangeMsg msg;
			msg.frameID=fid; msg.direction=dir;
			futureMsgs.push(msg);
			dbgf2(b2DEBUG_WIFI, "Current frame is %d. Buffering frame %d", state->frameID, fid);
		} else {
			dbgf4(b2DEBUG_WIFI, "Current frame is %d. Recalculating from frame %d, posself: %d, posother: %d. ", state->frameID, fid, state->posSelf, state->posOther);
			// if yes, recalculate all frames from that id
			PongGameState *st = getStateWithID(fid);
			if (!st) { // this can happen in only one case: message arrived that late that it is out of buffer now
				for (int idx=0; idx<100/*GAMESTATE_BUFFER_SIZE*/; idx++) dbgf(b2DEBUG_WIFI, "\t%d", getState(idx)->frameID);
				// TODO: no calculation is possible on this side, request full game state
				// TODO: do we do this on both sides or only client? -> server should have authority
			} 
			PongGameState *pSt = prevState(st);
			while (st) {
				dbgf4(b2DEBUG_WIFI, "State pointer: %p (frameID: %d), prev state pointer: %p (frameID: %d). ", st, st?st->frameID:0, pSt, pSt?pSt->frameID:0);
				st->dirOther = dir; // we recalculate all frames until current (because we are using TCP connection which guarantees the order of packets, which in turn guarantees that we have not received any messages from a later frame than the current message)
				recalcFrame(st, pSt);
				dbgf2(b2DEBUG_WIFI, "New posself: %d, posother: %d. ", st->posSelf, st->posOther);
				pSt = st;
				st = nextState(st);
			}
			dbgf2(b2DEBUG_WIFI, "Final new posself: %d, posother: %d. ", state->posSelf, state->posOther);
			dbgln(b2DEBUG_WIFI, "");
		}
	}
	if (!isServer) {
  	// if we are a client see if received score frame (server sends score frame from checkScore)
		uint32_t fid, lastFrameHandled, lastFrameShouldReceive;
		if (acceptPotentialScore(&fid, &lastFrameHandled, &lastFrameShouldReceive)) {
			// check if all commands from client was handled on server and all commands from server was handled on client
			// since we are on TCP which guarantees message order we can not have unhandled server messages on client
			if (lastFrameShouldReceive>lastFrameReceived) {
				// this is a real surprise, means server messages are out of order
				// we should just crash / reboot here
				ESP.restart();
				delay(1000);
			}
			// we can just send the acknowledge message because TCP guarantees message order
			sendPotentialScoreAck(state->frameID, lastFrameReceived, lastFrameSent);
		}
		int8_t scoring;
		if (acceptFinalScore(&fid, &scoring)) {
			// we signal to the checkScore routine
			scoringSituation=-scoring; // need to reverse the roles in scoring direction
		}
	} else {
		if (scoreCheckingStartFrame==0) {
			int8_t scoring = checkScoreSituation(state);
			if (scoring!=0) {
				// we have a potential scoring situation
				sendPotentialScore(state->frameID, lastFrameReceived, lastFrameSent);
				scoreCheckingStartFrame=state->frameID;
			}
		} else {
  		uint32_t fid, lastFrameHandled, lastFrameShouldReceive;
			if (acceptPotentialScoreAck(&fid, &lastFrameHandled, &lastFrameShouldReceive)) {
				scoreCheckingStartFrame=0; // signal for future self that we restarted score checking
				int8_t scoring=checkScoreSituation(state);
				if (scoring!=0) {
					scoringSituation=scoring; // signal for checkScore to show win/lose screen
					sendFinalScore(state->frameID, scoring);
				}
			} else {
				if (state->frameID-scoreCheckingStartFrame > 90) { // 3 seconds timeout
				  // on timeout we can safely assume that we lost connection so restart to fall back to local game
					ESP.restart();
					delay(1000);
				}
			}
		}
	}
}

bool predict(int32_t deltaTime, PongGameState *state) {
  // only re-predict if the ball changed direction, or its been some amount of time since last prediction
  if (hasPred && // we have earlier prediction
			((predSpeedX * state->speedBallX) > 0) && // no direction change since then
			((predSpeedY * state->speedBallY) > 0) && // no direction change since then
			(predElapsed < aiReaction)) { // and we are not yet recalibrating
		predElapsed += deltaTime; // some time elapsed
		return true;
	}

  int32_t interceptTime = deltaTime;
	int32_t intX = (SCREEN_WIDTH-PADDLE_WIDTH)*1000;
	int32_t intY;
  bool intercept = checkVCollision(intX, -10000000, 10000000,
																	 state->posBallX, state->posBallY, state->speedBallX, state->speedBallY,
																	 aiForesee, &interceptTime); 
	if (intercept) {
		intY = state->posBallY+interceptTime*state->speedBallY/1000;
		dbgf5(b2DEBUG_AIPRED, "ball: (%d;%d) ballspeed: (%d;%d) intercept: y=%d", state->posBallX, state->posBallY, state->speedBallX, state->speedBallY, intY);

    int32_t t = 0 + BALL_RADIUS*1000; //this.minY + ball.radius;
    int32_t b = (SCREEN_HEIGHT - 0 - PADDLE_HEIGHT + PADDLE_HEIGHT - BALL_RADIUS) * 1000; //this.maxY + this.height - ball.radius;

		while ((intY < t) || (intY > b)) {
			if (intY < t) {
				intY = t + (t - intY);
			} else if (intY > b) {
				intY = t + (b - t) - (intY - b);
			}
		}
		hasPred = true;
		dbgf(b2DEBUG_AIPRED," intercept adjusted: y=%d", intY);
	} else {
		hasPred = false;
	}

	if (hasPred) {
		predElapsed = 0;
		predSpeedX = state->speedBallX;
		predSpeedY = state->speedBallY;
		predExactX = intX;
		predExactY = intY;
		int32_t closeness = (predSpeedX < 0 ? state->posBallX - (SCREEN_WIDTH*1000) : (SCREEN_WIDTH-PADDLE_WIDTH)*1000 - state->posBallX) / SCREEN_WIDTH;
		int32_t error = aiError * closeness;
		predPosY = predExactY + random(-error, error);
		dbgf5(b2DEBUG_AIPRED," prediction: exact=(%d;%d) y=%d closeness=%d error=%d\n", predExactX, predExactY, predPosY, closeness, error);
	}
}

void calcAI(PongGameState *state, PongGameState *pState) {
	// calc deltaTime
	int32_t deltaTime = FRAME_TIME;
	// don't do any AI thing if ball is over the other side of the paddle
	if (((pState->posBallX < (SCREEN_WIDTH-PADDLE_WIDTH)*1000) && (pState->speedBallX < 0)) ||
			((pState->posBallX > SCREEN_WIDTH*1000) && (pState->speedBallX > 0))) {
		state->dirOther=0;
		return;
	}
  // predict the ball position
	predict(deltaTime, pState);
	// handle prediction to movement conversion
	if (hasPred) {
		if (predPosY < pState->posOther - 5000) {
			state->dirOther=-1;
		} else if (predPosY > pState->posOther + 5000) {
			state->dirOther=1;
		} else {
			state->dirOther=0;
		}
		dbgf3(b2DEBUG_AIMOVE, "predicted pos: %d, paddle pos: %d, AI move dir: %d\n", predPosY, pState->posOther, state->dirOther);
	} else state->dirOther = 0; // no prediction no move
}

void initRound(bool lost) {	
	// clear the gamestate buffer and add our first frame while carrying on the scores
	uint32_t scoreSelf = curState()->scoreSelf, scoreOther = curState()->scoreOther;
	initBuffer(); 
	bufferAdd(); 
	curState()->scoreSelf = scoreSelf; curState()->scoreOther = scoreOther;
	// initialize the new round
	scoringSituation=0;
	if (isServer || ! isNetworked) { // server or local
		float angle;
		if (lost)
			angle=random(60)-30;
		else
			angle=random(60)+150;
		PongGameState *state=curState();
		state->speedBallX=45*cos(angle*PI/180);
		state->speedBallY=45*sin(angle*PI/180);
		state->posBallX=SCREEN_WIDTH*500;
		state->posBallY=SCREEN_HEIGHT*500;
		state->posOther = SCREEN_HEIGHT/2*1000;
		if (isNetworked) {
			printGameState(state);
			sendGameState(state);
			waitMsg("ACK");
			// handle latency (fast forward some frames)
			PongGameState *curstate=curState();
			for (int i=getReceivingLatency() / FRAME_TIME; i>0; i--) {
				PongGameState* newstate=copyLatestState();
				recalcFrame(newstate, curstate);
				curstate=newstate;
			}
		}
	} else { // client
		waitGameState(curState());
		sendMsg("ACK");
		printGameState(curState());
		reverseRoles(curState());
	}
}

/*****************************************************************************************
 * Network scoring:                                                                      *
 *   Server                                   Client                                     *
 *    - sees a potential scoring situation                                               *
 *    - sends a "potential score" message       - gets a potential score message         *
 *    - continues to process frames until       - checks if out of order and reboots to  *
 *      all frame messages are processed          fall back to local game                *
 *      and gets the "score acknowledge"        - sends "score acknowledge" message      *
 *      message                                                                          *
 *    - checks timeout and reboots to fall                                               *
 *      back to local game                                                               *
 *    - checks if score situation is still                                               *
 *      existing                                                                         *
 *    - sends a "final score" message          - if gets a final score message shows     *
 *    - shows win/lose scren or starts new       win/lose screen or starts new round     *
 *      round                                                                            *
 *****************************************************************************************/

void checkScore(PongGameState *state) {
	int8_t scoring=scoringSituation;
	if (!isNetworked) scoring=checkScoreSituation(state); // if it is local we check the situation here
	if (scoring!=0) {
		if (scoring<0) state->scoreOther++; else state->scoreSelf++;
		dbgf2(b2DEBUG_SCORE, "Scored: %d vs %d\n", state->scoreSelf, state->scoreOther);
		// check if game is over
		int32_t h, w; const uint8_t *p=NULL;
		if (state->scoreSelf>=SCORE_MAX && state->scoreSelf>=state->scoreOther+SCORE_MINDIFF) {
			h=win_height; w=win_width; p=win_bits;
		}
		if (state->scoreOther>=SCORE_MAX && state->scoreOther>=state->scoreSelf+SCORE_MINDIFF) {
			h=lose_height; w=lose_width; p=lose_bits;
		}
		if (p) {
		  dbgf2(b2DEBUG_SCORE, "Game ended: %d vs %d\n", state->scoreSelf, state->scoreOther);
			// display the icon
			display.clear();
			display.drawXbm((SCREEN_WIDTH - w)/2, (SCREEN_HEIGHT - h)/2, w, h, p);
			display.display();
			// wait a fixed amount of time (because touch will be still on when we get here: player will still be controlling the paddle)
			delay(3000);
			// wait for touch
			uint16_t t1, t2, i;
			do {
				t1=0; i=0;
				do {
					t1=std::max(touchRead(TOUCHPIN_UP), t1);
					i++;
					dbgf(b2DEBUG_SCORE, "touch1: %d;", t1);
				} while (t1<10 || i<5); // below 10 its measurement error, we measure the maximum of 5 measurements
				t2=0; i=0;
				do {
					t2=std::max(touchRead(TOUCHPIN_DOWN), t2);
					i++;
					dbgf(b2DEBUG_SCORE, "touch2: %d;", t2);
				} while (t2<10 || i<5); // below 10 its measurement error, we use the maximum of 5 measurements
				dbgf3(b2DEBUG_SCORE, "touch1: %d; touch2: %d; sensi: %d\n", t1, t2, TOUCH_SENSITIVITY);
				delay(1);
			} while (t1>TOUCH_SENSITIVITY && t2>TOUCH_SENSITIVITY);
			state->scoreSelf=0;
			state->scoreOther=0;
		}
		// restart game
		initRound(scoring<0);
	}
}

void setup()
{
	// init board
	dbgstart();
	display.init();
  isServer = ((uint32_t)ESP.getEfuseMac())==SERVERID;
	// init network and bail out if connection fails
	isNetworked=networkInit();
	// init game
	initRound(false);
}

void loop()
{
	uint32_t st = micros();
	// get the state
	PongGameState* previousState=curState();
	// draw the latest gamestate on the display
  drawFrame(previousState); 
	// create a new gamestate
	PongGameState* state=copyLatestState();
	// get the controls >>modifies dirSelf
	getControls(state); 
	// get the opponents move (and send ours) >>modifies dirOther
	if (isNetworked) commNetwork(state, previousState); // if we got message from network for old frames we also recalculate from there
	else calcAI(state, previousState);
	// recalculate the frame >>moves paddles and ball
	recalcFrame(state, previousState);
	// check the scoring state (and communicate to network opponent)
	checkScore(state);
	uint32_t elapsed = micros() - st;
	#ifdef b2DEBUG_FPS 
	  display.setFont(ArialMT_Plain_10);
	  display.setTextAlignment(TEXT_ALIGN_LEFT);
	  display.drawString(0,0,String(1000000/elapsed)); // display fps
	  display.display();
	  elapsed = micros() - st;
	#endif
	if (FRAME_TIME > elapsed) // we are over our frame time -> no wait
    delayMicroseconds(FRAME_TIME-elapsed); // wait until frame should end
}