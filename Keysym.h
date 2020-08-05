//
//  Keysym.h
//  editordebugger
//
//  Created by Brett Letner on 5/17/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#ifndef Keysym_h
#define Keysym_h

#include "Util.h"

typedef void (*keyHandler_t)(uchar c);
extern keyHandler_t keyHandler[NUM_MODES][NUM_KEYS];
void initMacros(void);

void keysymInit(void);
uchar getKeyChar(SDL_Keycode c);

void doNothing(uchar c);
void setNavigateModeAndDoKeyPress(uchar c);
void forwardEOF();
void backwardSOF();
void setSearchMode();
void gotoView();
void backwardChar();
void forwardChar();
void backwardLine();
void forwardLine();
void forwardEOL();
void backwardSOL();
void startOrStopRecording();
void stopRecordingOrPlayMacro();
void pasteBefore();
void forwardSearch();
void backwardSearch();
void setInsertMode();
void setNavigateMode();
void forwardPage();
void backwardPage();
void selectChars();
void selectLines();
void forwardFrame();
void backwardFrame();
void forwardView();
void backwardView();
void forwardSpace();
void backwardSpace();
void forwardBlankLine();
void backwardBlankLine();
void indent();
void outdent();
// void appendLine();
// void prependLine();
// void setInsertModeAndInsertChar(uchar c);
void insertChar(uchar c);
void cut();
// void backspace();
#endif /* Keysym_h */
