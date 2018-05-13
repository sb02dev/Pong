#include "networkWiFi.h"

#include "b2debug.h"

#include "SSD1306.h"

extern SSD1306  display;
#include <WiFi.h>
#include <WiFiClient.h>

#define PORT 5263
#define CALIBRATION_COUNT 20

extern bool isServer;

const char* ssid = "ESPPong";
const char* password = "FhDjSkAl";
const char* host = "192.168.4.1";

const char* CMD_FULLGAMESTATE="FLGMST";
const char* CMD_CHGDIR="C";
const char* CMD_POTENTIALSCORE="P";
const char* CMD_POTENTIALSCOREACK="Q";
const char* CMD_FINALSCORE="F";

WiFiServer srv(PORT);
WiFiClient clnt;

uint32_t sendingLatency;
uint32_t receivingLatency;

bool networkInit() {
  uint32_t roundtime[CALIBRATION_COUNT], minLatency, maxLatency;
  if (isServer) {
    /************
    ** Server side connection code
    *************/
    displayMsg("Acting as SERVER", "Starting Access Point");
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(ssid, password)) { // create access point
      dbg(b2DEBUG_WIFI, "Access point started, IP address: ");
      dbgln(b2DEBUG_WIFI, WiFi.softAPIP());
      displayMsg("Acting as SERVER", "Access pt started, IP:", WiFi.softAPIP().toString().c_str());
      // waiting for socket connection
      srv.begin();
      dbgln(b2DEBUG_WIFI, "Waiting for socket connection...");
      displayMsg("Acting as SERVER", "Waiting for socket conn");
      uint32_t start=millis();
      do {
        if (millis()-start > CONNECT_TIMEOUT) break;
        clnt=srv.available();
        delay(100);
      } while (!clnt);
      if (clnt) {
        dbg(b2DEBUG_WIFI, "Client connected from IP: ");
        dbgln(b2DEBUG_WIFI, clnt.remoteIP());
        displayMsg("Acting as SERVER", "Client connected, IP:", clnt.remoteIP().toString().c_str());
        // calibrate latency
        sendingLatency=0; 
        minLatency = -1; // max unsigned int
        maxLatency = 0; // min unsigned int
        for (int i=0; i<CALIBRATION_COUNT; i++) {
          roundtime[i]=micros();
          sendMsg("CALIBREQU");
          waitMsg("CALIBRESP", CONNECT_TIMEOUT);
          roundtime[i]=micros()-roundtime[i];
          minLatency = std::min(minLatency, roundtime[i]); // calc the lowest (to drop later from average)
          maxLatency = std::max(maxLatency, roundtime[i]); // calc the highest (to drop later from average)
          dbgf2(b2DEBUG_WIFI, "\tRoundtime[%d]: %d\n", i, roundtime[i]);
          displayMsg("Acting as SERVER", "Calibrating latency", String(i).c_str());
        }
        for (int i=0; i<CALIBRATION_COUNT; i++)
          if (roundtime[i] != minLatency && roundtime[i] != maxLatency)
            sendingLatency+=roundtime[i];
        sendingLatency/=(CALIBRATION_COUNT-2)*2; // half of the average roundtrip
        sendMsg("CALIBDONE");
        receivingLatency=0;
        minLatency = -1; // max unsigned int
        maxLatency = 0; // min unsigned int
        for (int i=0; i<CALIBRATION_COUNT; i++) {
          roundtime[i]=micros();
          waitMsg("CALIBREQU", CONNECT_TIMEOUT);
          sendMsg("CALIBRESP");
          roundtime[i]=micros()-roundtime[i];
          minLatency = std::min(minLatency, roundtime[i]); // calc the lowest (to drop later from average)
          maxLatency = std::max(maxLatency, roundtime[i]); // calc the highest (to drop later from average)
          dbgf2(b2DEBUG_WIFI, "\tRoundtime[%d]: %d\n", i, roundtime[i]);
          displayMsg("Acting as SERVER", "Calibrating latency", String(i).c_str());
        }
        for (int i=0; i<CALIBRATION_COUNT; i++)
          if (roundtime[i] != minLatency && roundtime[i] != maxLatency)
            receivingLatency+=roundtime[i];
        receivingLatency/=(CALIBRATION_COUNT-2)*2; // half of the average roundtrip
        waitMsg("CALIBDONE", CONNECT_TIMEOUT);
        dbgf2(b2DEBUG_WIFI, "Calibration done, sending latency: %d, receiving latency: %d", sendingLatency, receivingLatency);
      } else {
        dbgln(b2DEBUG_WIFI, "No client connection, fallback to local game");
        displayMsg("Acting as SERVER", "Fallback to local game");
        WiFi.enableAP(false);
        WiFi.enableSTA(false);
        WiFi.mode(WIFI_OFF);
        delay(1000); // just wait a little
        return false;
      }
    } else {
      dbgln(b2DEBUG_WIFI, "Access point could not be created");
      return false;
    }
  } else {
    /************
    ** Client side connection code
    *************/
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    dbg(b2DEBUG_WIFI, "Trying to connect to server...");
    displayMsg("Acting as CLIENT", "Connecting to WiFi");
    uint32_t start=millis();
    do {
      if (millis()-start>CONNECT_TIMEOUT) break;
      delay(100);
      dbg(b2DEBUG_WIFI, ".");
    } while (WiFi.status() != WL_CONNECTED);
    dbgln(b2DEBUG_WIFI, "");
    dbg(b2DEBUG_WIFI, "Connected to WiFi, IP: "); dbgln(b2DEBUG_WIFI, WiFi.localIP());
    displayMsg("Acting as CLIENT", "Connected to WiFi, IP:", WiFi.localIP().toString().c_str());
    if (WiFi.status()==WL_CONNECTED) {
      dbg(b2DEBUG_WIFI, "Connecting to server...");
      clnt.connect(host, PORT);
      start=millis();
      do {
        if (millis()-start>CONNECT_TIMEOUT) break;
        delay(100);
        dbg(b2DEBUG_WIFI, ".");
      } while (!clnt.connected());
      dbgln(b2DEBUG_WIFI, "");
      if (clnt.connected()) {
        dbgln(b2DEBUG_WIFI, "Connected, calibrating latency");
        displayMsg("Acting as CLIENT", "Connected, calibrating latency");
        receivingLatency=0;
        minLatency = -1; // max unsigned int
        maxLatency = 0; // min unsigned int
        for (int i=0; i<CALIBRATION_COUNT; i++) {
          roundtime[i]=micros();
          waitMsg("CALIBREQU", CONNECT_TIMEOUT);
          sendMsg("CALIBRESP");
          roundtime[i]=micros()-roundtime[i];
          minLatency = std::min(minLatency, roundtime[i]); // calc the lowest (to drop later from average)
          maxLatency = std::max(maxLatency, roundtime[i]); // calc the highest (to drop later from average)
          dbgf2(b2DEBUG_WIFI, "\tRoundtime[%d]: %d\n", i, roundtime[i]);
          displayMsg("Acting as CLIENT", "Calibrating latency", String(i).c_str());
        }
        for (int i=0; i<CALIBRATION_COUNT; i++)
          if (roundtime[i] != minLatency && roundtime[i] != maxLatency)
            receivingLatency+=roundtime[i];
        receivingLatency/=(CALIBRATION_COUNT-2)*2; // half of the average roundtrip
        waitMsg("CALIBDONE", CONNECT_TIMEOUT);
        sendingLatency=0;
        minLatency = -1; // max unsigned int
        maxLatency = 0; // min unsigned int
        for (int i=0; i<CALIBRATION_COUNT; i++) {
          roundtime[i]=micros();
          sendMsg("CALIBREQU");
          waitMsg("CALIBRESP", CONNECT_TIMEOUT);
          roundtime[i]=micros()-roundtime[i];
          minLatency = std::min(minLatency, roundtime[i]); // calc the lowest (to drop later from average)
          maxLatency = std::max(maxLatency, roundtime[i]); // calc the highest (to drop later from average)
          dbgf2(b2DEBUG_WIFI, "\tRoundtime[%d]: %d\n", i, roundtime[i]);
          displayMsg("Acting as CLIENT", "Calibrating latency", String(i).c_str());
        }
        for (int i=0; i<CALIBRATION_COUNT; i++)
          if (roundtime[i] != minLatency && roundtime[i] != maxLatency)
            sendingLatency+=roundtime[i];
        sendingLatency/=(CALIBRATION_COUNT-2)*2; // half of the average roundtrip
        sendMsg("CALIBDONE");
        dbgf2(b2DEBUG_WIFI, "Calibration done, sending latency: %d, receiving latency: %d", sendingLatency, receivingLatency);
      } else {
        dbgln(b2DEBUG_WIFI, "Connection timed out, fallback to local game");
        displayMsg("Acting as CLIENT", "Connection timeout, fallback to local");
        WiFi.enableAP(false);
        WiFi.enableSTA(false); // we should turn off wifi
        WiFi.mode(WIFI_OFF);
        return false;
      }
    } else {
      dbgln(b2DEBUG_WIFI, "Connection timed out, fallback to local game");
      displayMsg("Acting as CLIENT", "Connection timeout, fallback to local");
      WiFi.enableAP(false);
      WiFi.enableSTA(false); // we should turn off wifi
      WiFi.mode(WIFI_OFF);
      return false;
    }
  }
  return true;
}

uint32_t getSendingLatency() { return sendingLatency; }
uint32_t getReceivingLatency() { return receivingLatency; }

void displayMsg(const char *line1, const char *line2, const char *line3) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (line1!=NULL) display.drawString(0,0,line1);
  if (line2!=NULL) display.drawString(0,10,line2);
  if (line3!=NULL) display.drawString(0,20,line3);
  display.display();
}

void sendMsg(const char *msg) {
  dbg(b2DEBUG_WIFI, ("Sending message '"+String(msg)+"'. ").c_str());
  clnt.write_P(msg, strlen(msg));
  dbgln(b2DEBUG_WIFI, "Message sent.");
}

bool waitMsg(const char *msg, uint32_t timeout) {
  dbg(b2DEBUG_WIFI, ("Waiting for message '"+String(msg)+"'. ").c_str());
  char buf[10];
  memset(buf, 0, 10);
  uint32_t rbytes=0, size=strlen(msg), start=millis();
  if (size>9) size=9; // to not overflow the buffer
  while (rbytes<size) {
    if (millis()-start>timeout) {
      dbgln(b2DEBUG_WIFI, "Timed out.");
      return false; // message did not arrive in time
    }
    if (clnt.available()) {
      buf[rbytes++]=clnt.read();
      if (memcmp(buf, msg, rbytes)!=0) {
        dbg(b2DEBUG_WIFI, ("Received something else: "+String(buf)).c_str());
        rbytes=0; // if so far does not compare, start over
      }
    }
  }
  dbgln(b2DEBUG_WIFI, "Received.");
  return true; // if we got here, full size compares good
}

void sendGameState(PongGameState *state) {
  dbg(b2DEBUG_WIFI, "Sending game state. ");
  clnt.write_P(CMD_FULLGAMESTATE, strlen(CMD_FULLGAMESTATE));
  dbg(b2DEBUG_WIFI, "Header sent. ");
  clnt.write_P((const char *)state, sizeof(PongGameState));
  dbgln(b2DEBUG_WIFI, "State sent.");
}

void waitGameState(PongGameState *state) {
  dbg(b2DEBUG_WIFI, "Waiting for game state. ");
  waitMsg(CMD_FULLGAMESTATE);
  dbg(b2DEBUG_WIFI, "Header received. ");
  clnt.readBytes((uint8_t *)state, sizeof(PongGameState));
  dbgln(b2DEBUG_WIFI, "State received.");
}

void sendDirChg(PongGameState *state) {
  dbg(b2DEBUG_WIFI, "Sending direction change. ");
  clnt.write_P(CMD_CHGDIR, strlen(CMD_CHGDIR));
  dbg(b2DEBUG_WIFI, "Header sent. ");
  clnt.write_P((const char *)&state->frameID, sizeof(state->frameID));
  clnt.write_P((const char *)&state->dirSelf, sizeof(state->dirSelf));
  dbgf2(b2DEBUG_WIFI, "Direction change sent, frameID: %d, direction: %d\n", state->frameID, state->dirSelf);  
}

bool acceptDirChg(uint32_t *fid, int8_t *dir) {
  if (clnt.available()>=strlen(CMD_CHGDIR)+sizeof(uint32_t)+sizeof(uint8_t)) {
    char c=clnt.peek();
    if (c==CMD_CHGDIR[0]) { // one byte command to be able to use peek
      clnt.read(); // pop command from buffer
      clnt.readBytes((uint8_t*)fid, sizeof(*fid));
      *dir=clnt.read();
      dbgf2(b2DEBUG_WIFI, "Direction change received, frameID: %d, direction: %d\n", *fid, *dir);
      return true;
    }
  }
  return false;
}

void sendPotentialScore(uint32_t frameID, uint32_t lastFrameReceived, uint32_t lastFrameSent) {
  dbg(b2DEBUG_WIFI, "Sending potential score. ");
  clnt.write_P(CMD_POTENTIALSCORE, strlen(CMD_POTENTIALSCORE));
  dbg(b2DEBUG_WIFI, "Header sent. ");
  clnt.write_P((const char *)&frameID, sizeof(frameID));
  clnt.write_P((const char *)&lastFrameReceived, sizeof(lastFrameReceived));
  clnt.write_P((const char *)&lastFrameSent, sizeof(lastFrameSent));
  dbgf3(b2DEBUG_WIFI, "Potential score sent (12 bytes). frameids: %d, %d, %d\n", frameID, lastFrameReceived, lastFrameSent);  
}

bool acceptPotentialScore(uint32_t *frameID, uint32_t *lastFrameHandled, uint32_t *lastFrameShouldReceive) {
  if (clnt.available()>=strlen(CMD_POTENTIALSCORE)+sizeof(uint32_t)*3) {
    char c=clnt.peek();
    if (c==CMD_POTENTIALSCORE[0]) { // one byte command to be able to use peek
      clnt.read(); // pop command from buffer
      clnt.readBytes((uint8_t*)frameID, sizeof(*frameID));
      clnt.readBytes((uint8_t*)lastFrameHandled, sizeof(*lastFrameHandled));
      clnt.readBytes((uint8_t*)lastFrameShouldReceive, sizeof(*lastFrameShouldReceive));
      dbgf3(b2DEBUG_WIFI, "Potential score received, frameIDs: %d, %d, %d\n", *frameID, *lastFrameHandled, *lastFrameShouldReceive);
      return true;
    }
  }
  return false;
}

void sendPotentialScoreAck(uint32_t frameID, uint32_t lastFrameReceived, uint32_t lastFrameSent) {
  dbg(b2DEBUG_WIFI, "Sending potential score acknowledgment. ");
  clnt.write_P(CMD_POTENTIALSCOREACK, strlen(CMD_POTENTIALSCOREACK));
  dbg(b2DEBUG_WIFI, "Header sent. ");
  clnt.write_P((const char *)&frameID, sizeof(frameID));
  clnt.write_P((const char *)&lastFrameReceived, sizeof(lastFrameReceived));
  clnt.write_P((const char *)&lastFrameSent, sizeof(lastFrameSent));
  dbgf3(b2DEBUG_WIFI, "Potential score acknowledgement sent (12 bytes). frameID: %d, %d, %d\n", frameID, lastFrameReceived, lastFrameSent);  
}

bool acceptPotentialScoreAck(uint32_t *frameID, uint32_t *lastFrameHandled, uint32_t *lastFrameShouldReceive) {
  if (clnt.available()>=strlen(CMD_POTENTIALSCOREACK)+sizeof(uint32_t)*3) {
    char c=clnt.peek();
    if (c==CMD_POTENTIALSCOREACK[0]) { // one byte command to be able to use peek
      clnt.read(); // pop command from buffer
      clnt.readBytes((uint8_t*)frameID, sizeof(*frameID));
      clnt.readBytes((uint8_t*)lastFrameHandled, sizeof(*lastFrameHandled));
      clnt.readBytes((uint8_t*)lastFrameShouldReceive, sizeof(*lastFrameShouldReceive));
      dbgf3(b2DEBUG_WIFI, "Potential score acknowledgement received, frameID: %d, %d, %d\n", *frameID, *lastFrameHandled, *lastFrameShouldReceive);
      return true;
    }
  }
  return false;
}

void sendFinalScore(uint32_t frameID, int8_t scoring) {
  dbg(b2DEBUG_WIFI, "Sending final score. ");
  clnt.write_P(CMD_POTENTIALSCOREACK, strlen(CMD_POTENTIALSCOREACK));
  dbg(b2DEBUG_WIFI, "Header sent. ");
  clnt.write_P((const char *)&frameID, sizeof(frameID));
  clnt.write_P((const char *)&scoring, sizeof(scoring));
  dbgf2(b2DEBUG_WIFI, "Scoring sent (5 bytes). frameID: %d, scoring: %d\n", frameID, scoring);  
}

bool acceptFinalScore(uint32_t *frameID, int8_t *scoring) {
  if (clnt.available()>=strlen(CMD_FINALSCORE)+sizeof(int8_t)) {
    char c=clnt.peek();
    if (c==CMD_POTENTIALSCOREACK[0]) { // one byte command to be able to use peek
      clnt.read(); // pop command from buffer
      clnt.readBytes((uint8_t*)frameID, sizeof(*frameID));
      clnt.readBytes((uint8_t*)scoring, sizeof(*scoring));
      dbgf2(b2DEBUG_WIFI, "Final score received, frameID: %d, scoring: %d\n", *frameID, *scoring);
      return true;
    }
  }
  return false;
}
