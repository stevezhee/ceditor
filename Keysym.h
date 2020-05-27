//
//  Keysym.h
//  editordebugger
//
//  Created by Brett Letner on 5/17/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#ifndef Keysym_h
#define Keysym_h

#include <SDL.h>
#include "Util.h"

typedef void (*keyHandler_t)(state_t *st, uchar c);
extern keyHandler_t keyHandler[NUM_MODES][NUM_KEYS];
void initMacros(void);

void keysymInit(void);
uchar getKeyChar(SDL_Keycode c);

void doNothing(state_t *st, uchar c);
void setNavigateModeAndDoKeyPress(state_t *st, uchar c);
void forwardEOF(state_t *st);
void backwardSOF(state_t *st);
void setSearchMode(state_t *st);
void gotoView(state_t *st);
void backwardChar(state_t *st);
void forwardChar(state_t *st);
void backwardLine(state_t *st);
void forwardLine(state_t *st);
void forwardEOL(state_t *st);
void backwardSOL(state_t *st);
void startStopRecording(state_t *st);
void playMacro(state_t *st);
void pasteBefore(state_t *st);
void forwardSearch(state_t *st);
void backwardSearch(state_t *st);
void setInsertMode(state_t *st);
void setNavigateMode(state_t *st);
void forwardPage(state_t *st);
void backwardPage(state_t *st);

// void appendLine(state_t *st);
// void prependLine(state_t *st);
// void setInsertModeAndInsertChar(state_t *st, uchar c);
void insertChar(state_t *st, uchar c);
void delete(state_t *st);
// void backspace(state_t *st);
#endif /* Keysym_h */
