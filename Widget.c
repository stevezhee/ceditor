//
//  Widget.c
//  ceditor
//
//  Created by Brett Letner on 4/23/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#include "Util.h"
#include "Widget.h"

context_t context;

widget_t *newWidget() { return dieIfNull(malloc(sizeof(widget_t))); }

widget_t *leaf(widgetTag_t tag, void *a) {
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.data = a;
  return p;
}

widget_t *singleton(widgetTag_t tag, void *a, widget_t *b) {
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.data = a;
  p->b.child = b;
  return p;
}

widget_t *singleton2(widgetTag_t tag, void *a, widget_t *b, void *c) {
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.data = a;
  p->b.child = b;
  p->c.data = c;
  return p;
}

widget_t *wid(int i, widget_t *a) {
  widget_t *p = newWidget();
  p->tag = WID;
  p->a.wid = i;
  p->b.child = a;
  return p;
}

widget_t *node(widgetTag_t tag, void *a, void *b) {
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.data = a;
  p->b.data = b;
  return p;
}

void drawBox(void *_unused) { fillRect(context.w, context.h); }

int widgetWidth(widget_t *widget) {
  int w = 0;

  switch (widget->tag) {
  case HCATR: // fall through
  case HCAT:
    w = widgetWidth(widget->a.child) + widgetWidth(widget->b.child);
    break;
  case OVER:  // fall through
  case VCATR: // fall through
  case VCAT:
    w = max(widgetWidth(widget->a.child), widgetWidth(widget->b.child));
    break;
  case SCROLL_Y: // fall through
  case WID:      // fall through
  case COLOR:    // fall through
  case FONT:
    w = widgetWidth(widget->b.child);
    break;
  case HSPC:
    w = *widget->a.length;
    break;
  default:
    assert(widget->tag == VSPC || widget->tag == DRAW);
    break;
  }

  return min(w, context.w);
}

int widgetHeight(widget_t *widget) {
  int h = 0;
  switch (widget->tag) {
  case OVER:  // fall through
  case HCATR: // fall through
  case HCAT:
    h = max(widgetHeight(widget->a.child), widgetHeight(widget->b.child));
    break;
  case VCATR: // fall through
  case VCAT:
    h = widgetHeight(widget->a.child) + widgetHeight(widget->b.child);
    break;
  case SCROLL_Y: // fall through
  case COLOR:    // fall through
  case WID:      // fall through
  case FONT:
    h = widgetHeight(widget->b.child);
    break;
  case VSPC:
    h = *widget->a.length;
    break;
  default:
    assert(widget->tag == HSPC || widget->tag == DRAW);
    break;
  }
  return min(h, context.h);
}

void contextSetViewport() {
  SDL_Rect rect;
  rect.x = context.x;
  rect.y = context.y;
  rect.w = context.w;
  rect.h = context.h;
  setViewport(&rect);
}

void widgetAt(widget_t *widget, int x, int y) {
  assert(context.w >= 0);
  assert(context.h >= 0);

  if (x >= context.w || y >= context.h) {
    context.wid = -1;
    return;
  }

  switch (widget->tag) {
  case HCAT: {
    int w = widgetWidth(widget->a.child);
    if (x < w) {
      context.w = w;
      widgetAt(widget->a.child, x, y);
      return;
    }
    context.w -= w;
    context.x += w;
    widgetAt(widget->b.child, x - w, y);
    return;
  }
  case HCATR: {
    int w = context.w - widgetWidth(widget->b.child);
    if (context.x < w) {
      context.w = w;
      widgetAt(widget->a.child, x, y);
      return;
    }
    context.w -= w;
    context.x += w;
    widgetAt(widget->b.child, x - w, y);
    return;
  }
  case VCAT: {
    int h = widgetHeight(widget->a.child);
    if (context.y < h) {
      context.h = h;
      widgetAt(widget->a.child, x, y);
      return;
    }
    context.h -= h;
    context.y += h;
    widgetAt(widget->b.child, x, y - h);
    return;
  }
  case VCATR: {
    int h = context.h - widgetHeight(widget->b.child);
    if (context.y < h) {
      context.h = h;
      widgetAt(widget->a.child, x, y);
      return;
    }
    context.h -= h;
    context.y += h;
    widgetAt(widget->b.child, x, y - h);
    return;
  }
  case OVER:
    widgetAt(widget->a.child, x, y);
    return;
  case WID:
    context.wid = widget->a.wid;
    widgetAt(widget->b.child, x, y);
    return;
  case COLOR: // fall through
  case FONT:  // fall through
    widgetAt(widget->b.child, x, y);
    return;
  case SCROLL_Y: {
    int dy = widget->a.scrollYFun(widget->c.ref);
    context.y += dy;
    widgetAt(widget->b.child, x, y + dy);
    return;
  }
  default:
    assert(widget->tag == VSPC || widget->tag == HSPC || widget->tag == DRAW);
    return;
  }
}

void widgetDraw(widget_t *widget) {
  switch (widget->tag) {
  case HCAT: {
    int x = context.x;
    int w = context.w;
    int wa = widgetWidth(widget->a.child);

    context.x += wa;
    context.w -= wa;
    contextSetViewport();
    widgetDraw(widget->b.child);

    context.x = x;
    context.w = wa;
    contextSetViewport();
    widgetDraw(widget->a.child);

    context.w = w;
    contextSetViewport();
    return;
  }
  case HCATR: {
    int x = context.x;
    int w = context.w;

    context.w -= widgetWidth(widget->b.child);
    contextSetViewport();
    widgetDraw(widget->a.child);

    context.x += context.w;
    context.w = w - context.w;
    contextSetViewport();
    widgetDraw(widget->b.child);

    context.x = x;
    context.w = w;
    contextSetViewport();
    return;
  }
  case VCAT: {
    int y = context.y;
    int h = context.h;

    context.h = widgetHeight(widget->a.child);
    contextSetViewport();
    widgetDraw(widget->a.child);

    context.y += context.h;
    context.h = h - context.h;
    contextSetViewport();
    widgetDraw(widget->b.child);

    context.h = h;
    context.y = y;
    contextSetViewport();
    return;
  }
  case VCATR: {
    int y = context.y;
    int h = context.h;

    context.h -= widgetHeight(widget->b.child);
    contextSetViewport();
    widgetDraw(widget->a.child);

    context.y += context.h;
    context.h = h - context.h;
    contextSetViewport();
    widgetDraw(widget->b.child);

    context.h = h;
    context.y = y;
    contextSetViewport();
    return;
  }
  case OVER: {
    widgetDraw(widget->b.child);
    widgetDraw(widget->a.child);
    return;
  }
  case SCROLL_Y: {
    int dy = context.dy;
    context.dy += widget->a.scrollYFun(widget->c.ref);
    widgetDraw(widget->b.child);
    context.dy = dy;
    return;
  }
  case FONT: {
    font_t *font = context.font;
    context.font = *widget->a.font;
    widgetDraw(widget->b.child);
    context.font = font;
    return;
  }
  case WID:
    widgetDraw(widget->b.child);
    return;
  case COLOR: {
    int color = context.color;
    context.color = *widget->a.color;
    setDrawColor(context.color);
    widgetDraw(widget->b.child);
    context.color = color;
    setDrawColor(context.color);
    return;
  }
  case DRAW:
    widget->a.drawFun(widget->b.ref);
    return;
  default:
    assert(widget->tag == VSPC || widget->tag == HSPC);
    return;
  }
}

void contextReinit(font_t *font, int w, int h) {
  context.x = 0;
  context.y = 0;
  context.color = BRWHITE;
  context.font = font;
  context.w = w;
  context.h = h;
  context.dy = 0;
  context.wid = -1;
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = context.w;
  rect.h = context.h;
  setViewport(&rect);
  setDrawColor(context.color);
}
