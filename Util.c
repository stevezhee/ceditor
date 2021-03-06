//
//  Util.c
//  ceditor
//
//  Created by Brett Letner on 4/23/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#include "Util.h"

SDL_Renderer *renderer;

char *builtinBufferTitle[NUM_BUILTIN_BUFFERS] = {
    "*help",   "*messages", "*buffers", "*macros",
    "*copies", "*config", "*directory", "*search"};
bool builtinBufferReadOnly[NUM_BUILTIN_BUFFERS] = {true, true, true, true,
                                                   true, false, true, false};

char *editorModeDescr[NUM_MODES] = {"NAV", "INS"};

void myMemcpy(void *dst, const void *src, size_t n)
{
  // BAL: inline
  // printf("memcpy\n");
  memcpy(dst, src, n); // BAL: check return value
}
void myMemset(void *b, int c, size_t len)
{
  // BAL: inline
  // printf("memset\n");
  memset(b, c, len); // BAL: check return value
}
void die(const char *msg) {
  fprintf(stdout, "error: %s\n", msg);
  exit(1);
}

void *dieIfNull(void *p) {
  if (!p)
    die("unexpected null pointer");
  return p;
}

void setBlendMode(SDL_BlendMode m) {
  if (SDL_SetRenderDrawBlendMode(renderer, m) != 0)
    die(SDL_GetError());
}

void setDrawColor(color_t c) {
  color_t alpha = c & 0xff;
  setBlendMode(alpha == 0xff ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
  int r = c >> 24;
  int g = (c >> 16) & 0xff;
  int b = (c >> 8) & 0xff;
  if (SDL_SetRenderDrawColor(renderer, c >> 24, (c >> 16) & 0xff,
                             (c >> 8) & 0xff, alpha) != 0)
    die(SDL_GetError());
}

void setTextureColorMod(SDL_Texture *t, color_t c) {
  assert(t);

  int r = c >> 24;
  int g = (c >> 16) & 0xff;
  int b = (c >> 8) & 0xff;
  if (SDL_SetTextureColorMod(t, r, g, b) != 0)
    die(SDL_GetError());
}

void rendererClear(void) {
  setDrawColor(FRAME_COLOR);
  if (SDL_RenderClear(renderer) != 0)
    die(SDL_GetError());
}

void rendererInit(SDL_Window *win) {
  renderer = dieIfNull(SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE));
}

void rendererPresent(void) { SDL_RenderPresent(renderer); }

void setViewport(SDL_Rect *r) {
  if (SDL_RenderSetViewport(renderer, r) != 0)
    die(SDL_GetError());
}

void fillRectAt(int x, int y, int w, int h) {
  SDL_Rect r;
  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;
  if (SDL_RenderFillRect(renderer, &r) != 0)
    die(SDL_GetError());
}

void fillRect(int w, int h) { fillRectAt(0, 0, w, h); }

int numLinesString(char *s, int len) {
  int n = 0;
  char *end = s + len;

  while (s < end) {
    if (*s == '\n')
      n++;
    s++;
  }
  return n;
}

char *getClipboardText(void) {
  if (!SDL_HasClipboardText())
    return NULL;
  return SDL_GetClipboardText();
}

void setClipboardText(const char *text) {
  if (SDL_SetClipboardText(text) != 0)
    die(SDL_GetError());
}
