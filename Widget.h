//
//  Widget.h
//  ceditor
//
//  Created by Brett Letner on 5/27/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#ifndef Widget_h
#define Widget_h

#include "Util.h"

typedef enum {
  HSPC,
  VSPC,
  DRAW,
  COLOR,
  FONT,
  SCROLL_Y,
  WID,
  OVER,
  HCAT,
  VCAT,
  HCATR,
  VCATR,
  NUM_WIDGET_TAGS
} widgetTag_t;

struct widget_s;

typedef struct widget_s widget_t;

struct widget_s {
  widgetTag_t tag;
  union {
    int wid;
    int *length;
    int *color;
    font_t **font;
    widget_t *child;
    void (*drawFun)(int);
    int (*scrollYFun)(int);
    void *data;
  } a;
  union {
    widget_t *child;
    void *data;
    int ref;
  } b;
  union {
    void *data;
    int ref;
  } c;
};

typedef struct {
  int x;
  int y;
  int w;
  int h;
  int color;
  font_t *font;
  int dy;
  int wid;
} context_t;

extern context_t context;

void contextReinit(font_t *font, int w, int h);
widget_t *wid(int i, widget_t *a);
widget_t *node(widgetTag_t tag, void *a, void *b);
widget_t *singleton(widgetTag_t tag, void *a, widget_t *b);
  widget_t *singleton2(widgetTag_t tag, void *a, widget_t *b, void *c);
void widgetAt(widget_t *widget, int x, int y);
void widgetDraw(widget_t *widget);
widget_t *leaf(widgetTag_t tag, void *a);
void drawBox(void *_unused);

#define over(a, b) node(OVER, a, b)
#define hcat(a, b) node(HCAT, a, b)
#define hcatr(a, b) node(HCATR, a, b)
#define vcat(a, b) node(VCAT, a, b)
#define vcatr(a, b) node(VCATR, a, b)
#define draw(a, b) node(DRAW, a, (void *)(uintptr_t)(b))
#define box() node(DRAW, drawBox, NULL)
#define color(a, b) singleton(COLOR, a, b)
#define font(a, b) singleton(FONT, a, b)
#define scrollY(a, c, b) singleton2(SCROLL_Y, a, b, (void *)(uintptr_t)(c))
#define hspc(len) leaf(HSPC, len)
#define vspc(len) leaf(VSPC, len)

#endif /* Widget_h */
