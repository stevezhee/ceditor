#include "Cursor.h"
#include "Doc.h"
#include "DynamicArray.h"
#include "Font.h"
#include "Keysym.h"
#include "Util.h"
#include "Search.h"

void insertCString(char *s);
void insertString(char *s, uint len);

void frameTrackSelection(frame_t *frame);
void frameScrollY(frame_t *frame, int dR);

color_t unfocusedFrameColor = 0x303030ff;
color_t focusedFrameColor = 0x101010ff;

typedef enum { HSPC, VSPC, DRAW, COLOR, FONT, SCROLL_Y, WID, OVER, HCAT, VCAT, HCATR, VCATR, NUM_WIDGET_TAGS } widgetTag_t;

struct widget_s;

typedef struct widget_s widget_t;

typedef struct singletonData_s
{
  void *a;
  widget_t *child;
} singletonData_t;

typedef struct pairData_s
{
  widget_t *a;
  widget_t *b;
} pairData_t;

struct widget_s
{
  widgetTag_t tag;
  union {
    int wid;
    int *length;
    int *color;
    int *dy;
    font_t **font;
    widget_t *child;
    void (*drawFun)(void *);
    void *data;
  } a;
  union {
    widget_t *child;
    void *data;
  } b;
};

state_t st;
widget_t *gui;
int xToColumn(int x)
{
  return x / st.font.charSkip;
}
int yToRow(int x)
{
  return x / st.font.lineSkip;
}

int numViews()
{
  return st.views.numElems;
}

int numFrames()
{
  return st.frames.numElems;
}

int numDocs()
{
  return st.docs.numElems;
}

int frameHeight(frame_t *frame)
{
    return frame->rect.h;
}

uint frameRows(frame_t *frame)
{
    return frameHeight(frame) / st.font.lineSkip;
}

int getFocusFrame()
{
  return st.frames.offset;
}

frame_t *focusFrame()
{
  return arrayFocus(&st.frames);
}

void setFocusFrame(int i)
{
  int j = getFocusFrame();
  if (i != j)
  {
    st.lastFocusFrame = j;
    frame_t *frame = focusFrame();
    frame->color = unfocusedFrameColor;
    arraySetFocus(&st.frames, i);
    frame = focusFrame();
    frame->color = focusedFrameColor;
    // BAL: !!!!!!!!!!!!!!!! YOU ARE HERE ...
    // BAL: sort out all this push/pop view nonsense...
      // printf("file:%s\n", focusDoc()->filepath);
  }
}

frame_t *frameOf(int i)
{
  return arrayElemAt(&st.frames, i);
}

int getViewFocus()
{
  return focusFrame()->refView;
}

view_t *viewOf(frame_t *frame)
{
  return arrayElemAt(&st.views, frame->refView);
}

view_t *focusView()
{
  return viewOf(focusFrame());
}

doc_t *docOf(view_t *view)
{
  return arrayElemAt(&st.docs, view->refDoc);
}

doc_t *focusDoc()
{
  return docOf(focusView());
}

void frameUpdate(frame_t *frame)
{
  view_t *view = viewOf(frame);
  doc_t *doc = docOf(view);
  // BAL: make sure we don't overflow the buffer
  arrayReinit(&frame->status);
  sprintf(frame->status.start, "<%s> %3d:%2d %s", editorModeDescr[view->mode], view->cursor.row + 1, view->cursor.column, doc->filepath);
  frame->status.numElems = strlen(frame->status.start);
}

void setFrameView(int refFrame, int refView)
{
  frame_t *frame = frameOf(refFrame);
  frame->refView = refView;

  view_t *view = viewOf(frame);
  frame->scrollY = &view->scrollY;

  frameUpdate(frame);
}

void setFocusView(int refView)
{
  setFrameView(getFocusFrame(), refView);
}

/* void stSetViewFocus(uint i) */
/* { */
/*     i = clamp(0, i, st.views.numElems - 1); */
/*     frame_t *frame = focusFrame(); */
/*     frame->refView = i; */
/*     printf("file:%s\n", focusDoc()->filepath); */
/* } */

void frameInit(frame_t *frame)
{
    memset(frame, 0, sizeof(frame_t));
    frame->color = unfocusedFrameColor;

    arrayInit(&frame->status, sizeof(char));
    arrayGrow(&frame->status, 1024);
    // printf("frame init: %d\n", (int)frame->status.start);
    // printf("frame init status: %p %p %d %d\n", frame->status, frame->status.start, frame->status->numElems, frame->status->maxElems);
    //    printf("status size buf=%p start=%p\n", frame->status, frame->status.start);

}

void viewInit(view_t *view, uint i)
{
    memset(view, 0, sizeof(view_t));
    view->refDoc = i;
}

void docRender(doc_t *doc, frame_t *frame)
{
    /* resetCharRect(&st.font, frame->scrollX, frame->scrollY); */

    /* if(!doc->contents.start) return; */

    /* for(char *p = doc->contents.start; p < (char*)(doc->contents.start) + doc->contents.numElems; p++) */
    /* { */
    /*     renderAndAdvChar(&st.font, *p); */
    /* } */

    /* renderEOF(&st.font); */
}

bool noActiveSearch(frame_t *frame)
{
    return
      frame->refView != st.searchRefView ||
      st.searchLen == 0 ||
      st.results.numElems == 0;
}

widget_t *newWidget()
{
  return dieIfNull(malloc(sizeof(widget_t)));
}

widget_t *leaf(widgetTag_t tag, void *a)
{
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.data = a;
  return p;
}

#define hspc(len) leaf(HSPC, len)
#define vspc(len) leaf(VSPC, len)

widget_t *singleton(widgetTag_t tag, void *a, widget_t *b)
{
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.child = a;
  p->b.child = b;
  return p;
}

#define color(a, b) singleton(COLOR, a, b)
#define font(a, b) singleton(FONT, a, b)
#define scrollY(a, b) singleton(SCROLL_Y, a, b)

widget_t *wid(int i, widget_t *a)
{
  widget_t *p = newWidget();
  p->tag = WID;
  p->a.wid = i;
  p->b.child = a;
  return p;
}

widget_t *node(widgetTag_t tag, void *a, void *b)
{
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.data = a;
  p->b.data = b;
  return p;
}

typedef struct
{
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

void drawBox(void *_unused)
{
  fillRect(context.w, context.h);
}

#define over(a, b) node(OVER, a, b)
#define hcat(a, b) node(HCAT, a, b)
#define hcatr(a, b) node(HCATR, a, b)
#define vcat(a, b) node(VCAT, a, b)
#define vcatr(a, b) node(VCATR, a, b)
#define draw(a, b) node(DRAW, a, b)
#define box() node(DRAW, drawBox, NULL)

int maxLineLength(char *s)
{
  assert(s);
  char c;
  int rmax = 0;
  int r = 0;
  while((c = *s) != '\0')
    {
      switch(c) {
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

int numLinesCString(char *s)
{
  int n = 0;
  char c;
  while ((c = *s) != '\0')
    {
      if (c == '\n') n++;
      s++;
    }
  return n;
}

void drawStringSelection(int x, int y, char *s, int len)
{
  // BAL: would it look good to bold the characters in addition/instead?
  assert(s);
  int w = context.font->charSkip;
  int h = context.font->lineSkip;
  char *p = s + len;
  while(s < p)
    {
      fillRectAt(x, y, w, h);
      switch(*s) {
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

void drawString(string_t *s)
{
  if(!s) return;
  assert(s);
  if (!s->start) return;
  uchar c;
  SDL_Texture *txtr;
  SDL_Rect rect;
  rect.x = 0; // context.dx;
  rect.y = context.dy;
  rect.w = context.font->charSkip;
  rect.h = context.font->lineSkip;

  char *p = s->start;
  char *q = arrayTop(s);

  while(p < q)
    {
      c = *p;
      p++;
      switch(c) {
      case '\n':
        rect.x = 0; // context.dx;
        rect.y += rect.h;
        break;
      case ' ':
        rect.x += rect.w;
        break;
      default:
        txtr = context.font->charTexture[c];
        setTextureColorMod(txtr, context.color); // BAL: could just do this once if using a texture map
        SDL_RenderCopy(renderer, txtr, NULL, &rect);
        rect.x += rect.w;
      }
    }
}

int widgetWidth(widget_t *widget)
{
  int w = 0;

  switch (widget->tag) {
  case HCATR: // fall through
  case HCAT:
    w = widgetWidth(widget->a.child) + widgetWidth(widget->b.child);
    break;
  case OVER: // fall through
  case VCATR: // fall through
  case VCAT:
    w = max(widgetWidth(widget->a.child), widgetWidth(widget->b.child));
    break;
  case SCROLL_Y: // fall through
  case WID: // fall through
  case COLOR: // fall through
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

int widgetHeight(widget_t *widget)
{
  int h = 0;
  switch (widget->tag)
    {
    case OVER: // fall through
    case HCATR: // fall through
    case HCAT:
      h = max(widgetHeight(widget->a.child), widgetHeight(widget->b.child));
      break;
    case VCATR: // fall through
    case VCAT:
      h = widgetHeight(widget->a.child) + widgetHeight(widget->b.child);
      break;
    case SCROLL_Y: // fall through
    case COLOR: // fall through
    case WID: // fall through
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
void contextSetViewport()
{
  SDL_Rect rect;
  rect.x = context.x;
  rect.y = context.y;
  rect.w = context.w;
  rect.h = context.h;
  setViewport(&rect);
}

int columnToX(int column)
{
  return column * st.font.charSkip;
}

int rowToY(int row)
{
  return row * st.font.lineSkip + context.dy;
}

void drawCursor(view_t *view)
{
  int x = columnToX(view->cursor.column);
  int y = rowToY(view->cursor.row);

  if (view->mode == NAVIGATE_MODE)
    {
      setDrawColor(CURSOR_BACKGROUND_COLOR);
      fillRectAt(x, y, st.font.charSkip, st.font.lineSkip);
    }

  setDrawColor(CURSOR_COLOR);
  fillRectAt(x, y, CURSOR_WIDTH, st.font.lineSkip);

  setDrawColor(context.color);
}

int distanceToEOL(char *s)
{
  char *p = s;
  char c = *p;

  while(c != '\n' && c != '\0')
    {
      p++;
      c = *p;
    }
  return p - s;
}

int offsetOfStartOfElem(char *s, int offset)
{
  char *p = s + offset;
  char *p0 = p;
  while(p > s)
    {
      if (*p == '\0') return p - s + 2;
      p--;
    }
  return 0;
}

void getSelectionCoords(view_t *view, int *column, int *row, int *offset, int *len)
{
  cursor_t *a = &view->cursor;
  cursor_t *b = &view->selection;

  if (a->offset > b->offset)
    {
      swap(cursor_t*, a, b);
    }

  *offset = a->offset;
  *len = b->offset - *offset;
  *row = a->row;
  *column = a->column;

  char *s = docOf(view)->contents.start;

  if(view->selectMode == LINE_SELECT)
    {
      *len += distanceToEOL(s + *offset + *len);
      *offset -= *column;
      *len += *column;
      *column = 0;
    }
  *len += 1;
}

void drawSelection(view_t *view)
{

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

bool cursorEq(cursor_t *a, cursor_t *b)
{
  return (a->row == b->row && a->column == b->column);
}

bool selectionActive(view_t *view)
{
  return (view->selectMode != NO_SELECT);
}

void drawCursorOrSelection(view_t *view)
{
  if (selectionActive(view))
    {
      drawSelection(view);
      return;
    }
  drawCursor(view);
}

void widgetAt(widget_t *widget, int x, int y)
{
  assert(context.w >= 0);
  assert(context.h >= 0);

  if (x >= context.w || y >= context.h)
    {
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
  case FONT: // fall through
    widgetAt(widget->b.child, x, y);
    return;
  case SCROLL_Y:
    context.y += *widget->a.dy;
    widgetAt(widget->b.child, x, y + *widget->a.dy);
    return;
  default:
    assert(widget->tag == VSPC || widget->tag == HSPC || widget->tag == DRAW);
    return;
  }
}
void widgetDraw(widget_t *widget)
{
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
    context.dy += *widget->a.dy;
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

void contextReinit()
{
  context.x = 0;
  context.y = 0;
  context.color = 0xffffffff;
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
void stDraw(void)
{
  setDrawColor(0x00ff00ff);
  rendererClear();
  contextReinit();
  widgetDraw(gui);
  rendererPresent();
}

char systemBuf[1024];  // BAL: use string_t

void gitInit(void)
{
    if (system("git inint") != 0) die("git init failed");
}

void docGitAdd(doc_t *doc)
{
    if (DEMO_MODE || NO_GIT) return;
    sprintf(systemBuf, "git add %s", doc->filepath);
    system(systemBuf);
}

void docGitCommit(doc_t *doc)
{
    if (DEMO_MODE || NO_GIT) return;
    docGitAdd(doc);
    sprintf(systemBuf, "git commit -m\"cp\" %s", doc->filepath);
    system(systemBuf);
}
// BAL: put back in docGitCommit(doc);

void windowInit(window_t *win, int width, int height)
{
    win->window = dieIfNull(SDL_CreateWindow("Editor", 0, 0, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));

    win->width = width;
    win->height = height;
}

void frameResize(frame_t *frame)
{
  frame->width = st.window.width / 3;
  frame->height = st.window.height - st.font.lineSkip;
}

void stResize(void)
{
    int w;
    int h;

    SDL_GetWindowSize(st.window.window, &w, &h);
    st.window.width = w;
    st.window.height = h;
    for(int i = 0; i < numFrames(); ++i)
      {
        frameResize(frameOf(i));
      }
}

void message(char *s)
{
  int refView = getViewFocus();
  setFocusView(MESSAGE_BUF);
  insertCString(s);
  setFocusView(refView);
}

void helpBufInit()
{
  setFocusView(HELP_BUF);
  insertCString("here is some help text.");
}

void buffersBufInit()
{
  setFocusView(BUFFERS_BUF);

  for(int i = 0; i < numViews(); ++i)
  {
    backwardSOF(); // BAL: forwardEOF
    view_t *view = arrayElemAt(&st.views, i);
    insertString("\0\n", 2);
    insertCString(docOf(view)->filepath);
  }
}

void drawFrameCursor(frame_t *frame)
{
  drawCursorOrSelection(viewOf(frame));
}

void drawFrameDoc(frame_t *frame)
{
  drawString(&docOf(viewOf(frame))->contents);
}

widget_t *frame(int i)
{
  frame_t *frame = arrayElemAt(&st.frames, i);
  widget_t *textarea = wid(i, scrollY(frame->scrollY, over(draw(drawFrameDoc, frame), draw(drawFrameCursor, frame))));
  widget_t *status = over(draw(drawString, &frame->status), vspc(&st.font.lineSkip));
  widget_t *background = color(&frame->color, over(box(), hspc(&frame->width)));
  return over(vcatr(textarea, status), background);
}

void stInit(int argc, char **argv)
{
  argc--;
  argv++;
  memset(&st, 0, sizeof(state_t));

  if (SDL_Init(SDL_INIT_VIDEO) != 0) die(SDL_GetError());

  windowInit(&st.window, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);

  rendererInit(st.window.window);

  initFont(&st.font, INIT_FONT_FILE, INIT_FONT_SIZE);

  initMacros();


    arrayInit(&st.docs, sizeof(doc_t));
    arrayInit(&st.views, sizeof(view_t));
    arrayInit(&st.frames, sizeof(frame_t));
    arrayInit(&st.results, sizeof(int));

    keysymInit();

    for(int i = 0; i < NUM_BUILTIN_BUFFERS; ++i)
      {
        doc_t *doc = arrayPushUninit(&st.docs);
        docInit(doc, builtinBufferTitle[i]);
        viewInit(arrayPushUninit(&st.views), i);
      }

    for(int i = 0; i < argc; ++i) {
      doc_t *doc = arrayPushUninit(&st.docs);
      docInit(doc, argv[i]);
      docRead(doc);
      viewInit(arrayPushUninit(&st.views), NUM_BUILTIN_BUFFERS + i);
    }

    for(int i = 0; i < NUM_FRAMES; ++i)
    {
        frame_t *v = arrayPushUninit(&st.frames);
        frameInit(v);
    }

    printf("views: %d %d\n", NUM_BUILTIN_BUFFERS, argc);

    for(int i = 0; i < NUM_FRAMES; ++i)
      {
        setFrameView(i, NUM_BUILTIN_BUFFERS + argc - i - 1);
      }

    setFocusFrame(MAIN_FRAME);

    gui = hcat(frame(0), hcat(frame(1), frame(2)));

    stResize();

    // BAL: helpBufInit();
    // BAL: buffersBufInit();

    /* message("hello from the beyond\n"); */

    /* setFocusFrame(SECONDARY_FRAME); */
    /* stSetViewFocus(MESSAGE_BUF); */
    /* setFocusFrame(BUILTINS_FRAME); */
    /* stSetViewFocus(BUFFERS_BUF); */
    /* setFocusFrame(MAIN_FRAME); */
    /* stSetViewFocus(NUM_BUILTIN_BUFFERS); */

}

void quitEvent()
{
    for(int i = NUM_BUILTIN_BUFFERS; i < st.docs.numElems; ++i)
    {
        doc_t *doc = arrayElemAt(&st.docs, i);
        assert(doc);
        docWrite(doc);
    }
    TTF_Quit();
    SDL_Quit();
    exit(0);
}

void windowEvent()
{
    switch (st.event.window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            stResize();
            break;
        default:
            break;
    }
}

void selectChars()
{
  view_t *view = focusView();
  cursorCopy(&view->selection, &view->cursor);
  view->selectMode = CHAR_SELECT;
}

void selectLines()
{
  view_t *view = focusView();
  cursorCopy(&view->selection, &view->cursor);
  view->selectMode = LINE_SELECT;
}

void selectionCancel()
{
  view_t *view = focusView();
  view->selectMode = NO_SELECT;
}

void selectionSetRowCol()
{
  view_t *view = focusView();
  int column = xToColumn(st.event.button.x - st.downCxtX);
  cursorSetRowCol(&view->selection, yToRow(st.event.button.y - st.downCxtY), column, focusDoc());
}

void selectionEnd()
{
  view_t *view = focusView();
  view->selectMode = NO_SELECT;
}

void mouseButtonUpEvent()
{
  st.mouseSelectionInProgress = false;
  selectionSetRowCol();
  view_t *view = focusView();
  if (cursorEq(&view->cursor, &view->selection)) selectionEnd();
}

void mouseButtonDownEvent()
{
  contextReinit();
  widgetAt(gui, st.event.button.x, st.event.button.y);
  if (context.wid < 0 || context.wid > numFrames())
    {
      printf("%d\n", context.wid);
      return;
    }

  st.mouseSelectionInProgress = true;
  setFocusFrame(context.wid);

  view_t *view = focusView();
  view->selectMode = CHAR_SELECT;
  if (st.event.button.button == SDL_BUTTON_RIGHT) view->selectMode = LINE_SELECT;

  st.downCxtX = context.x;
  st.downCxtY = context.y;

  selectionSetRowCol();
  cursorCopy(&view->cursor, &view->selection);
}

void mouseMotionEvent()
{
    if (st.mouseSelectionInProgress)
    {
      selectionSetRowCol();
      return;
    }
}

int docHeight(doc_t *doc)
{
    return doc->numLines * st.font.lineSkip;
}

void focusScrollY(int dR)
{
  frame_t *frame = focusFrame();
  doc_t *doc = focusDoc();

  *frame->scrollY += dR * st.font.lineSkip;

  *frame->scrollY = clamp(frame->height - docHeight(doc) - st.font.lineSkip, *frame->scrollY, 0);
}

void mouseWheelEvent()
{
  focusScrollY(st.event.wheel.y);
}

void doKeyPress(uchar c)
{
    keyHandler[focusView()->mode][c](c);
}

void recordKeyPress(uchar c)
{
  int refView = getViewFocus();
  setFocusView(MACRO_BUF);
  insertChar(c);
  setFocusView(refView);
}

void keyDownEvent()
{
    SDL_Keycode sym = st.event.key.keysym.sym;
    if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) return;
    uchar c = getKeyChar(sym);
    bool wasRecording = st.isRecording;
    doKeyPress(c);
    if (wasRecording && st.isRecording)
    {
        recordKeyPress(c);
    }
}

cursor_t *cursorFocus()
{
  return &focusView()->cursor;
}

void stMoveCursorOffset(int offset)
{
    cursorSetOffset(cursorFocus(), offset, focusDoc());
    frameTrackSelection(focusFrame());
}

void stMoveCursorRowCol(int row, int col)
{
    cursorSetRowCol(cursorFocus(), row, col, focusDoc());
    frameTrackSelection(focusFrame());
}

void backwardChar()
{
  stMoveCursorOffset(cursorFocus()->offset - 1);
}

void forwardChar()
{
  stMoveCursorOffset(cursorFocus()->offset + 1);
}

void setNavigateMode()
{
    focusView()->mode = NAVIGATE_MODE;
}

void setInsertMode()
{
    focusView()->mode = INSERT_MODE;
}

void setNavigateModeAndDoKeyPress(uchar c)
{
    setNavigateMode();
    doKeyPress(c);
}

void moveLines(int dRow)
{
  cursor_t *cursor = cursorFocus();
    int col = cursor->preferredColumn;

    stMoveCursorRowCol(cursor->row + dRow, col);
    cursor->preferredColumn = col;
}

void backwardLine()
{
    moveLines(-1);
}

void forwardLine()
{
    moveLines(1);
}

void backwardPage()
{
    moveLines(-max(1, frameRows(focusFrame()) - AUTO_SCROLL_HEIGHT));
}

void forwardPage()
{
    moveLines(max(1, frameRows(focusFrame()) - AUTO_SCROLL_HEIGHT));
}

void backwardSOF()
{
    stMoveCursorOffset(0);
}

void forwardEOF()
{
    stMoveCursorOffset(INT_MAX - 1);
}

void backwardSOL()
{
    stMoveCursorRowCol(cursorFocus()->row, 0);
}

void forwardEOL()
{
    stMoveCursorRowCol(cursorFocus()->row, INT_MAX);
}

void insertString(char *s, uint len)
{
  assert(s);
  if(len <= 0) return;
  docInsert(focusDoc(), cursorFocus()->offset, s, len);
}

void insertCString(char *s)
{
    assert(s);
    insertString(s, (uint)strlen(s));
}

void insertChar(uchar c)
{
    insertString((char*)&c, 1);
    forwardChar();
}

void backwardStartOfElem()
{
  int n = offsetOfStartOfElem(focusDoc()->contents.start, focusView()->cursor.offset);
  stMoveCursorOffset(n);
}
int lastFocusView()
{
  return frameOf(st.lastFocusFrame)->refView;
}
void pasteBefore()
{
  if (focusFrame()->refView == COPY_BUF)
    {
      if (lastFocusView() == COPY_BUF) return;
      backwardStartOfElem();
      char *s = focusDoc()->contents.start + focusView()->cursor.offset;
      setClipboardText(s);
      setFocusFrame(st.lastFocusFrame);
    }

  char *s = getClipboardText();
  if (s) insertCString(s);
}

void copy(char *s, uint len)
{
  int refView = getViewFocus();
  if (refView == COPY_BUF) return;
  setFocusView(COPY_BUF);
  backwardSOF();
  insertString("\0\n", 2);
  backwardSOF();
  insertString(s, len);
  setClipboardText(focusDoc()->contents.start);
  setFocusView(refView);
}

void cut()
{
    // BAL: add to undo

  int column;
  int row;
  int offset;
  int length;

  int refView = getViewFocus();
  if (refView == COPY_BUF) return;

  view_t *view = focusView();

  getSelectionCoords(view, &column, &row, &offset, &length);

  selectionCancel();

  if (length <= 0) return;

  doc_t *doc = focusDoc();

  copy(doc->contents.start + offset, length);

  docDelete(doc, offset, length);

  cursorSetRowCol(&view->cursor, row, column, focusDoc());
}

void playMacroString(char *s, uint len)
{
    char *done = s + len;

    while (s < done)
    {
        doKeyPress(*s);
        s++;
    }
}

void playMacroCString(char *s)
{
    playMacroString(s, (uint)strlen(s));
}

void doNothing(uchar c)
{
    if (macro[c])
    {
        playMacroCString(macro[c]);
        return;
    }
    selectionEnd();
}

void playMacro()
{
/*     if (st.isRecording) return; */
/*     int refView = getViewFocus(); */
/*     setFocusView(MACRO_BUF); */
/*     backwardSOL(); */
/*     doc_t *doc = focusDoc(); */
/*     view_t *view = focusView(); */
/*     if (!doc->contents.start) */
/*       { */
/*           setFocusView(refView); */
/*           return; */
/*       } */
/*     char *s = doc->contents.start + view->cursor.offset; */
/*     setFocusView(refView); */

/*     playMacroCString(s); */
}

/* void setCursorToSearch() */
/* { */
/*     frame_t *frame = focusFrame(); */
/*     if (noActiveSearch(frame)) return; */
/*     view_t *view = focusView(); */
/*     doc_t *doc = focusDoc(); */
/*     uint *off = arrayFocus(&st.results); */
/*     cursorSetOffset(&view->cursor, *off, doc); */
/*     cursorSetOffset(&view->selection, *off + st.searchLen - 1, doc); */
/* } */

void forwardSearch()
{
/*     st.results.offset++; */
/*     st.results.offset %= st.results.numElems; */
/*     setCursorToSearch(); */
}

void backwardSearch()
{
/*     st.results.offset += st.results.numElems - 1; */
/*     st.results.offset %= st.results.numElems; */
/*     setCursorToSearch(); */
}

/* void setCursorToSearch(); */

/* void computeSearchResults() */
/* { */
/*     view_t *view = arrayElemAt(&st.views, st.searchRefView); */
/*     doc_t *doc = docOf(view); */

/*     int refView = getViewFocus(); */
/*     setFocusView(SEARCH_BUF); */

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
/*     arrayReinit(&st.results); */
/*     while ((p = strcasestr(p, needle))) */
/*     { */
/*         uint *off = arrayPushUninit(&st.results); */
/*         *off = (uint)(p - haystack); */
/*         p++; */
/*     } */

/*     // copy replace string to copy buffer // BAL: don't use copy buffer ... */
/*     if (replace) copy(replace, (uint)strlen(replace)); */
/* done: */
/*     free(needle); */
/* nofree: */
/*     setFocusView(refView); */
/* } */

void setSearchMode()
{
/*     if ((focusFrame()->refView) == SEARCH_BUF) */
/*     { */
/*       // popView(); */
/*       //  computeSearchResults(); */

/*       setFocusFrame(MAIN_FRAME); */

/*       //  stSetViewFocus(st.searchRefView); */
/*       //  setCursorToSearch(); */
/*         return; */
/*     } */

/*     st.searchRefView = focusFrame()->refView; */

/*     // insert null at the end of the doc */
/*     doc_t *doc = focusDoc(); */
/*     docInsert(doc, doc->contents.numElems, "", 1); */
/*     doc->contents.numElems--; */

/*     int refView = getViewFocus(); */
/*     setFocusView(SEARCH_BUF); */
/*     setInsertMode(); */
/*     backwardSOF(); */
/*     insertString("\0\n", 2); */
/*     backwardSOF(); */
/*     setFocusView(refView); */
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

void startStopRecording()
{
/*     st.isRecording ^= true; */
/*     int refView = getViewFocus(); */
/*     setFocusView(MACRO_BUF); */
/*     if (st.isRecording) */
/*     { */
/*         backwardSOF(); */
/*         insertString("\0\n", 2); */
/*     } */
/*     backwardSOF(); */
/*     setFocusView(refView); */

/*     // stopped recording */
/*     // if 1st line is empty delete it */
}

void frameTrackSelection(frame_t *frame)
{
    /* view_t *view = arrayElemAt(&st.views, frame->refView); */
    /* int scrollR = frame->scrollY / st.font.lineSkip; */
    /* int dR = view->selection.row + scrollR; */
    /* if (dR < AUTO_SCROLL_HEIGHT) */
    /* { */
    /*     frameScrollY(frame, AUTO_SCROLL_HEIGHT - dR); */
    /* } else { */
    /*     int lastRow = frameRows(frame) - AUTO_SCROLL_HEIGHT; */
    /*     if (dR > lastRow) { */
    /*         frameScrollY(frame, lastRow - dR); */
    /*     } */
    /* } */
}

void forwardFrame()
{
  setFocusFrame((getFocusFrame() + 1) % numFrames());
}

void backwardFrame()
{
  setFocusFrame((getFocusFrame() + numFrames() - 1) % numFrames());
}

void forwardView()
{
  setFocusView((getViewFocus() + 1) % numViews());
}

void backwardView()
{
  setFocusView((getViewFocus() + numViews() - 1) % numViews());
}

int main(int argc, char **argv)
{
    assert(sizeof(char) == 1);
    assert(sizeof(int) == 4);
    assert(sizeof(unsigned int) == 4);
    assert(sizeof(void*) == 8);

    stInit(argc, argv);
    stDraw();
    while (SDL_WaitEvent(&st.event))
    {
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
            default:
                break;
        }
        //        frameUpdate(focusFrame());
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
TODO:
CORE:
    command line args/config to set config items (e.g. demo mode)
    status bar that has filename, row/col, modified, etc.
    cursor per frame
    select/cut/copy/paste with mouse
    multiple files
    create new file
    input screen (for search, paste, load, tab completion, etc.)
    Hook Cut/Copy/Paste into system Cut/Copy/Paste
    replace
    select region with mouse
    support editor API
    Name macro
    continual compile (on exiting insert or delete).  highlight errors/warnings
    jump to next error
    tab completion
    help screen
    status bar per frame
    reload file when changed outside of editor
    sort lines
    check spelling
    jump to prev/next placeholders
    jump to prev/next change
    collapse/expand selection (code folding)
    reload file when changed outside of editor

VIS:
    Display line/column/offset info
    Display line number of every line
    Minimap
    syntax highlighting
    highlight nested parens
    resize nested parens
    buffer list screen
    display multiple characters as one (e.g. lambda)

MACRO:
    switch between frames
    comment/uncomment region
    indent/outdent region
    more move, delete, insert commands
    select all

DONE:
indent/outdent line
split screen
Record/play macro
Scroll
Search
Move cursor on mouse click
File automatically loads on startup
File automatically saves (on return)
File automatically checkpoints into git (on return)
Select region
Cut/Copy/Paste
Undo command
Redo command
Move cursor to the start/end of a line
Move cursor up/down
Insert characters
Delete characters
Enforce cursor boundaries
wait for events, don't poll
switch between insert and navigation mode
Display filename and mode in title bar
Redraw on window size change
Display cursor
Move cursor left/right
Display text with spaces and newlines
Display text
Display window
exit on quit
*/
