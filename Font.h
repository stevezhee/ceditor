//
//  Font.h
//  editordebugger
//
//  Created by Brett Letner on 4/23/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#ifndef Font_h
#define Font_h

#include "Util.h"

void initFont(font_t *font, SDL_Renderer *renderer, const char* file, unsigned int size);
void reinitFont(font_t *font);
void resetCharRect(font_t *font, int scrollX, int scrollY);
void renderEOF(font_t *font);
void renderBox(font_t *font, char *p, unsigned int len);
void renderAndAdvChar(font_t *font, char c);

#endif /* Font_h */
