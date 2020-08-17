//
//  Util.h
//  editordebugger
//
//  Created by Brett Letner on 4/23/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#ifndef Util_h
#define Util_h

#include <SDL.h>
#include <SDL_ttf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdbool.h>

#define min(x, y) ((x) <= (y) ? (x) : (y))
#define max(x, y) ((x) >= (y) ? (x) : (y))
#define swap(ty, x, y) { ty tmp = x; x = y; y = tmp; }
#define clamp(a,b,c) min(max((a), (b)), (c))

struct state_s;
typedef struct state_s state_t;
typedef int color_t;

void die(const char *msg);
void *dieIfNull(void *p);
void fillRectAt(int x, int y, int w, int h);
void fillRect(int width, int height);
void setBlendMode(SDL_BlendMode m);
void setDrawColor(color_t c);
void rendererClear(void);
void setViewport(SDL_Rect *r);
void rendererInit(SDL_Window *win);
void rendererPresent(void);
char *getClipboardText(void);
void setClipboardText(const char *text);
int numLinesString(char *s, int len);

typedef enum { NAVIGATE_MODE, INSERT_MODE, SEARCH_MODE, NUM_MODES } editorMode_t; // BAL: remove search_mode?
extern char *editorModeDescr[NUM_MODES];

typedef unsigned char uchar;
typedef enum { NOT_DIRTY = 0b0, DOC_DIRTY = 0b1, FOCUS_DIRTY = 0b10, WINDOW_DIRTY = 0b100 } windowDirty_t;
extern char *macro[256];
void setTextureColorMod(SDL_Texture *t, color_t c);

// "solarized" colors
#define BRBLACK 0x002b36ff
#define BLACK 0x073642ff
#define BRGREEN 0x586e75ff
#define BRYELLOW 0x657b83ff
#define BRBLUE 0x839496ff
#define BRCYAN 0x93a1a1ff
#define WHITE 0xeee8d5ff
#define BRWHITE 0xfdf6e3ff
#define YELLOW 0xb58900ff
#define BRRED 0xcb4b16ff
#define RED 0xdc322fff
#define MAGENTA 0xd33682ff
#define BRMAGENTA 0x6c71c4ff
#define BLUE 0x268bd2ff
#define CYAN 0x2aa198ff
#define GREEN 0x859900ff

#define FOCUS_FRAME_COLOR BRBLACK
#define FRAME_COLOR BLACK
#define CURSOR_COLOR 0xffff00ff
#define CURSOR_BACKGROUND_COLOR (CURSOR_COLOR & 0xffffff30)
#define SELECTION_COLOR CURSOR_BACKGROUND_COLOR
#define SEARCH_COLOR (BRRED & 0xffffff60)
#define INIT_WINDOW_WIDTH 2488
#define INIT_WINDOW_HEIGHT 1300
#define INIT_FONT_SIZE 20
#define INIT_FONT_FILE "./assets/SourceCodePro-Semibold.ttf"
#define SELECTION_RECT_GAP 8
#define AUTO_SCROLL_HEIGHT 4

#define CURSOR_WIDTH 3
#define BORDER_WIDTH 4
#define DISPLAY_NEWLINES false
#define DISPLAY_EOF false
#define DEMO_MODE true
#define NO_GIT false
#define NO_COMPILE false

enum { HELP_BUF, MESSAGE_BUF, BUFFERS_BUF, MACRO_BUF, COPY_BUF, SEARCH_BUF, CONFIG_BUF, NUM_BUILTIN_BUFFERS };  // BAL: DIRECTORY_BUF for loading files?  or just put in config?

extern char *builtinBufferTitle[NUM_BUILTIN_BUFFERS];
extern bool builtinBufferReadOnly[NUM_BUILTIN_BUFFERS];

enum { SECONDARY_FRAME, MAIN_FRAME, BUILTINS_FRAME, NUM_FRAMES };

extern uint16_t unicode[256];

extern SDL_Renderer *renderer;

struct font_s {
  int lineSkip;
  int charSkip;
  SDL_Texture *charTexture[256]; // BAL: ['~' + 1];
  SDL_Rect charRect; // BAL: remove
  SDL_Rect cursorRect; // BAL: remove
  const char *filepath;
  unsigned int size;
};

typedef struct font_s font_t;

struct dynamicArray_s {
  void *start;
  int numElems;
  int maxElems;
  int elemSize;
  int offset;
};

typedef struct dynamicArray_s dynamicArray_t;

typedef dynamicArray_t string_t; // contains characters

typedef enum { DELETE, INSERT } commandTag_t;

struct command_s {
  commandTag_t tag;
  int offset;
  string_t string;
};

typedef struct command_s command_t;
typedef dynamicArray_t undoStack_t; // contains commands
typedef dynamicArray_t searchBuffer_t; // contains result offsets

struct doc_s {
  char *filepath;
  bool isUserDoc;
  bool isReadOnly;
  string_t contents;
  undoStack_t undoStack;
  int numLines;
  searchBuffer_t searchResults;
};

typedef struct doc_s doc_t;

struct cursor_s {
  int offset;
  int row;
  int column;
  int preferredColumn;
};

typedef struct cursor_s cursor_t;

typedef enum { NO_SELECT, CHAR_SELECT, LINE_SELECT, NUM_SELECT_MODES } selectMode_t;

struct view_s // BAL: this needs to be renamed
{
  editorMode_t mode;
  int refDoc;
  int scrollY;
  cursor_t cursor;
  cursor_t selection;
  selectMode_t selectMode;
};

typedef struct view_s view_t;

typedef dynamicArray_t docsBuffer_t;
typedef dynamicArray_t viewsBuffer_t;

struct frame_s
{
  viewsBuffer_t views;
  color_t color;
  int height;
  int width;
  string_t status;
};

typedef struct frame_s frame_t;

struct window_s
{
  SDL_Window *window;
  int width;
  int height;
};

typedef struct window_s window_t;

typedef dynamicArray_t frameBuffer_t;

struct state_s
{
  docsBuffer_t docs;
  frameBuffer_t frames;
  window_t window;
  font_t font;
  SDL_Event event;
  bool isRecording;
  int searchLen;
  bool isReplace;
  string_t replace;
  int downCxtX;
  int downCxtY;
  bool mouseSelectionInProgress;
};

typedef struct state_s state_t;

#define KEY_UNKNOWN 0
#define KEY_SHIFT_RETURN 1
#define KEY_SHIFT_SPACE 2
#define KEY_SHIFT_BACKSPACE 3
#define KEY_SHIFT_DELETE 4
#define KEY_BACKSPACE 5
#define KEY_CTRL 6
#define KEY_ALT 7
#define KEY_SHIFT_TAB 11
#define KEY_SHIFT_ESCAPE 12
#define KEY_ESCAPE 27
#define KEY_DELETE 127

#define KEY_F1 128
#define KEY_F2 129
#define KEY_F3 130
#define KEY_F4 131
#define KEY_F5 132
#define KEY_F6 133
#define KEY_F7 134
#define KEY_F8 135
#define KEY_F9 136
#define KEY_F10 137
#define KEY_F11 138
#define KEY_F12 139
#define KEY_PRINTSCREEN 140
#define KEY_SCROLLLOCK 141
#define KEY_PAUSE 142
#define KEY_INSERT 143
#define KEY_HOME 144
#define KEY_PAGEUP 145
#define KEY_UNUSED_DELETE 146
#define KEY_END 147
#define KEY_PAGEDOWN 148
#define KEY_RIGHT 149
#define KEY_LEFT 150
#define KEY_DOWN 151
#define KEY_UP 152

#define KEY_SHIFT_F1 153
#define KEY_SHIFT_F2 154
#define KEY_SHIFT_F3 155
#define KEY_SHIFT_F4 156
#define KEY_SHIFT_F5 157
#define KEY_SHIFT_F6 158
#define KEY_SHIFT_F7 159
#define KEY_SHIFT_F8 160
#define KEY_SHIFT_F9 161
#define KEY_SHIFT_F10 162
#define KEY_SHIFT_F11 163
#define KEY_SHIFT_F12 164
#define KEY_SHIFT_PRINTSCREEN 165
#define KEY_SHIFT_SCROLLLOCK 166
#define KEY_SHIFT_PAUSE 167
#define KEY_SHIFT_INSERT 168
#define KEY_SHIFT_HOME 169
#define KEY_SHIFT_PAGEUP 170
#define KEY_SHIFT_UNUSED_DELETE 171
#define KEY_SHIFT_END 172
#define KEY_SHIFT_PAGEDOWN 173
#define KEY_SHIFT_RIGHT 174
#define KEY_SHIFT_LEFT 175
#define KEY_SHIFT_DOWN 176
#define KEY_SHIFT_UP 177
#define NUM_KEYS 178

#endif /* Util_h */
