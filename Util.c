//
//  Util.c
//  editordebugger
//
//  Created by Brett Letner on 4/23/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include "Util.h"

char *builtinBufferTitle[NUM_BUILTIN_BUFFERS] = { "*help", "*messages", "*buffers", "*macros", "*copy buffer", "*searches", "*config" };

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

void setBlendMode(SDL_Renderer *renderer, SDL_BlendMode m)
{
  if (SDL_SetRenderDrawBlendMode(renderer, m) != 0) die(SDL_GetError());
}

void setDrawColor(SDL_Renderer *renderer, color_t c)
{
  color_t alpha = c & 0xff;
  setBlendMode(renderer, alpha == 0xff ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
  if (SDL_SetRenderDrawColor(renderer, c >> 24, (c >> 16) & 0xff, (c >> 8) & 0xff, alpha) != 0) die(SDL_GetError());
}

void clearFrame(SDL_Renderer *renderer)
{
  if (SDL_RenderClear(renderer) != 0) die(SDL_GetError());
}

void setViewport(SDL_Renderer *renderer, SDL_Rect *r)
{
  if (SDL_RenderSetViewport(renderer, r) != 0) die(SDL_GetError());
}

void fillRect(SDL_Renderer *renderer, const SDL_Rect *rect)
{
  if (SDL_RenderFillRect(renderer, rect) != 0) die(SDL_GetError());
}

// BAL: these don't belong here
int frameWidth(frame_t *frame)
{
  return frame->rect.w;
}

int frameColumns(frame_t *frame, state_t *st)
{
  return frameWidth(frame) / st->font.charSkip;
}

