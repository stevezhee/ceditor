//
//  Keysym.c
//  editordebugger
//
//  Created by Brett Letner on 5/17/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include <assert.h>
#include "Keysym.h"
#include "Util.h"

const char shiftChars[] = "\"....<_>?)!@#$%^&*(.:.+.............................{|}..~ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// BAL: really need this for shift characters?  Nothing in SDL lib?

keyHandler_t keyHandler[NUM_MODES][NUM_KEYS];

char *macro[255];

#define NUM_BUILTIN_MACROS 16

char builtinMacros[NUM_BUILTIN_MACROS][8] = {
    { KEY_BACKSPACE, KEY_LEFT, KEY_DELETE, '\0' },
    { 'p', KEY_RIGHT, 'P', '\0' },
    { 'c', 'x', 'P', '\0' },
    { 'a', '0', '\0' },
    { 'e', '$', '\0' },
    { 'I', '0', 'i', '\0'},
    { ' ', 'i', ' ', '\0' },
    { '\n', 'i', '\n', '\0' },
    { 'A', '$', 'i', '\0' },
    { 'o', '$', '\n', '\0' },
    { 'O', '0', '\n', KEY_LEFT, '\0' },
    { 'J', '$', KEY_DELETE, '\0' },
    { KEY_INSERT, 'i', '\0' },
    { '\t', '0', 'i', ' ', ' ', KEY_LEFT, KEY_LEFT, '\0' },
    { KEY_SHIFT_TAB, '0', 'x', 'x', '\0' },
    { ';', '0', 'i', '/', '/', KEY_LEFT, KEY_LEFT, '\0' },
};

void initMacros(void)
{
    memset(macro, 0, sizeof(macro));
    for(int i = 0; i < NUM_BUILTIN_MACROS; ++i)
    {
        macro[builtinMacros[i][0]] = &builtinMacros[i][1];
    }
}

void keysymInit(void)
{
    for(int i = 0; i < NUM_KEYS; ++i)
    {
        keyHandler[NAVIGATE_MODE][i] = doNothing;
        keyHandler[INSERT_MODE][i] = setNavigateModeAndDoKeyPress;
    }

    keyHandler[NAVIGATE_MODE][KEY_DELETE] = (keyHandler_t)delete;

    keyHandler[NAVIGATE_MODE]['x'] = (keyHandler_t)delete;

    //    keyHandler[NAVIGATE_MODE][KEY_SHIFT_RETURN] = (keyHandler_t)prependLine;

    keyHandler[NAVIGATE_MODE]['P'] = (keyHandler_t)pasteBefore;

    keyHandler[NAVIGATE_MODE]['v'] = (keyHandler_t)selectChars;
    keyHandler[NAVIGATE_MODE]['V'] = (keyHandler_t)selectLines;

    keyHandler[NAVIGATE_MODE][KEY_LEFT] = (keyHandler_t)backwardChar;
    keyHandler[NAVIGATE_MODE][KEY_RIGHT] = (keyHandler_t)forwardChar;
    keyHandler[NAVIGATE_MODE][KEY_UP] = (keyHandler_t)backwardLine;
    keyHandler[NAVIGATE_MODE][KEY_DOWN] = (keyHandler_t)forwardLine;
    keyHandler[NAVIGATE_MODE]['n'] = (keyHandler_t)forwardSearch;
    keyHandler[NAVIGATE_MODE]['N'] = (keyHandler_t)backwardSearch;
    keyHandler[NAVIGATE_MODE][KEY_PAGEUP] = (keyHandler_t)backwardPage;
    keyHandler[NAVIGATE_MODE][KEY_PAGEDOWN] = (keyHandler_t)forwardPage;
    keyHandler[NAVIGATE_MODE][KEY_HOME] = (keyHandler_t)backwardSOF;
    keyHandler[NAVIGATE_MODE][KEY_END] = (keyHandler_t)forwardEOF;
    
    // keyHandler[NAVIGATE_MODE]['h'] = (keyHandler_t)backwardChar;
    // BAL: 'h' or '?' goto help buffer?
    // keyHandler[NAVIGATE_MODE]['l'] = (keyHandler_t)forwardChar;
    // keyHandler[NAVIGATE_MODE]['k'] = (keyHandler_t)forwardLine;
    // keyHandler[NAVIGATE_MODE]['j'] = (keyHandler_t)backwardLine;
//    keyHandler[NAVIGATE_MODE]['w'] = forwardWord;
//    keyHandler[NAVIGATE_MODE][KEY_SHIFT_RIGHT] = forwardWord;
//    keyHandler[NAVIGATE_MODE]['b'] = backwardWord;
//    keyHandler[NAVIGATE_MODE][KEY_SHIFT_LEFT] = backwardWord;
    keyHandler[NAVIGATE_MODE]['0'] = (keyHandler_t)backwardSOL;
    keyHandler[NAVIGATE_MODE]['$'] = (keyHandler_t)forwardEOL;
    keyHandler[NAVIGATE_MODE]['g'] = (keyHandler_t)gotoView;
    //    keyHandler[NAVIGATE_MODE][KEY_SHIFT_DOWN] = forwardParagraph;
//    keyHandler[NAVIGATE_MODE][KEY_SHIFT_UP] = backwardParagraph;
//    
    keyHandler[NAVIGATE_MODE]['i'] = (keyHandler_t)setInsertMode;


//    keyHandler[NAVIGATE_MODE]['-'] = decreaseFont;
//    keyHandler[NAVIGATE_MODE]['+'] = increaseFont;
//
//    keyHandler[NAVIGATE_MODE]['u'] = undo;
//    keyHandler[NAVIGATE_MODE]['U'] = redo;
    //    keyHandler[NAVIGATE_MODE]['r'] = redo;
    keyHandler[NAVIGATE_MODE]['/'] = (keyHandler_t)setSearchMode;
    keyHandler[NAVIGATE_MODE][','] = (keyHandler_t)playMacro;
    keyHandler[NAVIGATE_MODE]['m'] = (keyHandler_t)startStopRecording;
    
    for(char c = '!'; c <= '~'; ++c)
    {
        keyHandler[INSERT_MODE][c] = insertChar;
    }
    
    keyHandler[INSERT_MODE]['\n'] = insertChar;
    keyHandler[INSERT_MODE][' '] = insertChar;
}

uchar getKeyChar(SDL_Keycode c)
{
    int isShift = SDL_GetModState() & KMOD_SHIFT;
    
    if (c >= '!' && c <= '~')
    {
        if (isShift) { c = shiftChars[c - '\'']; }
        return c;
    }
    
    if (c >= SDLK_F1 && c <= SDLK_UP)
    {
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
        
    switch (c)
    {
        case SDLK_INSERT:
        case 167:
            if (isShift) return KEY_SHIFT_INSERT;
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
            if (isShift) return KEY_SHIFT_BACKSPACE;
            return KEY_BACKSPACE;
        case SDLK_TAB:
            assert(c == 9);
            if (isShift) return KEY_SHIFT_TAB;
            return '\t';
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            assert('\n' == 10);
            if (isShift) return KEY_SHIFT_RETURN;
            return '\n';
        case SDLK_ESCAPE:
            assert(c == KEY_ESCAPE);
            if (isShift) return KEY_SHIFT_ESCAPE;
            return KEY_ESCAPE;
        case SDLK_SPACE:
            assert(c == 32);
            if (isShift) return KEY_SHIFT_SPACE;
            return c;
        case SDLK_DELETE:
            assert(c == KEY_DELETE);
            if (isShift) return KEY_SHIFT_DELETE;
            return KEY_DELETE;
        default:
            return KEY_UNKNOWN;
    }
}

//void increaseFont()
//{
//    if (font->size >= 140) return;
//    font->size++;
//    reinitFont(font);
//}
//
//void decreaseFont()
//{
//    if (font->size <= 4) return;
//    font->size--;
//    reinitFont(font);
//}
//
