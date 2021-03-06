//
//  Keysym.h
//  ceditor
//
//  Created by Brett Letner on 5/17/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#ifndef Keysym_h
#define Keysym_h

#include "Util.h"

typedef void (*keyHandler_t)(uchar c);
extern keyHandler_t keyHandler[NUM_MODES][NUM_KEYS];
extern char *keyHandlerHelp[NUM_MODES][NUM_KEYS];
#define NUM_BUILTIN_MACROS 18
#define MAX_BUILTIN_MACRO_LEN 8
char builtinMacros[NUM_BUILTIN_MACROS][MAX_BUILTIN_MACRO_LEN];

extern char *builtinMacrosHelp[NUM_BUILTIN_MACROS];

void macrosInit(void);

void keysymInit(void);
uchar getKeyChar(SDL_Keycode c);

char *keysymName(uchar key);
void doNothing(uchar c);
void doKeyPressInNavigateMode(uchar c);
void doEscapeInsert();
void doEscapeNavigate();
void forwardEOF();
void backwardSOF();
void newSearch();
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
void enter();
void replace();
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
void undo();
void redo();
void insertChar(uchar c);
void cut();
void insertOpenCloseChars(uchar c);
void saveAll();
void increaseFont();
void decreaseFont();
void insertNewline();

#endif /* Keysym_h */
