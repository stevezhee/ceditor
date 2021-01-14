//
//  Syntax.c
//  ceditor
//
//  Created by Brett Letner on 4/23/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#include "DynamicArray.h"
#include "Widget.h"
#include "Syntax.h"

bool isLidentChar(char c) { return (c >= 'a' && c <= 'z') || c == '_'; }
bool isUidentChar(char c) { return c >= 'A' && c <= 'Z'; }
bool isNumChar(char c) { return (c >= '0' && c <= '9') || c == '.'; }
bool isIdentChar(char c) {
  return isLidentChar(c) || isUidentChar(c) || isNumChar(c);
}
bool isSpecialChar(char c) {
  return (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' ||
          c == ']' || c == ';' || c == ':' || c == ',');
}
char peekChar(char *p, char *q) {
  if (p < q)
    return *p;
  return '\0';
}
// BAL: handle reserved words (requires lookahead)
// BAL: enable other font characteristics bold, italic, underline
color_t getCharColor(char c, tokSt_t *accp, char *p, char *q) {
  tokSt_t acc = *accp;

  switch (acc) {
  case IN_STRING:
    if (c == '"')
      *accp = TOKBEGIN;
    else if (c == '\\')
      *accp = IN_STRING_ESC;
    return literalColor;
  case IN_STRING_ESC:
    *accp = IN_STRING;
    return literalColor;
  case IN_CHAR:
    if (c == '\'')
      *accp = TOKBEGIN;
    else if (c == '\\')
      *accp = IN_CHAR_ESC;
    return literalColor;
  case IN_CHAR_ESC:
    *accp = IN_CHAR;
    return literalColor;
  case IN_SLCOMMENT:
    if (c == '\n')
      *accp = TOKBEGIN;
    return commentColor;
  case IN_MLCOMMENT:
    if (c == '*')
      *accp = IN_MLCOMMENT_END;
    return commentColor;
  case IN_MLCOMMENT_END:
    if (c == '/')
      *accp = TOKBEGIN;
    else
      *accp = IN_MLCOMMENT;
    return commentColor;
  case IN_LIDENT:
    if (isIdentChar(c))
      return lidentColor;
    goto tok_begin;
  case IN_UIDENT:
    if (isIdentChar(c))
      return uidentColor;
    goto tok_begin;
  case IN_PREPROC:
    if (isIdentChar(c))
      return preprocColor;
    goto tok_begin;
  case IN_NUMBER:
    if (isIdentChar(c))
      return literalColor;
    goto tok_begin;
  default:
    goto tok_begin;
  }

tok_begin:
  switch (c) {
  case '\'':
    *accp = IN_CHAR;
    return literalColor;
  case '"':
    *accp = IN_STRING;
    return literalColor;
  case '-': {
    char c1 = peekChar(p, q);
    if (isNumChar(c1)) {
      *accp = IN_NUMBER;
      return literalColor;
    }
    *accp = TOKBEGIN;
    return symbolColor;
  }
  case '/': {
    char c1 = peekChar(p, q);
    switch (c1) {
    case '/':
      *accp = IN_SLCOMMENT;
      return commentColor;
    case '*':
      *accp = IN_MLCOMMENT;
      return commentColor;
    default:
      *accp = TOKBEGIN;
      return symbolColor;
    }
  }
  case '#':
    *accp = IN_PREPROC;
    return preprocColor;
  default:
    if (isLidentChar(c)) {
      *accp = IN_LIDENT;
      return lidentColor;
    }
    if (isUidentChar(c)) {
      *accp = IN_UIDENT;
      return uidentColor;
    }
    if (isNumChar(c)) {
      *accp = IN_NUMBER;
      return literalColor;
    }
    if (isSpecialChar(c)) {
      *accp = TOKBEGIN;
      return specialColor;
    }
    if (c >= ' ' && c <= '~') {
      *accp = TOKBEGIN;
      return symbolColor;
    }
  }
  assert(c < ' ' || c > '~');
  *accp = TOKBEGIN;
  return unknownColor;
}

void drawCString(char *s, int n)
{
  assert(s);
  assert(n >= 0);

  uchar c;
  SDL_Texture *txtr;
  SDL_Rect rect;
  rect.x = 0; // context.dx;
  rect.y = context.dy;
  rect.w = context.font->charSkip;
  rect.h = context.font->lineSkip;

  char *p = s;
  char *q = s + n;
  tokSt_t acc = TOKBEGIN;

  while (p < q) {
    c = *p;
    p++;
    color_t color = getCharColor(c, &acc, p, q);
    switch (c) {
    case '\n':
      rect.x = 0; // context.dx;
      rect.y += rect.h;
      break;
    case ' ':
      rect.x += rect.w;
      break;
    default:
      txtr = context.font->charTexture[c];

      // BAL: could do setTextureColorMod only when the color changes if we used
      // a texture atlas
      setTextureColorMod(txtr, color);

      SDL_RenderCopy(renderer, txtr, NULL, &rect);
      rect.x += rect.w;
    }
  }

}
void drawString(string_t *s) {
  if (!s)
    return;
  assert(s);
  if (!s->start)
    return;
  drawCString(s->start, s->numElems);
}

