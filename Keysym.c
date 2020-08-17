//
//  Keysym.c
//  editordebugger
//
//  Created by Brett Letner on 5/17/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include "Keysym.h"

const char shiftChars[] = "\"....<_>?)!@#$%^&*(.:.+............................"
                          ".{|}..~ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// BAL: really need this for shift characters?  Nothing in SDL lib?

keyHandler_t keyHandler[NUM_MODES][NUM_KEYS];
char *keyHandlerHelp[NUM_MODES][NUM_KEYS];

char *macro[256];
char *macroHelp[256];

char builtinMacros[NUM_BUILTIN_MACROS][MAX_BUILTIN_MACRO_LEN] = {
    {KEY_BACKSPACE, KEY_LEFT, KEY_DELETE, '\0'},
    {'p', KEY_RIGHT, 'P', '\0'},
    {'c', 'x', 'P', '\0'},
    {'a', KEY_RIGHT, 'i', '\0'},
    {'e', '$', '\0'},
    {'I', '0', 'i', '\0'},
    {' ', 'i', ' ', '\0'},
    {'\n', 'i', '\n', '\0'},
    {'A', '$', 'i', '\0'},
    {'o', '$', '\n', '\0'},
    {'O', '0', '\n', KEY_LEFT, '\0'},
    {'J', '$', KEY_DELETE, '\0'},
    {KEY_INSERT, 'i', '\0'},
    {';', '0', 'i', '/', '/', KEY_LEFT, KEY_LEFT, '\0'},
    {'x', KEY_DELETE, '\0'},
    {'w', KEY_SHIFT_RIGHT, '\0'},
    {'b', KEY_SHIFT_LEFT, '\0'},
};

char *builtinMacrosHelp[NUM_BUILTIN_MACROS] = {
    "backspace",
    "paste to the right",
    "copy",
    "append",
    "move to end of line",
    "insert at start of line",
    "insert space",
    "insert newline",
    "append at end of line",
    "insert new line after current line",
    "insert new line before current line",
    "join lines",
    "insert",
    "comment region",
    "cut/delete",
    "forward word",
    "backward word",
};

void macrosInit(void) {
  memset(macro, 0, sizeof(macro));
  for (int i = 0; i < NUM_BUILTIN_MACROS; ++i) {
    macro[builtinMacros[i][0]] = &builtinMacros[i][1];
  }
}

extern char *keyName[256];
char *keyName[256];

char keyNameBuf[2];

char *keysymName(uchar key) {
  char *s = keyName[key];
  if (s)
    return s;
  keyNameBuf[0] = key;
  return keyNameBuf;
}

void keyNameInit(void) {
  memset(keyName, 0, sizeof(keyName));
  keyName['\n'] = "\\n";
  keyName['\t'] = "\\t";
  keyName['\''] = "\\'";
  keyName[KEY_UNKNOWN] = "UNKNOWN";
  keyName[KEY_SHIFT_RETURN] = "SHIFT_RETURN";
  keyName[KEY_SHIFT_SPACE] = "SHIFT_SPACE";
  keyName[KEY_SHIFT_BACKSPACE] = "SHIFT_BACKSPACE";
  keyName[KEY_SHIFT_DELETE] = "SHIFT_DELETE";
  keyName[KEY_BACKSPACE] = "BACKSPACE";
  keyName[KEY_CTRL] = "CTRL";
  keyName[KEY_ALT] = "ALT";
  keyName[KEY_SHIFT_TAB] = "SHIFT_TAB";
  keyName[KEY_SHIFT_ESCAPE] = "SHIFT_ESCAPE";
  keyName[KEY_ESCAPE] = "ESCAPE";
  keyName[KEY_DELETE] = "DELETE";
  keyName[KEY_F1] = "F1";
  keyName[KEY_F2] = "F2";
  keyName[KEY_F3] = "F3";
  keyName[KEY_F4] = "F4";
  keyName[KEY_F5] = "F5";
  keyName[KEY_F6] = "F6";
  keyName[KEY_F7] = "F7";
  keyName[KEY_F8] = "F8";
  keyName[KEY_F9] = "F9";
  keyName[KEY_F10] = "F10";
  keyName[KEY_F11] = "F11";
  keyName[KEY_F12] = "F12";
  keyName[KEY_PRINTSCREEN] = "PRINTSCREEN";
  keyName[KEY_SCROLLLOCK] = "SCROLLLOCK";
  keyName[KEY_PAUSE] = "PAUSE";
  keyName[KEY_INSERT] = "INSERT";
  keyName[KEY_HOME] = "HOME";
  keyName[KEY_PAGEUP] = "PAGEUP";
  keyName[KEY_UNUSED_DELETE] = "UNUSED_DELETE";
  keyName[KEY_END] = "END";
  keyName[KEY_PAGEDOWN] = "PAGEDOWN";
  keyName[KEY_RIGHT] = "RIGHT";
  keyName[KEY_LEFT] = "LEFT";
  keyName[KEY_DOWN] = "DOWN";
  keyName[KEY_UP] = "UP";
  keyName[KEY_SHIFT_F1] = "SHIFT_F1";
  keyName[KEY_SHIFT_F2] = "SHIFT_F2";
  keyName[KEY_SHIFT_F3] = "SHIFT_F3";
  keyName[KEY_SHIFT_F4] = "SHIFT_F4";
  keyName[KEY_SHIFT_F5] = "SHIFT_F5";
  keyName[KEY_SHIFT_F6] = "SHIFT_F6";
  keyName[KEY_SHIFT_F7] = "SHIFT_F7";
  keyName[KEY_SHIFT_F8] = "SHIFT_F8";
  keyName[KEY_SHIFT_F9] = "SHIFT_F9";
  keyName[KEY_SHIFT_F10] = "SHIFT_F10";
  keyName[KEY_SHIFT_F11] = "SHIFT_F11";
  keyName[KEY_SHIFT_F12] = "SHIFT_F12";
  keyName[KEY_SHIFT_PRINTSCREEN] = "SHIFT_PRINTSCREEN";
  keyName[KEY_SHIFT_SCROLLLOCK] = "SHIFT_SCROLLLOCK";
  keyName[KEY_SHIFT_PAUSE] = "SHIFT_PAUSE";
  keyName[KEY_SHIFT_INSERT] = "SHIFT_INSERT";
  keyName[KEY_SHIFT_HOME] = "SHIFT_HOME";
  keyName[KEY_SHIFT_PAGEUP] = "SHIFT_PAGEUP";
  keyName[KEY_SHIFT_UNUSED_DELETE] = "SHIFT_UNUSED_DELETE";
  keyName[KEY_SHIFT_END] = "SHIFT_END";
  keyName[KEY_SHIFT_PAGEDOWN] = "SHIFT_PAGEDOWN";
  keyName[KEY_SHIFT_RIGHT] = "SHIFT_RIGHT";
  keyName[KEY_SHIFT_LEFT] = "SHIFT_LEFT";
  keyName[KEY_SHIFT_DOWN] = "SHIFT_DOWN";
  keyName[KEY_SHIFT_UP] = "SHIFT_UP";
}

void keysymInit(void) {
  memset(keyHandlerHelp, 0, sizeof(keyHandlerHelp));
  keyNameInit();

  for (int i = 0; i < NUM_KEYS; ++i) {
    keyHandler[NAVIGATE_MODE][i] = doNothing;
    keyHandler[INSERT_MODE][i] = doKeyPressInNavigateMode;
  }

  keyHandler[INSERT_MODE][KEY_ESCAPE] = (keyHandler_t)setNavigateMode;
  keyHandlerHelp[INSERT_MODE][KEY_ESCAPE] = "set navigate mode";

  keyHandler[NAVIGATE_MODE]['\t'] = (keyHandler_t)indent;
  keyHandlerHelp[NAVIGATE_MODE]['\t'] = "indent region";
  keyHandler[NAVIGATE_MODE][KEY_SHIFT_TAB] = (keyHandler_t)outdent;
  keyHandlerHelp[NAVIGATE_MODE][KEY_SHIFT_TAB] = "outdent region";

  keyHandler[NAVIGATE_MODE][KEY_DELETE] = (keyHandler_t)cut;
  keyHandlerHelp[NAVIGATE_MODE][KEY_DELETE] = "cut/delete";

  //    keyHandler[NAVIGATE_MODE][KEY_SHIFT_RETURN] = (keyHandler_t)prependLine;

  keyHandler[NAVIGATE_MODE]['P'] = (keyHandler_t)pasteBefore;
  keyHandlerHelp[NAVIGATE_MODE]['P'] = "paste before";

  keyHandler[NAVIGATE_MODE]['v'] = (keyHandler_t)selectChars;
  keyHandlerHelp[NAVIGATE_MODE]['v'] = "select characters";
  keyHandler[NAVIGATE_MODE]['V'] = (keyHandler_t)selectLines;
  keyHandlerHelp[NAVIGATE_MODE]['V'] = "select lines";

  keyHandler[NAVIGATE_MODE][KEY_LEFT] = (keyHandler_t)backwardChar;
  keyHandlerHelp[NAVIGATE_MODE][KEY_LEFT] = "backward char";
  keyHandler[NAVIGATE_MODE][KEY_RIGHT] = (keyHandler_t)forwardChar;
  keyHandlerHelp[NAVIGATE_MODE][KEY_RIGHT] = "forward char";
  keyHandler[NAVIGATE_MODE][KEY_UP] = (keyHandler_t)backwardLine;
  keyHandlerHelp[NAVIGATE_MODE][KEY_UP] = "backward line";
  keyHandler[NAVIGATE_MODE][KEY_DOWN] = (keyHandler_t)forwardLine;
  keyHandlerHelp[NAVIGATE_MODE][KEY_DOWN] = "forward line";
  keyHandler[NAVIGATE_MODE][KEY_PAGEUP] = (keyHandler_t)backwardPage;
  keyHandlerHelp[NAVIGATE_MODE][KEY_PAGEUP] = "backward page";
  keyHandler[NAVIGATE_MODE][KEY_PAGEDOWN] = (keyHandler_t)forwardPage;
  keyHandlerHelp[NAVIGATE_MODE][KEY_PAGEDOWN] = "forward page";
  keyHandler[NAVIGATE_MODE][KEY_HOME] = (keyHandler_t)backwardSOF;
  keyHandlerHelp[NAVIGATE_MODE][KEY_HOME] = "backward to start of file";
  keyHandler[NAVIGATE_MODE][KEY_END] = (keyHandler_t)forwardEOF;
  keyHandlerHelp[NAVIGATE_MODE][KEY_END] = "forward to end of file";
  keyHandler[NAVIGATE_MODE]['h'] = (keyHandler_t)backwardFrame;
  keyHandlerHelp[NAVIGATE_MODE]['h'] = "backward frame";
  keyHandler[NAVIGATE_MODE]['g'] = (keyHandler_t)forwardFrame;
  keyHandlerHelp[NAVIGATE_MODE]['g'] = "forward frame";
  keyHandler[NAVIGATE_MODE]['j'] = (keyHandler_t)backwardView;
  keyHandlerHelp[NAVIGATE_MODE]['j'] = "backward view";
  keyHandler[NAVIGATE_MODE]['f'] = (keyHandler_t)forwardView;
  keyHandlerHelp[NAVIGATE_MODE]['f'] = "forward view";
  keyHandler[NAVIGATE_MODE][KEY_SHIFT_RIGHT] = (keyHandler_t)forwardSpace;
  keyHandlerHelp[NAVIGATE_MODE][KEY_SHIFT_RIGHT] = "forward to space";
  keyHandler[NAVIGATE_MODE][KEY_SHIFT_LEFT] = (keyHandler_t)backwardSpace;
  keyHandlerHelp[NAVIGATE_MODE][KEY_SHIFT_LEFT] = "backward to space";
  keyHandler[NAVIGATE_MODE][KEY_SHIFT_DOWN] = (keyHandler_t)forwardBlankLine;
  keyHandlerHelp[NAVIGATE_MODE][KEY_SHIFT_DOWN] = "forward to blank line";
  keyHandler[NAVIGATE_MODE][KEY_SHIFT_UP] = (keyHandler_t)backwardBlankLine;
  keyHandlerHelp[NAVIGATE_MODE][KEY_SHIFT_UP] = "backward to blank line";
  keyHandler[NAVIGATE_MODE]['0'] = (keyHandler_t)backwardSOL;
  keyHandlerHelp[NAVIGATE_MODE]['0'] = "backward to start of line";
  keyHandler[NAVIGATE_MODE]['$'] = (keyHandler_t)forwardEOL;
  keyHandlerHelp[NAVIGATE_MODE]['$'] = "forward to end of line";
  keyHandler[NAVIGATE_MODE]['i'] = (keyHandler_t)setInsertMode;
  keyHandlerHelp[NAVIGATE_MODE]['i'] = "insert mode";
  keyHandler[NAVIGATE_MODE]['u'] = (keyHandler_t)undo;
  keyHandlerHelp[NAVIGATE_MODE]['u'] = "undo";
  keyHandler[NAVIGATE_MODE]['r'] = (keyHandler_t)redo;
  keyHandlerHelp[NAVIGATE_MODE]['r'] = "redo";
  keyHandler[NAVIGATE_MODE]['/'] = (keyHandler_t)newSearch;
  keyHandlerHelp[NAVIGATE_MODE]['/'] = "new search";
  keyHandler[NAVIGATE_MODE]['n'] = (keyHandler_t)forwardSearch;
  keyHandlerHelp[NAVIGATE_MODE]['n'] = "search forward";
  keyHandler[NAVIGATE_MODE]['N'] = (keyHandler_t)backwardSearch;
  keyHandlerHelp[NAVIGATE_MODE]['N'] = "search backward";
  keyHandler[NAVIGATE_MODE][','] = (keyHandler_t)stopRecordingOrPlayMacro;
  keyHandlerHelp[NAVIGATE_MODE][','] = "play/stop recording macro";
  keyHandler[NAVIGATE_MODE]['m'] = (keyHandler_t)startOrStopRecording;
  keyHandlerHelp[NAVIGATE_MODE]['m'] = "start/stop recording macro";
  keyHandler[NAVIGATE_MODE]['{'] = insertOpenCloseChars;
  keyHandlerHelp[NAVIGATE_MODE]['{'] = "insert open/close braces";
  keyHandler[NAVIGATE_MODE]['('] = insertOpenCloseChars;
  keyHandlerHelp[NAVIGATE_MODE]['('] = "insert open/close parens";
  keyHandler[NAVIGATE_MODE]['['] = insertOpenCloseChars;
  keyHandlerHelp[NAVIGATE_MODE]['['] = "insert open/close brackets";
  keyHandler[NAVIGATE_MODE]['\''] = insertOpenCloseChars;
  keyHandlerHelp[NAVIGATE_MODE]['\''] = "insert open/close single quotes";
  keyHandler[NAVIGATE_MODE]['"'] = insertOpenCloseChars;
  keyHandlerHelp[NAVIGATE_MODE]['"'] = "insert open/close double quotes";
  keyHandler[NAVIGATE_MODE]['S'] = (keyHandler_t)saveAll;
  keyHandlerHelp[NAVIGATE_MODE]['S'] = "save all and run make";

  // BAL: 'h' or '?' goto help buffer?
  //    keyHandler[NAVIGATE_MODE]['-'] = decreaseFont;
  //    keyHandler[NAVIGATE_MODE]['+'] = increaseFont;
  //
  for (char c = '!'; c <= '~'; ++c) {
    keyHandler[INSERT_MODE][c] = insertChar;
  }

  keyHandler[INSERT_MODE]['\n'] = insertChar;
  keyHandler[INSERT_MODE][' '] = insertChar;
}

uchar getKeyChar(SDL_Keycode c) {
  int isShift = SDL_GetModState() & KMOD_SHIFT;

  if (c >= '!' && c <= '~') {
    if (isShift) {
      c = shiftChars[c - '\''];
    }
    return c;
  }

  if (c >= SDLK_F1 && c <= SDLK_UP) {
    c = 128 + (c - SDLK_F1);
    assert(c >= KEY_F1);
    assert(c <= KEY_UP);
    if (isShift) {
      c += 1 + KEY_UP - KEY_F1;
      assert(c >= KEY_SHIFT_F1);
      assert(c <= KEY_SHIFT_UP);
    }
    return c;
  }

  switch (c) {
  case SDLK_INSERT:
  case 167:
    if (isShift)
      return KEY_SHIFT_INSERT;
    return KEY_INSERT;
  case SDLK_CAPSLOCK:
  case SDLK_LCTRL:
  case SDLK_RCTRL:
    return KEY_CTRL;
  case SDLK_LALT:
  case SDLK_RALT:
  case SDLK_LGUI:
  case SDLK_RGUI:
    return KEY_ALT;
  case SDLK_BACKSPACE:
    if (isShift)
      return KEY_SHIFT_BACKSPACE;
    return KEY_BACKSPACE;
  case SDLK_TAB:
    assert(c == 9);
    if (isShift)
      return KEY_SHIFT_TAB;
    return '\t';
  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    assert('\n' == 10);
    if (isShift)
      return KEY_SHIFT_RETURN;
    return '\n';
  case SDLK_ESCAPE:
    assert(c == KEY_ESCAPE);
    if (isShift)
      return KEY_SHIFT_ESCAPE;
    return KEY_ESCAPE;
  case SDLK_SPACE:
    assert(c == 32);
    if (isShift)
      return KEY_SHIFT_SPACE;
    return c;
  case SDLK_DELETE:
    assert(c == KEY_DELETE);
    if (isShift)
      return KEY_SHIFT_DELETE;
    return KEY_DELETE;
  default:
    return KEY_UNKNOWN;
  }
}

// void increaseFont()
//{
//    if (font->size >= 140) return;
//    font->size++;
//    reinitFont(font);
//}
//
// void decreaseFont()
//{
//    if (font->size <= 4) return;
//    font->size--;
//    reinitFont(font);
//}
//
