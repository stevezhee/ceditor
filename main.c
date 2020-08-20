#include "Cursor.h"
#include "Doc.h"
#include "DynamicArray.h"
#include "Font.h"
#include "Keysym.h"
#include "Search.h"
#include "Util.h"

void insertCString(char *s);
void insertString(char *s, uint len);
void builtinInsertCString(char *s);
void builtinInsertString(char *s, uint len);
void builtinInsertChar(uchar c);

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
    void (*drawFun)(void *);
    int (*scrollYFun)(void *);
    void *data;
  } a;
  union {
    widget_t *child;
    void *data;
  } b;
  union {
    void *data;
  } c;
};

state_t st;
widget_t *gui;
int xToColumn(int x) { return x / st.font.charSkip; }
int yToRow(int x) { return x / st.font.lineSkip; }

int numFrames() { return st.frames.numElems; }

int numDocs() { return st.docs.numElems; }

uint frameRows(frame_t *frame) { return frame->height / st.font.lineSkip; }

int focusFrameRef() { return st.frames.offset; }

frame_t *focusFrame() { return arrayFocus(&st.frames); }

frame_t *frameOf(int i) { return arrayElemAt(&st.frames, i); }

int focusViewRef() { return focusFrame()->views.offset; }

view_t *viewOf(frame_t *frame) { return arrayFocus(&frame->views); }

view_t *focusView() { return viewOf(focusFrame()); }

int numViews() { return focusFrame()->views.numElems; }

doc_t *docOf(view_t *view) { return arrayElemAt(&st.docs, view->refDoc); }

doc_t *focusDoc() { return docOf(focusView()); }

cursor_t *focusCursor() { return &focusView()->cursor; }

cursor_t *focusSelection() { return &focusView()->selection; }

bool selectionActive(view_t *view) { return (view->selectMode != NO_SELECT); }

void frameResize(int frameRef) {
  frame_t *frame = frameOf(frameRef);
  int focusRef = focusFrameRef();
  int w = st.window.width / 3;

  if (frameRef == focusRef && frameRef != BUILTINS_FRAME)
    w += w / 2;
  else if (frameRef != focusRef && frameRef == BUILTINS_FRAME)
    w /= 2;

  frame->width = w;
  frame->height = st.window.height - st.font.lineSkip;
}

void setFocusFrame(int i) {
  assert(i >= 0);
  assert(i < numFrames());
  int j = focusFrameRef();
  if (i == j)
    return;

  frame_t *frame = frameOf(j);
  frame->color = FRAME_COLOR;
  arraySetFocus(&st.frames, i);
  frame = frameOf(i);
  frame->color = FOCUS_FRAME_COLOR;

  for (int i = 0; i < numFrames(); ++i) {
    frameResize(i);
  }
}

void frameUpdate(frame_t *frame) {
  view_t *view = viewOf(frame);
  doc_t *doc = docOf(view);
  // BAL: make sure we don't overflow the buffer
  arrayReinit(&frame->status);
  sprintf(frame->status.start, "<%s> %3d:%2d %s", editorModeDescr[view->mode],
          view->cursor.row + 1, view->cursor.column, doc->filepath);
  frame->status.numElems = strlen(frame->status.start);
}

void setFrameView(int refFrame, int refView) {
  frame_t *frame = frameOf(refFrame);
  assert(refView < frame->views.numElems);
  arraySetFocus(&frame->views, refView);

  frameUpdate(frame);
}

void setFocusView(int refView) { setFrameView(focusFrameRef(), refView); }

void setFocusBuiltinsView(int ref) {
  setFocusFrame(BUILTINS_FRAME);
  setFocusView(ref);
}

void frameInit(frame_t *frame) {
  memset(frame, 0, sizeof(frame_t));
  frame->color = FRAME_COLOR;

  arrayInit(&frame->views, sizeof(view_t));
  arrayInit(&frame->status, sizeof(char));
  arrayGrow(&frame->status, 1024);
}

void viewInit(view_t *view, uint i) {
  memset(view, 0, sizeof(view_t));
  view->refDoc = i;
}

/* bool noActiveSearch(frame_t *frame) */
/* { */
/*     return */
/*       frame->refView != st.searchRefView || // BAL: there are now multiple
 * views per frame */
/*       st.searchLen == 0 || */
/*       st.searchResults.numElems == 0; */
/* } */

widget_t *newWidget() { return dieIfNull(malloc(sizeof(widget_t))); }

widget_t *leaf(widgetTag_t tag, void *a) {
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.data = a;
  return p;
}

#define hspc(len) leaf(HSPC, len)
#define vspc(len) leaf(VSPC, len)

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

#define color(a, b) singleton(COLOR, a, b)
#define font(a, b) singleton(FONT, a, b)
#define scrollY(a, c, b) singleton2(SCROLL_Y, a, b, c)

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

context_t context;

void drawBox(void *_unused) { fillRect(context.w, context.h); }

#define over(a, b) node(OVER, a, b)
#define hcat(a, b) node(HCAT, a, b)
#define hcatr(a, b) node(HCATR, a, b)
#define vcat(a, b) node(VCAT, a, b)
#define vcatr(a, b) node(VCATR, a, b)
#define draw(a, b) node(DRAW, a, b)
#define box() node(DRAW, drawBox, NULL)

int maxLineLength(char *s) {
  assert(s);
  char c;
  int rmax = 0;
  int r = 0;
  while ((c = *s) != '\0') {
    switch (c) {
    case '\n':
      rmax = max(rmax, r + 1);
      r = 0;
      break;
    default:
      r++;
    }
    s++;
  }
  return max(rmax, r);
}

int numLinesCString(char *s) {
  int n = 0;
  char c;
  while ((c = *s) != '\0') {
    if (c == '\n')
      n++;
    s++;
  }
  return n;
}

void drawStringSelection(int x, int y, char *s, int len) {
  // BAL: would it look good to bold the characters in addition/instead?
  assert(s);
  int w = context.font->charSkip;
  int h = context.font->lineSkip;
  char *p = s + len;
  while (s < p) {
    fillRectAt(x, y, w, h);
    switch (*s) {
    case '\n':
      x = 0;
      y += h;
      break;
    default:
      x += w;
    }
    s++;
  }
}

#define preprocColor BRMAGENTA
#define literalColor BRRED
#define commentColor BRGREEN
#define unknownColor MAGENTA
#define uidentColor CYAN
#define lidentColor BRCYAN
#define symbolColor YELLOW
#define specialColor BLUE

typedef enum {
  TOKBEGIN,
  IN_LIDENT,
  IN_UIDENT,
  IN_NUMBER,
  IN_PREPROC,
  IN_SLCOMMENT,
  IN_MLCOMMENT,
  IN_MLCOMMENT_END,
  IN_STRING,
  IN_STRING_ESC,
  IN_CHAR,
  IN_CHAR_ESC
} tokSt_t;

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

void drawString(string_t *s) {
  if (!s)
    return;
  assert(s);
  if (!s->start)
    return;
  uchar c;
  SDL_Texture *txtr;
  SDL_Rect rect;
  rect.x = 0; // context.dx;
  rect.y = context.dy;
  rect.w = context.font->charSkip;
  rect.h = context.font->lineSkip;

  char *p = s->start;
  char *q = arrayTop(s);
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

int columnToX(int column) { return column * st.font.charSkip; }

int rowToY(int row) { return row * st.font.lineSkip + context.dy; }

void drawCursor(view_t *view) {
  int x = columnToX(view->cursor.column);
  int y = rowToY(view->cursor.row);

  if (view->mode == NAVIGATE_MODE) {
    setDrawColor(CURSOR_BACKGROUND_COLOR);
    fillRectAt(x, y, st.font.charSkip, st.font.lineSkip);
  }

  setDrawColor(CURSOR_COLOR);
  fillRectAt(x, y, CURSOR_WIDTH, st.font.lineSkip);

  setDrawColor(context.color);
}

int distanceToEOL(char *s) {
  char *p = s;
  char c = *p;

  while (c != '\n' && c != '\0') {
    p++;
    c = *p;
  }
  return p - s;
}

int distanceToNextSpace(char *s0, char *t) {
  char *s = s0;
  char c = *s;

  while (s < t) {
    s++;
    c = *s;
    if (c == ' ' || c == '\n') {
      while (s < t) {
        s++;
        c = *s;
        if (c != ' ' && c != '\n')
          return s - s0;
      }
      return s - s0;
    }
  }

  return s - s0;
}
int distanceToIndent(char *p0, char *q) {
  char *p = p0;
  while (p < q && *p == ' ') {
    p++;
  }
  return p - p0;
}
int distanceToPrevSpace(char *t, char *s0) {
  char *s = s0;
  char c = *s;

  while (s > t) {
    s--;
    c = *s;
    if (c == ' ' || c == '\n') {
      while (s > t) {
        s--;
        c = *s;
        if (c != ' ' && c != '\n')
          return s - s0;
      }
      return s - s0;
    }
  }

  return s - s0;
}

int distanceToStartOfElem(char *s, int offset) {
  char *p0 = s + offset;
  char *p = p0;
  if (p > s && *p == '\n')
    p--;
  if (p > s && *p == '\0')
    p--;
  while (p > s) {
    if (*p == '\0') {
      return p - p0 + 2;
    }
    p--;
  }
  return p - p0;
}

void getSelectionCoords(view_t *view, int *column, int *row, int *offset,
                        int *len) {
  if (!selectionActive(view)) {
    *column = view->cursor.column;
    *row = view->cursor.row;
    *offset = view->cursor.offset;
    *len = 1;
    return;
  }

  cursor_t *a = &view->cursor;
  cursor_t *b = &view->selection;

  if (a->offset > b->offset) {
    swap(cursor_t *, a, b);
  }

  *offset = a->offset;
  *len = b->offset - *offset;
  *row = a->row;
  *column = a->column;

  char *s = docOf(view)->contents.start;

  if (view->selectMode == LINE_SELECT) {
    *len += distanceToEOL(s + *offset + *len);
    *offset -= *column;
    *len += *column;
    *column = 0;
  }
  *len += 1;
}

void drawSearch(view_t *view) {
  doc_t *doc = docOf(view);
  char *s = doc->contents.start;

  setDrawColor(SEARCH_COLOR);

  searchBuffer_t *results = &doc->searchResults;

  for (int i = 0; i < results->numElems; ++i) {
    cursor_t c;
    int *offset = arrayElemAt(results, i);
    cursorSetOffset(
        &c, *offset,
        doc); // BAL: this is inefficient (it starts over every time)
    drawStringSelection(columnToX(c.column), rowToY(c.row), s + *offset,
                        st.searchLen);
  }

  setDrawColor(context.color);
}

void drawSelection(view_t *view) {

  int offset;
  int len;
  int column;
  int row;
  char *s = docOf(view)->contents.start;

  setDrawColor(SELECTION_COLOR);

  getSelectionCoords(view, &column, &row, &offset, &len);

  drawStringSelection(columnToX(column), rowToY(row), s + offset, len);

  setDrawColor(context.color);
}

bool cursorEq(cursor_t *a, cursor_t *b) {
  return (a->row == b->row && a->column == b->column);
}

bool searchActive(frame_t *frame) {
  // if the doc in the builtins frame is searche
  // and this isn't the searches view
  return frameOf(BUILTINS_FRAME)->views.offset == SEARCH_BUF &&
         frame->views.offset != SEARCH_BUF;
}

void drawCursorOrSelection(frame_t *frame) {
  view_t *view = viewOf(frame);
  if (searchActive(frame)) {
    drawSearch(view);
  }
  if (selectionActive(view)) {
    drawSelection(view);
    return;
  }
  drawCursor(view);
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
    context.w = context.w - w;
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
    context.w = context.w - w;
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
    context.h = context.h - h;
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
    context.h = context.h - h;
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
    int dy = widget->a.scrollYFun(widget->c.data);
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
    context.w = context.w - wa;
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

    context.w = context.w - widgetWidth(widget->b.child);
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

    context.h = context.h - widgetHeight(widget->b.child);
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
    context.dy += widget->a.scrollYFun(widget->c.data);
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
    widget->a.drawFun(widget->b.data);
    return;
  default:
    assert(widget->tag == VSPC || widget->tag == HSPC);
    return;
  }
}

void contextReinit() {
  context.x = 0;
  context.y = 0;
  context.color = BRWHITE;
  context.font = &st.font;
  context.w = st.window.width;
  context.h = st.window.height;
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
void stDraw(void) {
  setDrawColor(WHITE);
  rendererClear();
  contextReinit();
  widgetDraw(gui);
  rendererPresent();
}

void windowInit(window_t *win, int width, int height) {
  memset(win, 0, sizeof(window_t));
  win->window = dieIfNull(
      SDL_CreateWindow("Editor", 0, 0, width, height,
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));

  win->width = width;
  win->height = height;
}

void message(char *s);

void stResize(void) {
  int w;
  int h;

  SDL_GetWindowSize(st.window.window, &w, &h);
  st.window.width = w;
  st.window.height = h;
  for (int i = 0; i < numFrames(); ++i) {
    frameResize(i);
  }
  // BAL: show the window size somewhere (or update config?)
  // char s[1024];
  // sprintf(s, "resize %dx%d", w, h);
  // message(s);
}

void insertNewElem() {
  backwardSOF();
  builtinInsertString("\0\n", 2);
}

// BAL: delete
/* void buffersBufInit() */
/* { */
/*   setFocusView(BUFFERS_BUF); */

/*   for(int i = 0; i < numViews(); ++i) */
/*   { */
/*     view_t *view = arrayElemAt(&st.views, i); */
/*     insertNewElem(); */
/*     builtinInsertCString(docOf(view)->filepath); */
/*   } */
/* } */

void drawFrameCursor(frame_t *frame) { drawCursorOrSelection(frame); }

void drawFrameDoc(frame_t *frame) {
  drawString(&docOf(viewOf(frame))->contents);
}

int frameScrollY(frame_t *frame) {
  view_t *view = viewOf(frame);
  return view->scrollY;
}

widget_t *frame(int i) {
  frame_t *frame = arrayElemAt(&st.frames, i);
  widget_t *textarea = wid(i, scrollY(frameScrollY, frame,
                                      over(draw(drawFrameDoc, frame),
                                           draw(drawFrameCursor, frame))));
  widget_t *status =
      over(draw(drawString, &frame->status), vspc(&st.font.lineSkip));
  widget_t *background = color(&frame->color, over(box(), hspc(&frame->width)));
  return over(vcatr(textarea, status), background);
}

void message(char *s) {
  int refFrame = focusFrameRef();
  setFocusBuiltinsView(MESSAGE_BUF);
  insertNewElem();
  builtinInsertCString(s);
  setFocusFrame(refFrame);
}

void helpBufInit(void);

void stInit(int argc, char **argv) {
  memset(&st, 0, sizeof(state_t));
  argc--;
  argv++;
  if (argc == 0) {
    printf("no input files\n");
    exit(0);
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    die(SDL_GetError());

  windowInit(&st.window, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);

  rendererInit(st.window.window);

  initFont(&st.font, INIT_FONT_FILE, INIT_FONT_SIZE);

  macrosInit();

  arrayInit(&st.docs, sizeof(doc_t));
  arrayInit(&st.frames, sizeof(frame_t));
  arrayInit(&st.replace, sizeof(char));

  keysymInit();

  for (int i = 0; i < NUM_BUILTIN_BUFFERS; ++i) {
    doc_t *doc = arrayPushUninit(&st.docs);
    docInit(doc, builtinBufferTitle[i], false, builtinBufferReadOnly[i]);
  }

  for (int i = 0; i < argc; ++i) {
    doc_t *doc = arrayPushUninit(&st.docs);
    docInit(doc, argv[i], true, false);
    docRead(doc);
  }

  for (int i = 0; i < NUM_FRAMES; ++i) {
    frame_t *v = arrayPushUninit(&st.frames);
    frameInit(v);
    if (i == BUILTINS_FRAME) {
      for (int j = 0; j < NUM_BUILTIN_BUFFERS; ++j) {
        viewInit(arrayPushUninit(&v->views), j);
      }
    } else {
      for (int j = 0; j < argc; ++j) {
        viewInit(arrayPushUninit(&v->views), NUM_BUILTIN_BUFFERS + j);
      }
    }
    setFrameView(i, 0);
  }

  gui = hcat(frame(0), hcat(frame(1), frame(2)));

  helpBufInit();

  stResize();

  setFocusBuiltinsView(HELP_BUF);
  setFocusFrame(MAIN_FRAME);
}

void saveAll() {
  if(DEMO_MODE) return;
  bool changes = false;

  for (int i = NUM_BUILTIN_BUFFERS; i < st.docs.numElems; ++i) {
    doc_t *doc = arrayElemAt(&st.docs, i);
    assert(doc);
    changes |= doc->modified;
    docWrite(doc);
  }
  if (changes && !NO_BUILD)
    system("make");
}

void quitEvent() {
  saveAll();
  TTF_Quit();
  SDL_Quit();
  exit(0);
}

void windowEvent() {
  switch (st.event.window.event) {
  case SDL_WINDOWEVENT_SIZE_CHANGED:
    stResize();
    break;
  default:
    break;
  }
}

void selectChars() {
  view_t *view = focusView();
  cursorCopy(&view->selection, &view->cursor);
  view->selectMode = CHAR_SELECT;
}

void selectLines() {
  view_t *view = focusView();
  cursorCopy(&view->selection, &view->cursor);
  view->selectMode = LINE_SELECT;
}

void selectionCancel() {
  view_t *view = focusView();
  view->selectMode = NO_SELECT;
}

int docHeight(doc_t *doc) { return docNumLines(doc) * st.font.lineSkip; }

void setFrameScrollY(frame_t *frame, int dR) {
  view_t *view = viewOf(frame);
  doc_t *doc = docOf(view);

  view->scrollY += dR * st.font.lineSkip;
  view->scrollY = clamp(frame->height - docHeight(doc) - st.font.lineSkip,
                        view->scrollY, 0);
}

void setFocusScrollY(int dR) { setFrameScrollY(focusFrame(), dR); }

void frameTrackRow(frame_t *frame, int row) {
  view_t *view = viewOf(frame);
  int height = AUTO_SCROLL_HEIGHT;
  int scrollR = view->scrollY / st.font.lineSkip;

  int dR = scrollR + row;

  if (dR < height) {
    setFrameScrollY(frame, height - dR);
  } else {
    int lastRow = frameRows(frame) - height;
    if (dR > lastRow) {
      setFrameScrollY(frame, lastRow - dR);
    }
  }
}

void focusTrackRow(int row) { frameTrackRow(focusFrame(), row); }

// BAL: do this on mouse clicks...
void focusTrackCursor() {
  view_t *view = focusView();
  focusTrackRow(st.mouseSelectionInProgress ? view->selection.row
                                            : view->cursor.row);
}

void selectionSetRowCol() {
  view_t *view = focusView();
  int column = xToColumn(st.event.button.x - st.downCxtX);
  cursorSetRowCol(&view->selection, yToRow(st.event.button.y - st.downCxtY),
                  column, focusDoc());
}

void selectionEnd() {
  view_t *view = focusView();
  view->selectMode = NO_SELECT;
}

void mouseButtonUpEvent() {
  st.mouseSelectionInProgress = false;
  selectionSetRowCol();
  view_t *view = focusView();
  if (cursorEq(&view->cursor, &view->selection))
    selectionEnd();
}

void mouseButtonDownEvent() {
  contextReinit();
  widgetAt(gui, st.event.button.x, st.event.button.y);
  if (context.wid < 0 || context.wid > numFrames()) {
    return;
  }

  st.mouseSelectionInProgress = true;
  setFocusFrame(context.wid);

  view_t *view = focusView();
  view->selectMode = CHAR_SELECT;
  if (st.event.button.button == SDL_BUTTON_RIGHT)
    view->selectMode = LINE_SELECT;

  st.downCxtX = context.x;
  st.downCxtY = context.y;

  selectionSetRowCol();
  cursorCopy(&view->cursor, &view->selection);
}

void mouseMotionEvent() {
  if (st.mouseSelectionInProgress) {
    selectionSetRowCol();
    /* int height = 1; */
    /* view_t *view = focusView(); */
    /* int scrollR = view->scrollY / st.font.lineSkip; */

    /* int dR = scrollR + view->selection.row; */
    /* if (dR < height) */
    /*   { */
    /*     setFocusScrollY(height - dR); */
    /*     SDL_PushEvent(&st.event); */
    /*   } else { */
    /*   int lastRow = frameRows(focusFrame()) - height; */
    /*   if (dR > lastRow) { */
    /*     setFocusScrollY(lastRow - dR); */
    /*     SDL_PushEvent(&st.event); */
    /*   } */
    /* } */
  }
}

void mouseWheelEvent() { setFocusScrollY(st.event.wheel.y); }

void doKeyPress(uchar c) { keyHandler[focusView()->mode][c](c); }

void recordKeyPress(uchar c) {
  int refFrame = focusFrameRef();
  if (refFrame == BUILTINS_FRAME)
    return;
  setFocusBuiltinsView(MACRO_BUF);
  builtinInsertChar(c);
  setFocusFrame(refFrame);
}

void keyDownEvent() {
  SDL_Keycode sym = st.event.key.keysym.sym;
  if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT)
    return;
  uchar c = getKeyChar(sym);
  bool wasRecording = st.isRecording;
  doKeyPress(c);
  if (wasRecording && st.isRecording) {
    recordKeyPress(c);
  }
}

void stringAppendNull(string_t *s) {
  char *c = arrayPushUninit(s);
  *c = '\0';
  arrayPop(s);
}
void doSearch(frame_t *frame, char *search) {
  assert(frame);
  assert(search);
  view_t *view = viewOf(frame);
  doc_t *doc = docOf(view);
  cursor_t *cursor = &view->cursor;

  stringAppendNull(&doc->contents);
  char *haystack = doc->contents.start;
  char *replace = dieIfNull(strdup(search));
  char *temp = replace;
  char *needle = strsep(&replace, "/");
  // needle is length 0 if no needle
  // replace is NULL if no replace
  st.searchLen = strlen(needle);
  st.isReplace = replace != NULL;
  arrayReinit(&st.replace);
  searchBuffer_t *results = &doc->searchResults;
  arrayReinit(results);
  if (st.isReplace) {
    arrayInsert(&st.replace, 0, replace, strlen(replace));
  }
  if (st.searchLen == 0)
    goto done;

  char *p = haystack;
  int *off;
  int dist = INT_MAX;

  while ((p = strcasestr(p, needle))) {
    off = arrayPushUninit(results);
    *off = p - haystack;
    p++;

    // keep closest offset
    int dist1 = *off - cursor->offset;
    dist = abs(dist1) < abs(dist) ? dist1 : dist;
  }

  // track search
  cursor_t cur;
  cursorInit(&cur);
  cursorSetOffset(&cur, cursor->offset + dist, doc);
  frameTrackRow(frame, cur.row);

done:
  free(temp);
}

char *viewElem(view_t *view) {
  int offset = view->cursor.offset;
  char *s = docOf(view)->contents.start;
  int i = distanceToStartOfElem(s, offset);
  char *p = s + offset + i;
  return p;
}

char *focusElem() {
  if (focusFrameRef() != BUILTINS_FRAME)
    return NULL;
  return viewElem(focusView());
}

void resetSearch() {
  arrayReinit(&focusDoc()->searchResults);
  st.searchLen = 0;
}

view_t *builtinsViewOf(int viewRef)
{
  assert(viewRef >= 0);
  frame_t *frame = frameOf(BUILTINS_FRAME);
  assert(viewRef < frame->views.numElems);
  return arrayElemAt(&frame->views, viewRef);
}

void recomputeSearch() {
  // get search elem
  char *search = viewElem(builtinsViewOf(SEARCH_BUF));

  if (!search)
    return;

  // search main frame doc
  frame_t *frame = frameOf(MAIN_FRAME);
  doc_t *doc = docOf(viewOf(frame));
  doSearch(frame, search);

  // search secondary frame doc
  frame = frameOf(SECONDARY_FRAME);
  doc_t *doc1 = docOf(viewOf(frame));
  doSearch(frame, search);
}

void updateBuiltinsState(bool isModify) {
  if (focusViewRef() != SEARCH_BUF) {
    if (isModify)
      resetSearch();
    return;
  }
  switch() {
  case SEARCH_BUF:
    recomputeSearch();
    break;
  default:
    // do nothing
    break;
  }
}

void stMoveCursorOffset(int offset) {
  cursorSetOffset(focusCursor(), offset, focusDoc());
  focusTrackCursor();
  updateBuiltinsState(false);
}

void stMoveCursorRowCol(int row, int col) {
  cursorSetRowCol(focusCursor(), row, col, focusDoc());
  focusTrackCursor();
  updateBuiltinsState(false);
}

void backwardChar() { stMoveCursorOffset(focusCursor()->offset - 1); }

void forwardChar() { stMoveCursorOffset(focusCursor()->offset + 1); }

void setNavigateMode() { focusView()->mode = NAVIGATE_MODE; }

void setInsertMode() {
  if (focusDoc()->isReadOnly)
    return;

  focusView()->mode = INSERT_MODE;
}

void setMode(editorMode_t mode) {
  switch (mode) {
  case INSERT_MODE:
    setInsertMode();
    return;
  default:
    assert(mode == NAVIGATE_MODE);
    setNavigateMode();
    return;
  }
}

void doKeyPressInNavigateMode(uchar c) {
  int mode = focusView()->mode;
  setNavigateMode();
  doKeyPress(c);
  setMode(mode);
}

void moveLines(int dRow) {
  cursor_t *cursor = focusCursor();
  int col = cursor->preferredColumn;

  stMoveCursorRowCol(cursor->row + dRow, col);
  cursor->preferredColumn = col;
}

void backwardLine() { moveLines(-1); }

void forwardLine() { moveLines(1); }

char nextCharAfterSpaces() {
  doc_t *doc = focusDoc();
  char *p = doc->contents.start + focusCursor()->offset;
  char *end = arrayTop(&doc->contents);

  while (p < end && *p == ' ')
    p++;
  return p == end ? '\0' : *p;
}

void forwardBlankLine() {
  backwardSOL();
  char r = nextCharAfterSpaces();
  while (r != '\0' && r != '\n') {
    forwardLine();
    r = nextCharAfterSpaces();
  }
  while (r != '\0' && r == '\n') {
    forwardLine();
    r = nextCharAfterSpaces();
  }
}

bool isSOF() { return focusCursor()->offset == 0; }

void backwardBlankLine() {
  backwardSOL();
  char r = nextCharAfterSpaces();
  while (!isSOF() && r != '\n') {
    backwardLine();
    r = nextCharAfterSpaces();
  }
  while (!isSOF() && r == '\n') {
    backwardLine();
    r = nextCharAfterSpaces();
  }
}

void forwardSpace() {
  int i = focusCursor()->offset;
  doc_t *doc = focusDoc();
  stMoveCursorOffset(i + distanceToNextSpace(doc->contents.start + i,
                                             arrayTop(&doc->contents)));
}

void backwardSpace() {
  int i = focusCursor()->offset;
  doc_t *doc = focusDoc();
  stMoveCursorOffset(
      i + distanceToPrevSpace(doc->contents.start, doc->contents.start + i));
}

void backwardPage() {
  moveLines(-max(1, frameRows(focusFrame()) - AUTO_SCROLL_HEIGHT));
}

void forwardPage() {
  moveLines(max(1, frameRows(focusFrame()) - AUTO_SCROLL_HEIGHT));
}

void backwardSOF() { stMoveCursorOffset(0); }

void forwardEOF() { stMoveCursorOffset(INT_MAX - 1); }

void backwardSOL() { stMoveCursorRowCol(focusCursor()->row, 0); }

void forwardEOL() { stMoveCursorRowCol(focusCursor()->row, INT_MAX); }

void insertString(char *s, uint len) {
  assert(s);
  if (len <= 0)
    return;
  docPushInsert(focusDoc(), focusCursor()->offset, s, len);
}

void builtinInsertString(char *s, uint len) {
  assert(s);
  if (len <= 0)
    return;
  docInsert(focusDoc(), focusCursor()->offset, s, len);
}

void insertCString(char *s) {
  assert(s);
  insertString(s, (uint)strlen(s));
}

void builtinInsertCString(char *s) {
  assert(s);
  builtinInsertString(s, (uint)strlen(s));
}

void insertChar(uchar c) {
  insertString((char *)&c, 1);
  forwardChar();
}

void builtinInsertChar(uchar c) {
  builtinInsertString((char *)&c, 1);
  forwardChar();
}

void builtinAppendCString(char *s) {
  forwardEOF();
  builtinInsertCString(s);
}

void helpAppendKeysym(uchar c, char *s) {
  char buf[1024];
  sprintf(buf, "'%s': %s\n", keysymName(c), s);
  builtinAppendCString(buf);
}

void helpBufInit() {
  setFocusBuiltinsView(HELP_BUF);

  builtinAppendCString("Builtin Keys:\n");
  for (int i = 0; i < NUM_MODES; ++i) {
    for (int c = 0; c < NUM_KEYS; ++c) {
      if (!keyHandlerHelp[i][c])
        continue;
      helpAppendKeysym(c, keyHandlerHelp[i][c]);
    }
  }

  for (int i = 0; i < NUM_BUILTIN_MACROS; ++i) {
    helpAppendKeysym(builtinMacros[i][0], builtinMacrosHelp[i]);
  }
}

void backwardStartOfElem() // BAL: remove(?)
{
  int offset = focusView()->cursor.offset;
  int i = distanceToStartOfElem(focusDoc()->contents.start, offset);
  stMoveCursorOffset(offset + i);
}

void copyElemToClipboard() {
  backwardStartOfElem();
  char *s = focusDoc()->contents.start + focusView()->cursor.offset;
  setClipboardText(s);
}

void pasteBefore() {
  char *s = getClipboardText();
  if (s)
    insertCString(s);
}

void docPushCommand(commandTag_t tag, doc_t *doc, int offset, char *s,
                    int len) {
  command_t *cmd;
  while (doc->undoStack.offset < doc->undoStack.numElems) {
    cmd = arrayPop(&doc->undoStack);
    arrayFree(&cmd->string);
  }
  cmd = arrayPushUninit(&doc->undoStack);
  cmd->tag = tag;
  cmd->offset = offset;
  arrayInit(&cmd->string, sizeof(char));
  arrayInsert(&cmd->string, 0, s, len);
  doc->undoStack.offset++;
  assert(doc->undoStack.offset == doc->undoStack.numElems);
}

int docPushDelete(doc_t *doc, int offset, int len) {
  if (len <= 0)
    return 0;
  if (doc->isReadOnly)
    return 0;
  int n = docDelete(doc, offset, len);
  if (doc->isUserDoc && n > 0) {
    docPushCommand(DELETE, doc, offset, doc->contents.start + offset, n);
  }
  updateBuiltinsState(true);
  return n;
}

void docPushInsert(doc_t *doc, int offset, char *s, int len) {
  if (len <= 0)
    return;
  if (doc->isReadOnly)
    return;
  if (doc->isUserDoc) {
    docPushCommand(INSERT, doc, offset, s, len);
  }
  docInsert(doc, offset, s, len);
  updateBuiltinsState(true);
}

void docDoCommand(doc_t *doc, commandTag_t tag, int offset, string_t *string) {
  selectionCancel();
  stMoveCursorOffset(offset);
  switch (tag) {
  case DELETE:
    docDelete(doc, offset, string->numElems);
    return;
  default:
    assert(tag == INSERT);
    docInsert(doc, offset, string->start, string->numElems);
    return;
  }
}

void undo() {
  doc_t *doc = focusDoc();
  if (doc->undoStack.offset == 0)
    return;
  doc->undoStack.offset--;
  command_t *cmd = arrayFocus(&doc->undoStack);
  docDoCommand(doc, !cmd->tag, cmd->offset, &cmd->string);
}

void redo() {
  doc_t *doc = focusDoc();
  if (doc->undoStack.offset == doc->undoStack.numElems)
    return;
  command_t *cmd = arrayFocus(&doc->undoStack);
  docDoCommand(doc, cmd->tag, cmd->offset, &cmd->string);
  doc->undoStack.offset++;
}

void copy(char *s, uint len) {
  int refFrame = focusFrameRef();
  setFocusBuiltinsView(COPY_BUF);
  insertNewElem();
  builtinInsertString(s, len);
  setClipboardText(focusDoc()->contents.start);
  setFocusFrame(refFrame);
}

void cut() {
  int column;
  int row;
  int offset;
  int length;

  view_t *view = focusView();
  doc_t *doc = focusDoc();

  getSelectionCoords(view, &column, &row, &offset, &length);

  selectionCancel();

  length = min(length, doc->contents.numElems - offset);

  if (length <= 0)
    return;

  if (length > 1)
    copy(doc->contents.start + offset, length);
  docPushDelete(doc, offset, length);
  cursorSetRowCol(&view->cursor, row, column, focusDoc());
}

void playMacroString(char *s, uint len) {
  char *done = s + len;

  while (s < done) {
    doKeyPress(*s);
    s++;
  }
}

void playMacroCString(char *s) { playMacroString(s, (uint)strlen(s)); }

void doNothing(uchar c) {
  if (macro[c]) {
    playMacroCString(macro[c]);
    return;
  }
  selectionEnd();
}

void stopRecording() {
  int refFrame = focusFrameRef();
  st.isRecording = false;
  setFocusBuiltinsView(MACRO_BUF);
  backwardStartOfElem();
  setFocusFrame(refFrame);
}

void startOrStopRecording() {
  int refFrame = focusFrameRef();

  if (st.isRecording || refFrame == BUILTINS_FRAME) {
    stopRecording();
    return;
  }

  // start recording
  st.isRecording = true;
  setFocusBuiltinsView(MACRO_BUF);
  insertNewElem();
  // BAL: if 1st line in macro buf is empty delete it (or reuse it)
  setFocusFrame(refFrame);
}

void stopRecordingOrPlayMacro() {
  int refFrame = focusFrameRef();

  if (st.isRecording || refFrame == BUILTINS_FRAME ||
      focusViewRef() == MACRO_BUF) {
    stopRecording();
    return;
  }

  // play macro
  setFocusBuiltinsView(MACRO_BUF);
  backwardStartOfElem();
  doc_t *doc = focusDoc();
  view_t *view = focusView();

  if (!doc->contents.start) {
    message("no macros to play");
    setFocusFrame(refFrame);
    return;
  }

  setFocusFrame(refFrame);
  playMacroCString(doc->contents.start + view->cursor.offset);
}

/* void setCursorToSearch() */
/* { */
/*     frame_t *frame = focusFrame(); */
/*     if (noActiveSearch(frame)) return; */
/*     view_t *view = focusView(); */
/*     doc_t *doc = focusDoc(); */
/*     uint *off = arrayFocus(&st.searchResults); */
/*     cursorSetOffset(&view->cursor, *off, doc); */
/*     cursorSetOffset(&view->selection, *off + st.searchLen - 1, doc); */
/* } */

void forwardSearch() {
  doc_t *doc = focusDoc();
  searchBuffer_t *results = &doc->searchResults;
  if (results->numElems == 0)
    {
      recomputeSearch();
      return;
    }
  cursor_t *cursor = focusCursor();

  for (int i = 0; i < results->numElems; ++i) {
    int *offset = arrayElemAt(results, i);
    if (*offset > cursor->offset) {
      stMoveCursorOffset(*offset);
      //  cursorSetOffset(&view->selection, *off + st.searchLen - 1, doc);
      return;
    }
  }
  int *offset = arrayElemAt(results, 0);
  stMoveCursorOffset(*offset);
  /*     st.searchResults.offset++; */
  /*     st.searchResults.offset %= st.searchResults.numElems; */
  /*     setCursorToSearch(); */
}

void backwardSearch() {
  doc_t *doc = focusDoc();
  searchBuffer_t *results = &doc->searchResults;
  if (results->numElems == 0)
    {
      recomputeSearch();
      return;
    }
  cursor_t *cursor = focusCursor();

  int last = results->numElems - 1;
  for (int i = last; i >= 0; --i) {
    int *offset = arrayElemAt(results, i);
    if (*offset < cursor->offset) {
      stMoveCursorOffset(*offset);
      //  cursorSetOffset(&view->selection, *off + st.searchLen - 1, doc);
      return;
    }
  }
  int *offset = arrayElemAt(results, last);
  stMoveCursorOffset(*offset);
  /*     st.searchResults.offset += st.searchResults.numElems - 1; */
  /*     st.searchResults.offset %= st.searchResults.numElems; */
  /*     setCursorToSearch(); */
}

/* void setCursorToSearch(); */

/* void computeSearchResults() */
/* { */
/*     view_t *view = arrayElemAt(&st.views, st.searchRefView); */
/*     doc_t *doc = docOf(view); */

/*     int refView = focusViewRef(); */
/*     setFocusBuiltinsView(SEARCH_BUF); */

/*     view_t *viewFind = focusView(); */
/*     doc_t *find = focusDoc(); */

/*     cursor_t cursorFind; */
/*     cursorCopy(&cursorFind, &viewFind->cursor); */
/*     cursorSetRowCol(&cursorFind, cursorFind.row, 0, find); */

/*     char *search = find->contents.start + cursorFind.offset; */
/*     if (!search) goto nofree; */
/*     char *replace = dieIfNull(strdup(search)); */
/*     char *needle = strsep(&replace, "/"); */
/*     st.searchLen = (uint)strlen(needle); */
/*     if (st.searchLen == 0) goto done; */

/*     char *haystack = doc->contents.start; */
/*     char *p = haystack; */
/*     arrayReinit(&st.searchResults); */
/*     while ((p = strcasestr(p, needle))) */
/*     { */
/*         uint *off = arrayPushUninit(&st.searchResults); */
/*         *off = (uint)(p - haystack); */
/*         p++; */
/*     } */

/*     // copy replace string to copy buffer // BAL: don't use copy buffer ...
 */
/*     if (replace) copy(replace, (uint)strlen(replace)); */
/* done: */
/*     free(needle); */
/* nofree: */
/*     setFocusBuiltinsView(refView); */
/* } */

void newSearch() {
  st.searchFrameRef = focusFrameRef(); // BAL: last user frame ref
  setFocusBuiltinsView(SEARCH_BUF);
  setInsertMode();
  insertNewElem();
}

/* void gotoView() */
/* { */
/* int frame = st.frames.offset; */

/* if (frame == BUILTINS_FRAME) frame = MAIN_FRAME; */

/* setFocusFrame(BUILTINS_FRAME); */
/* stSetViewFocus(BUFFERS_BUF); */

/* view_t *view = focusView(); */

/* setFocusFrame(frame); */
/* stSetViewFocus(view->cursor.row); */
/* } */

void forwardFrame() { setFocusFrame((focusFrameRef() + 1) % numFrames()); }

void backwardFrame() {
  setFocusFrame((focusFrameRef() + numFrames() - 1) % numFrames());
}

void forwardView() { setFocusView((focusViewRef() + 1) % numViews()); }

void backwardView() {
  setFocusView((focusViewRef() + numViews() - 1) % numViews());
}

#define INDENT_LENGTH 2

int numLinesSelected(char *s, int len) {
  if (len <= 0)
    return 0;
  int n = numLinesString(s, len);
  if (*(s + len - 1) != '\n')
    n++;
  return n;
}

void insertChars(char c, int n) {
  assert(n < 1024);
  char s[1024];
  memset(s, c, n);
  insertString(s, n);
}

int indentLine() {
  string_t *s = &focusDoc()->contents;
  backwardSOL();
  int offset = focusCursor()->offset;
  int x = distanceToIndent(s->start + offset, arrayTop(s));
  int n = INDENT_LENGTH - x % INDENT_LENGTH;
  insertChars(' ', n);
  return n;
}

int outdentLine() {
  doc_t *doc = focusDoc();
  string_t *s = &doc->contents;
  backwardSOL();
  int offset = focusCursor()->offset;
  int x = distanceToIndent(s->start + offset, arrayTop(s));
  int n = x == 0 ? 0 : (x - (((x - 1) / INDENT_LENGTH) * INDENT_LENGTH));
  return docPushDelete(doc, offset, n);
}

void indent() {
  int col;
  int row;
  int off;
  int len;

  getSelectionCoords(focusView(), &col, &row, &off, &len);

  int n = numLinesSelected(focusDoc()->contents.start + off, len);

  int coff = focusCursor()->offset;
  int soff = focusSelection()->offset;
  int m = indentLine();
  int m0 = m;

  if (soff >= coff) { // left to right highlight order
    for (int i = 1; i < n; ++i) {
      forwardLine();
      m += indentLine();
    }
    cursorSetOffset(focusCursor(), coff + m0, focusDoc());
    cursorSetOffset(focusSelection(), soff + m, focusDoc());
    return;
  }

  // coff > soff // right to left highlight order
  for (int i = 1; i < n; ++i) {
    backwardLine();
    m0 = indentLine();
    m += m0;
  }

  cursorSetOffset(focusCursor(), coff + m, focusDoc());
  cursorSetOffset(focusSelection(), soff + m0, focusDoc());
}
void outdent() {
  int col;
  int row;
  int off;
  int len;

  getSelectionCoords(focusView(), &col, &row, &off, &len);

  int n = numLinesSelected(focusDoc()->contents.start + off, len);

  int coff = focusCursor()->offset;
  int soff = focusSelection()->offset;
  int m = outdentLine();
  int m0 = m;

  if (soff >= coff) { // left to right highlight order
    for (int i = 1; i < n; ++i) {
      forwardLine();
      m += outdentLine();
    }
    cursorSetOffset(focusCursor(), coff - m0, focusDoc());
    cursorSetOffset(focusSelection(), soff - m, focusDoc());
    return;
  }

  // coff > soff // right to left highlight order
  for (int i = 1; i < n; ++i) {
    backwardLine();
    m0 = outdentLine();
    m += m0;
  }

  cursorSetOffset(focusCursor(), coff - m, focusDoc());
  cursorSetOffset(focusSelection(), soff - m0, focusDoc());
}
/* void timerEvent() */
/* { */
/*   if (st.mouseSelectionInProgress) */
/*     { */
/*       focusTrackCursor(4); */
/*       printf("boom\n"); */
/*     } */
/* } */
/* Uint32 timerInit(Uint32 interval, void *param) */
/* { */
/*   SDL_Event event; */
/*   SDL_UserEvent userevent; */

/*   userevent.type = SDL_USEREVENT; */
/*   userevent.code = 0; */
/*   userevent.data1 = NULL; */
/*   userevent.data2 = NULL; */

/*   event.type = SDL_USEREVENT; */
/*   event.user = userevent; */

/*   SDL_PushEvent(&event); */
/*   return(interval); */
/* } */

uchar lookupCloseChar(uchar c) {
  switch (c) {
  case '(':
    return ')';
  case '[':
    return ']';
  case '{':
    return '}';
  case '\'':
    return '\'';
  default:
    assert(c == '"');
    return '"';
  }
}

void insertOpenCloseChars(uchar c) {
  uchar c1 = lookupCloseChar(c);

  int col;
  int row;
  int off;
  int len;
  view_t *view = focusView();

  if (selectionActive(view)) {
    getSelectionCoords(view, &col, &row, &off, &len);
    stMoveCursorOffset(off);
    insertChar(c);
    stMoveCursorOffset(off + len + 1);
    insertChar(c1);
    selectionCancel();
    return;
  }
  // if selection is on
  // insert begin/end punctuation around selection
  // otherwise
  //   insert begin/end punctuation and then go to insert mode in the middle
  insertChar(c);
  insertChar(c1);
  backwardChar();
  setInsertMode();
}
void doEscape() {
  setNavigateMode();
  if (focusFrameRef() == BUILTINS_FRAME && focusViewRef() == SEARCH_BUF) {
    setFocusFrame(st.searchFrameRef);
  }
}
int main(int argc, char **argv) {
  assert(sizeof(char) == 1);
  assert(sizeof(int) == 4);
  assert(sizeof(unsigned int) == 4);
  assert(sizeof(void *) == 8);

  stInit(argc, argv);
  stDraw();
  //    SDL_AddTimer(1000, timerInit, NULL);
  while (SDL_WaitEvent(&st.event)) {
    switch (st.event.type) {
    case SDL_WINDOWEVENT:
      windowEvent();
      break;
    case SDL_MOUSEWHEEL:
      mouseWheelEvent();
      break;
    case SDL_MOUSEBUTTONDOWN:
      mouseButtonDownEvent();
      break;
    case SDL_MOUSEBUTTONUP:
      mouseButtonUpEvent();
      break;
    case SDL_MOUSEMOTION:
      mouseMotionEvent();
      break;
    case SDL_KEYDOWN:
      keyDownEvent();
      break;
    case SDL_QUIT:
      quitEvent();
      break;
    case SDL_USEREVENT:
      //                timerEvent();
      break;
    default:
      break;
    }
    frameUpdate(focusFrame());
    stDraw();
    /* { */
    /*     computeSearchResults(&st); */
    /* } */
    /* { */
    /*   stDraw(); */
    /*   stRenderFull(&st); */
    /* { */
    /*     stRender(&st); */
    /* } */
  }
  return 0;
}

/*
  search:
  - do search interactive when in search buffer
  - do search on current line from search buffer
  - move forward/back among search items
  - clear search
  - do replace
  /
 */
/*
TODO:
CORE:
    do brace insertion on different lines
    reload file when changed outside of editor (inotify?  polling?)
    periodically save all (modified) files
    add pretty-print hotkey
    remember your place in the file on close (restore all settings/state?)
    modify frame widths based on columns (e.g. focus doc frame is 80/120 chars)
    Search and replace
    Goto line
    Scroll when mouse selection goes off screen command line
    args/config to set config items (e.g. demo mode)
    status bar that has modified,etc.
    cut/copy/paste with mouse
    create new file
    input screen (for search, paste,load, tab completion, etc.)
    Hook Cut/Copy/Paste into system Cut/Copy/Paste
    support editor API
    Name macro
    highlight compile errors/warnings
    jump to next error
    tab completion
    sort lines
    check spelling
    jump to prev/next placeholders
    jump to prev/next change
    collapse/expand selection (code folding)
    (tons of) optimizations
    make builtin buffers readonly (except config and search?)

VIS:
    make font smaller in non-focused frames
    italics, underline and bold in syntax highlighting
    reserved words syntax highlighting
    context sensitive syntax highlighting (parser)
    Put border on frames
    Make the active frame the biggest (both in width and font size)
    Make builtin frame very thin when not in use
    Make frame widths resizeable
    slightly change color of status bar
    Display line number of every line(hmmm, takes up horiz space)
    Minimap
    highlight matching paren
    resize nested parens
    display multiple characters as one (e.g. ->)
    buffer list screen

MACRO:
    comment/uncomment region
    more move, delete, insert commands
    select all

DONE:
add save/build hotkey
insert both open/close characters (e.g. "" or {}).  If selection is on, use
that for contents.
syntax highlighting
continual compile (on exiting insert or delete)
help screen
Undo/Redo commands
Display operation buffer during operation
multiple files
Track cursor when it goes off screen
make each frame have a list of independent views
Move to next/prev blank line
indent/outdent region
indent/outdent line
switch between views
switch between frames
status bar that has filename, row/col, mode.
cursor per frame
select region with mouse
status bar per frame
split screen
Record/play macro
Scroll
Move cursor on mouse click
File automatically loads on startup
File automatically saves on exit
Select region
Cut/Copy/Paste
Move cursor to the start/end of a line
Move cursor up/down
Move cursor to next/prev whitespace
Insert characters
Delete characters
Enforce cursor boundaries
wait for events, don't poll
switch between insert and navigation mode
Display filename and mode in status bar
Redraw on window size change
Display cursor
Move cursor left/right
Display text with spaces and newlines
Display text
Display window
exit on quit
*/
