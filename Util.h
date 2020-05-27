//
//  Util.h
//  editordebugger
//
//  Created by Brett Letner on 4/23/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#ifndef Util_h
#define Util_h

#define min(x, y) ((x) <= (y) ? (x) : (y))
#define max(x, y) ((x) >= (y) ? (x) : (y))
#define swap(ty, x, y) { ty tmp = x; x = y; y = tmp; }
#define clamp(a,b,c) min(max((a), (b)), (c))

struct state_s;
typedef struct state_s state_t;

void die(const char *msg);

typedef enum { NAVIGATE_MODE, INSERT_MODE, SEARCH_MODE, NUM_MODES } editorMode_t;
typedef unsigned char uchar;
typedef enum { NOT_DIRTY = 0b0, DOC_DIRTY = 0b1, FOCUS_DIRTY = 0b10, WINDOW_DIRTY = 0b100 } windowDirty_t;
typedef int color_t;
extern color_t viewColors[];
extern char *macro[255];

#define FOCUS_VIEW_COLOR 0x000000ff
#define VIEW_COLOR 0x303030ff
#define CURSOR_COLOR 0xffff00ff
#define CURSOR_BACKGROUND_COLOR 0xffff40a0
#define SELECTION_COLOR CURSOR_BACKGROUND_COLOR
#define SEARCH_COLOR 0xffff4080
#define INIT_WINDOW_WIDTH 1300
#define INIT_WINDOW_HEIGHT 800
#define INIT_FONT_SIZE 24
#define INIT_FONT_FILE "./assets/SourceCodePro-Semibold.ttf"

#define CURSOR_WIDTH 3
#define BORDER_WIDTH 4
#define BACKGROUND_COLOR 0xa0a0a0ff
#define DISPLAY_NEWLINES false
#define DISPLAY_EOF false
#define DEMO_MODE false

enum { HELP_BUF, MESSAGE_BUF, BUFFERS_BUF, MACRO_BUF, COPY_BUF, SEARCH_BUF, CONFIG_BUF, NUM_BUILTIN_BUFFERS };  // BAL: DIRECTORY_BUF for loading files?  or just put in config?

extern char *builtinBufferTitle[NUM_BUILTIN_BUFFERS];

enum { SECONDARY_VIEWPORT, MAIN_VIEWPORT, BUILTINS_VIEWPORT, NUM_VIEWPORTS };

extern uint16_t unicode[256];

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
