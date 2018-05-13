// debug bits
#define b2DEBUG_MOVEBALL 0b1
#define b2DEBUG_CONTROLS 0b10
#define b2DEBUG_COLLISION 0b100
#define b2DEBUG_AIPRED 0b1000
#define b2DEBUG_AIMOVE 0b10000
#define b2DEBUG_SCORE 0b100000
#define b2DEBUG_WIFI 0b1000000
#define b2DEBUG_GAMESTATE 0b10000000
#define b2DEBUG_RECALCFRAME 0b100000000
// debug mask
//#define b2DEBUG (b2DEBUG_WIFI | b2DEBUG_SCORE)
//#define b2DEBUG_FPS 
// debug macros
#ifdef b2DEBUG
#define dbgstart() Serial.begin(115200)
#define dbg(bit, x) if (b2DEBUG & bit) Serial.print(x)
#define dbgln(bit, x) if (b2DEBUG & bit) Serial.println(x)
#define dbgf(bit, fmt, par) if (b2DEBUG & bit) Serial.printf(fmt, par)
#define dbgf2(bit, fmt, par1, par2) if (b2DEBUG & bit) Serial.printf(fmt, par1, par2)
#define dbgf3(bit, fmt, par1, par2, par3) if (b2DEBUG & bit) Serial.printf(fmt, par1, par2, par3)
#define dbgf4(bit, fmt, par1, par2, par3, par4) if (b2DEBUG & bit) Serial.printf(fmt, par1, par2, par3, par4)
#define dbgf5(bit, fmt, par1, par2, par3, par4, par5) if (b2DEBUG & bit) Serial.printf(fmt, par1, par2, par3, par4, par5)
#else
#define dbgstart()
#define dbg(bit, x)
#define dbgln(bit, x)
#define dbgf(bit, fmt, par)
#define dbgf2(bit, fmt, par1, par2) 
#define dbgf3(bit, fmt, par1, par2, par3) 
#define dbgf4(bit, fmt, par1, par2, par3, par4) 
#define dbgf5(bit, fmt, par1, par2, par3, par4, par5) 
#endif
