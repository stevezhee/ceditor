//
//  Util.c
//  editordebugger
//
//  Created by Brett Letner on 4/23/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include "Util.h"

SDL_Renderer *renderer;

char *builtinBufferTitle[NUM_BUILTIN_BUFFERS] = { "*help", "*messages", "*buffers", "*macros", "*copy buffer", "*searches", "*config" };
char *editorModeDescr[NUM_MODES] = { "NAV", "INS", "FND" };

color_t viewColors[] = { VIEW_COLOR, FOCUS_VIEW_COLOR };

void die(const char *msg)
{
  fprintf(stdout, "error: %s\n", msg);
  exit(1);
}

void *dieIfNull(void *p)
{
  if (!p) die("unexpected null pointer");
  return p;
}

void setBlendMode(SDL_BlendMode m)
{
  if (SDL_SetRenderDrawBlendMode(renderer, m) != 0) die(SDL_GetError());
}

void setDrawColor(color_t c)
{
  color_t alpha = c & 0xff;
  setBlendMode(alpha == 0xff ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
  int r = c >> 24;
  int g = (c >> 16) & 0xff;
  int b = (c >> 8) & 0xff;
  if (SDL_SetRenderDrawColor(renderer, c >> 24, (c >> 16) & 0xff, (c >> 8) & 0xff, alpha) != 0) die(SDL_GetError());
}

void setTextureColorMod(SDL_Texture *t, color_t c)
{
  assert(t);

  int r = c >> 24;
  int g = (c >> 16) & 0xff;
  int b = (c >> 8) & 0xff;
  if (SDL_SetTextureColorMod(t, r, g, b) != 0) die(SDL_GetError());
}

void rendererClear(void)
{
  if (SDL_RenderClear(renderer) != 0) die(SDL_GetError());
}

void rendererInit(SDL_Window *win)
{
  // For some reason these cause memory corruption(?)...
  // renderer = dieIfNull(SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE));
  renderer = dieIfNull(SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED));
  // renderer = dieIfNull(SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC));
  // but this works(?)
  // SDL_Surface *srfc = dieIfNull(SDL_GetWindowSurface(win));
  // renderer = dieIfNull(SDL_CreateSoftwareRenderer(srfc));
}

void rendererPresent(void)
{
   SDL_RenderPresent(renderer);
}

void setViewport(SDL_Rect *r)
{
  if (SDL_RenderSetViewport(renderer, r) != 0) die(SDL_GetError());
}

void fillRectAt(int x, int y, int w, int h)
{
  SDL_Rect r;
  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;
  if (SDL_RenderFillRect(renderer, &r) != 0) die(SDL_GetError());
}
void fillRect(int w, int h)
{
  fillRectAt(0, 0, w, h);
}

char *getClipboardText(void)
{
  if (!SDL_HasClipboardText()) return NULL;
  return dieIfNull(SDL_GetClipboardText());
}

void setClipboardText(const char *text)
{
  if (SDL_SetClipboardText(text) != 0) die(SDL_GetError());
}
/* // BAL: these don't belong here */
/* int frameWidth(frame_t *frame) */
/* { */
/*   return frame->rect.w; */
/* } */

/* int frameColumns(frame_t *frame, state_t *st) */
/* { */
/*   return frameWidth(frame) / st->font.charSkip; */
/* } */

