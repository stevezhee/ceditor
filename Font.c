//
//  Font.c
//  ceditor
//
//  Created by Brett Letner on 4/23/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#include "Font.h"

SDL_Color white = {255, 255, 255};
uint16_t unicode[256];

static inline void initCharTexture(font_t *font, TTF_Font *ttfFont, uchar c) {
  uint16_t s[2];

  s[0] = unicode[c];
  s[1] = '\0';

  SDL_Surface *srfc = TTF_RenderUNICODE_Blended(ttfFont, s, white);
  if (srfc == NULL)
    die(TTF_GetError());
  font->charTexture[c] = SDL_CreateTextureFromSurface(renderer, srfc);
  if (font->charTexture[c] == NULL)
    die(TTF_GetError());

  SDL_FreeSurface(srfc);
}

static inline void initFontData(font_t *font) {
  TTF_Font *ttfFont = TTF_OpenFont(font->filepath, font->size);

  if (ttfFont == NULL)
    die(TTF_GetError());

  if (TTF_FontFaceIsFixedWidth(ttfFont) == 0)
    die("Requested font face is not fixed width\n");

  for (int c = 0; c < 256; ++c) {
    initCharTexture(font, ttfFont, c);
  }

  font->lineSkip = TTF_FontLineSkip(ttfFont);
  TTF_CloseFont(ttfFont);

  SDL_QueryTexture(font->charTexture['!'], NULL, NULL, &font->charSkip,
                   &font->charRect.h);
  font->charRect.x = 0;
  font->charRect.y = 0;
  font->charRect.w = font->charSkip;

  font->cursorRect.x = 0;
  font->cursorRect.y = 0;
  font->cursorRect.w = 3;
  font->cursorRect.h = font->lineSkip;
}

void reinitFont(font_t *font) {
  for (uint16_t c = 0; c <= 255; c++) {
    SDL_DestroyTexture(font->charTexture[c]);
  }

  initFontData(font);
}

void unicodeInit(void);

void initFont(font_t *font, const char *file, unsigned int size) {
  if (TTF_Init() != 0)
    die(TTF_GetError());

  font->filepath = file;
  font->size = size;

  unicodeInit();
  initFontData(font);
}

void resetCharRect(font_t *font, int scrollX, int scrollY) {
  font->charRect.x = font->cursorRect.w + scrollX;
  font->charRect.y = font->cursorRect.w + scrollY;
}
static void inline renderCh(font_t *font, int c) {
  SDL_RenderCopy(renderer, font->charTexture[c], NULL, &font->charRect);
}

void renderEOF(font_t *font) {
  if (DISPLAY_EOF)
    renderCh(font, '\0');
}

void renderEOL(font_t *font) {
  if (DISPLAY_NEWLINES)
    renderCh(font, '\n');
}

void advanceChar(font_t *font, SDL_Rect *r, char c) {
  switch (c) {
  case '\n':
    r->x = font->cursorRect.w;
    r->y += font->lineSkip;
    return;
  default:
    r->x += font->charSkip;
    return;
  }
}

void renderChar(font_t *font, uchar c) {
  switch (c) {
  case ' ':
    return;
  case '\n':
    renderEOL(font);
    return;
  default:
    renderCh(font, c);
    return;
  }
}

void renderAndAdvChar(font_t *font, char c) {
  renderChar(font, c);
  advanceChar(font, &font->charRect, c);
}

void unicodeInit(void) {
  memset(unicode, UINT16_MAX, sizeof(unicode));
  for (int i = '!'; i <= '~'; ++i) {
    unicode[i] = i;
  }
  unicode['\0'] = 0xa7;
  unicode[KEY_BACKSPACE] = 0x21d0;
  unicode[KEY_DELETE] = 0x3c7;
  unicode[KEY_CTRL] = 0x21d1;
  unicode[KEY_ALT] = 0x21d3;
  unicode[KEY_ESCAPE] = 0x3b5;
  unicode[KEY_PRINTSCREEN] = 0x3c0;
  unicode[KEY_SCROLLLOCK] = 0x3c3;
  unicode[KEY_PAUSE] = 0x3c6;
  unicode[KEY_INSERT] = 0x3b9;
  unicode[KEY_HOME] = 0x2302;
  unicode[KEY_PAGEUP] = 0x25b3;
  unicode[KEY_END] = 0x221e;
  unicode[KEY_PAGEDOWN] = 0x25bd;
  unicode[KEY_RIGHT] = 0x2192;
  unicode[KEY_LEFT] = 0x2190;
  unicode[KEY_DOWN] = 0x2193;
  unicode[KEY_UP] = 0x2191;
  unicode['\t'] = 0x21d2;
  unicode['\n'] = 0xb6;
  unicode[KEY_SHIFT_TAB] = 0x21d0;
}
