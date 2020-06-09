#include "Cursor.h"
#include "Doc.h"
#include "DynamicArray.h"
#include "Font.h"
#include "Keysym.h"
#include "Util.h"
#include "Search.h"

void insertCString(state_t *st, char *s);
void insertString(state_t *st, char *s, uint len);

void frameTrackSelection(frame_t *frame, state_t *st);
void frameScrollY(frame_t *frame, int dR, state_t *st);
void frameRender(frame_t *frame, state_t *st); // BAL: frameFocusRender
void stRender(state_t *st);

frame_t *stFrameFocus(state_t *st)
{
    return arrayFocus(&st->frames);
}

view_t *stViewFocus(state_t *st)
{
    return arrayElemAt(&st->views, stFrameFocus(st)->refView);
}

doc_t *stDocFocus(state_t *st)
{
    return arrayElemAt(&st->docs, stViewFocus(st)->refDoc);
}

int frameHeight(frame_t *frame)
{
    return frame->rect.h;
}

uint frameRows(frame_t *frame, state_t *st)
{
    return frameHeight(frame) / st->font.lineSkip;
}

void stSetFrameFocus(state_t *st, int i)
{
    int j = arrayFocusOffset(&st->frames);
    if (i != j)
    {
        arraySetFocus(&st->frames, i);
        st->dirty = WINDOW_DIRTY;
        printf("file:%s\n", stDocFocus(st)->filepath);
    }
}

void stSetViewFocus(state_t *st, uint i)
{
    i = clamp(0, i, st->views.numElems - 1);
    frame_t *frame = stFrameFocus(st);
    frame->refView = i;
    printf("file:%s\n", stDocFocus(st)->filepath);
}

void frameInit(frame_t *v, uint i)
{
    memset(v, 0, sizeof(frame_t));
    v->refView = i;
}

void viewInit(view_t *view, uint i)
{
    memset(view, 0, sizeof(view_t));
    view->refDoc = i;
}

void docRender(doc_t *doc, frame_t *frame, state_t *st)
{
    resetCharRect(&st->font, frame->scrollX, frame->scrollY);

    if(!doc->contents.start) return;

    for(char *p = doc->contents.start; p < (char*)(doc->contents.start) + doc->contents.numElems; p++)
    {
        renderAndAdvChar(&st->font, *p);
    }

    renderEOF(&st->font);
}

bool noActiveSearch(frame_t *frame, state_t *st)
{
    return
      frame->refView != st->searchRefView ||
      st->searchLen == 0 ||
      st->results.numElems == 0;
}

void searchRender(frame_t *frame, state_t *st)
{
    if (noActiveSearch(frame, st)) return;

    setDrawColor(SEARCH_COLOR);

    view_t *view = arrayElemAt(&st->views, frame->refView);
    doc_t *doc = arrayElemAt(&st->docs, view->refDoc);

    for(int i = 0; i < st->results.numElems; ++i)
    {
        cursor_t cursor;
        uint *off = arrayElemAt(&st->results, i);
        cursorSetOffset(&cursor, *off, doc); // BAL: don't start over every time ...
        int aRow = cursor.row;
        int aCol = cursor.column;
        cursorSetOffset(&cursor, *off + st->searchLen - 1, doc); // BAL: don't start over every time...
        int bRow = cursor.row;
        int bCol = cursor.column;
        selectionRender(aRow, aCol, bRow, bCol, frame, st);
    }
}

void viewRender(view_t *view, frame_t *frame, state_t *st)
{
    assert(view);

    if (!view->selectionInProgress && !view->selectionActive) {
      cursorRender(view, frame, st);
      return;
    }

    int aCol = view->cursor.column;
    int bCol = view->selection.column;
    int aRow = view->cursor.row;
    int bRow = view->selection.row;

    if ((bRow < aRow) || ((aRow == bRow) && (bCol < aCol)))
    {
        swap(int, aCol, bCol);
        swap(int, aRow, bRow);
    }

    if (view->selectionLines)
      {
        aCol = 0;
        bCol = frameColumns(frame, st);
      }
    setDrawColor(SELECTION_COLOR);
    selectionRender(aRow, aCol, bRow, bCol, frame, st);
}

void frameRender(frame_t *frame, state_t *st)
{
    setViewport(&frame->rect);

    SDL_Rect bkgd;
    bkgd.x = 0;
    bkgd.y = 0;
    bkgd.w = frame->rect.w;
    bkgd.h = frame->rect.h;

    color_t color = viewColors[frame == stFrameFocus(st)];
    setDrawColor(color);
    // fillRect(st->renderer, &bkgd); // background

    view_t *view = arrayElemAt(&st->views, frame->refView);
    searchRender(frame, st);
    viewRender(view, frame, st);
    docRender(arrayElemAt(&st->docs, view->refDoc), frame, st);
}

void stRender(state_t *st) // BAL: rename stFocusRender
{
    frameRender(stFrameFocus(st), st);
    SDL_RenderPresent(renderer);
}

typedef enum { BOX, HSPC, VSPC, TEXT, COLOR, FONT, SCROLL_Y, WID, OVER, HCAT, VCAT, HCATR, VCATR, NUM_WIDGET_TAGS } widgetTag_t;

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
    char **text;
    widget_t *child;
  } a;
  union {
    widget_t *child;
  } b;
};

widget_t *newWidget()
{
  return dieIfNull(malloc(sizeof(widget_t)));
}

widget_t *hspc(int *len)
{
  widget_t *p = newWidget();
  p->tag = HSPC;
  p->a.length = len;
  return p;
}

widget_t *vspc(int *len)
{
  widget_t *p = newWidget();
  p->tag = VSPC;
  p->a.length = len;
  return p;
}

widget_t *box(void)
{
  widget_t *p = newWidget();
  p->tag = BOX;
  return p;
}

widget_t *text(char **s)
{
  widget_t *p = newWidget();
  p->tag = TEXT;
  p->a.text = s;
  return p;
}

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
widget_t *node(widgetTag_t tag, widget_t *a, widget_t *b)
{
  widget_t *p = newWidget();
  p->tag = tag;
  p->a.child = a;
  p->b.child = b;
  return p;
}

#define over(a, b) node(OVER, a, b)
#define hcat(a, b) node(HCAT, a, b)
#define hcatr(a, b) node(HCATR, a, b)
#define vcat(a, b) node(VCAT, a, b)
#define vcatr(a, b) node(VCATR, a, b)

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
  printf("numberof lines%d\n", n);
  return n;
}
void drawCString(char *s)
{
  assert(s);
  char c;
  SDL_Rect rect;
  rect.x = 0; // context.x;
  rect.y = context.dy;
  rect.w = context.font->charSkip;
  rect.h = context.font->lineSkip;
  while((c = *s) != '\0')
    {
      switch(c) {
        case '\n':
          rect.x = 0; // context.x;
          rect.y += rect.h;
          break;
        case ' ':
          rect.x += rect.w;
          break;
        default:
          setTextureColorMod(context.font->charTexture[c], context.color);// BAL: could just do this once if using a texture map
          SDL_RenderCopy(renderer, context.font->charTexture[c], NULL, &rect);
          rect.x += rect.w;
        }
      s++;
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
  default: // TEXT, VSPC, BOX
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
    default: // TEXT, HSPC, BOX
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

widget_t *widgetAt(widget_t *widget)
{
  assert(context.w >= 0);
  assert(context.h >= 0);
  assert(context.x >= 0);
  assert(context.y >= 0);
  printf("at: %d %d\n", context.x, context.y);
  // assert(context.x < context.w);
  // assert(context.y < context.y);
  if (context.x >= context.w || context.y >= context.h) return widget;

  switch (widget->tag) {
  case HCAT: {
    int w = widgetWidth(widget->a.child);
    if (context.x < w) {
      context.w = w;
      return widgetAt(widget->a.child);
    }
    context.w = context.w - w;
    context.x -= w;
    return widgetAt(widget->b.child);
  }
  case HCATR: {
    int w = context.w - widgetWidth(widget->b.child);
    if (context.x < w) {
      context.w = w;
      return widgetAt(widget->a.child);
    }
    context.w = context.w - w;
    context.x -= w;
    return widgetAt(widget->b.child);
  }
  case VCAT: {
    int h = widgetHeight(widget->a.child);
    if (context.y < h) {
      context.h = h;
      return widgetAt(widget->a.child);
    }
    context.h = context.h - h;
    context.y -= h;
    return widgetAt(widget->b.child);
  }
  case VCATR: {
    int h = context.h - widgetHeight(widget->b.child);
    if (context.y < h) {
      context.h = h;
      return widgetAt(widget->a.child);
    }
    context.h = context.h - h;
    context.y -= h;
    return widgetAt(widget->b.child);
  }
  case OVER:
    return widgetAt(widget->a.child);
  case WID: // fall through
    context.wid = widget->a.wid;
    return widgetAt(widget->b.child);
  case COLOR: // fall through
  case FONT: // fall through
    return widgetAt(widget->b.child);
  case SCROLL_Y:
    return widgetAt(widget->b.child);
  default: // BOX, HSPC, VSPC, TEXT
    return widget;
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
  case TEXT:
    drawCString(*widget->a.text);
    return;
  case BOX:
    fillRect(context.w, context.h);
    return;
  default: // HSPC/VSPC
    return;
  }
}

state_t st;
int frameWidth0 = 0;
int frameWidth1 = 0;
int colBlack = 0x000000ff;
int colGreen = 0x00f000ff;
int colBlue = 0x0000f0ff;
int redcol = 0xff0000af;
char *str0 = "string #0 This is\nstring 0. done\n";
char *str1 = "string #1\ncheck\ning in\nok\ndone\n";
char *status0 = "status 0 is ok";
char *is = "status 1 is ";
char *red = "red";
char *alert = " alert.";
char *str2 = "something,\n something, \nstring two";
char *status2 = "status two";
widget_t *status1;
int dy0 = 0;
int dy1 = 0;
int dy2 = 0;
widget_t *frame0;
widget_t *frame1;
widget_t *frame2;
widget_t *gui;

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
  SDL_RenderPresent(renderer);
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

void stRenderFull(state_t *st)
{
    setDrawColor(BACKGROUND_COLOR);
    rendererClear();

    for(int i = 0; i < st->frames.numElems; ++i)
    {
        frameRender(arrayElemAt(&st->frames, i), st);
    }

    SDL_RenderPresent(renderer);
}

void stResize(void)
{
    int w;
    int h;

    SDL_GetWindowSize(st.window.window, &w, &h);
    st.window.width = w;
    st.window.height = h;

    frameWidth0 = st.window.width / 3;
    frameWidth1 = st.window.width / 3;

    /* w = (w - BORDER_WIDTH) / 3; */
    /* h = h - 2*BORDER_WIDTH; */

    /* frame_t *v0 = arrayElemAt(&st->frames, SECONDARY_FRAME); */
    /* v0->rect.x = BORDER_WIDTH; */
    /* v0->rect.y = BORDER_WIDTH; */
    /* v0->rect.w = w - BORDER_WIDTH; */
    /* v0->rect.h = h; */

    /* frame_t *v1 = arrayElemAt(&st->frames, MAIN_FRAME); */
    /* v1->rect.x = BORDER_WIDTH + v0->rect.x + v0->rect.w; */
    /* v1->rect.y = BORDER_WIDTH; */
    /* v1->rect.w = w + w / 2 - BORDER_WIDTH; */
    /* v1->rect.h = h; */

    /* frame_t *v2 = arrayElemAt(&st->frames, BUILTINS_FRAME); */
    /* v2->rect.x = BORDER_WIDTH + v1->rect.x + v1->rect.w; */
    /* v2->rect.y = BORDER_WIDTH; */
    /* v2->rect.w = st->window.width - v2->rect.x - BORDER_WIDTH; */
    /* v2->rect.h = h; */

    st.dirty = WINDOW_DIRTY;
}

void builtinsPushFocus(state_t *st, int i)
{
  st->pushedFocus = arrayFocusOffset(&st->frames);
  stSetFrameFocus(st, BUILTINS_FRAME);
  stSetViewFocus(st, i);
}

void builtinsPopFocus(state_t *st)
{
    stSetFrameFocus(st, st->pushedFocus);
}

void message(state_t *st, char *s)
{
    builtinsPushFocus(st, MESSAGE_BUF);
    insertCString(st, s);
    builtinsPopFocus(st);
}

void buffersBufInit(state_t *st)
{
    for(int i = 0; i < st->docs.numElems; ++i)
    {
      doc_t *doc = arrayElemAt(&st->docs, i);
      builtinsPushFocus(st, BUFFERS_BUF);
      forwardEOF(st);
      insertString(st, "\0\n", 2);
      insertCString(st, doc->filepath);
    }
}

widget_t *frame(color_t *col, widget_t *a, widget_t *b)
{
  return over(vcatr(a, b), color(col, box()));
}

void stInit(state_t *st, int argc, char **argv)
{
  argc--;
  argv++;
  memset(st, 0, sizeof(state_t));
  //  initMacros();


    /* arrayInit(&st->docs, sizeof(doc_t)); */
    /* arrayInit(&st->views, sizeof(view_t)); */
    /* arrayInit(&st->frames, sizeof(frame_t)); */
    /* arrayInit(&st->results, sizeof(uint)); */

    if (SDL_Init(SDL_INIT_VIDEO) != 0) die(SDL_GetError());

    windowInit(&st->window, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);
    renderer = dieIfNull(SDL_CreateRenderer(st->window.window, -1, SDL_RENDERER_SOFTWARE));

    initFont(&st->font, INIT_FONT_FILE, INIT_FONT_SIZE);

    status1 = hcat(text(&is), hcat(color(&redcol, text(&red)), text(&alert)));

    //frame0 =
    //  over(frame(&col0, scrollY(&dy0, wid(0, over(text(&str0), hspc(&framwWidth0))), text(&status0));
    // frame1 = over(frame(&col1, scrollY(&dy1, wid(1, text(&str1))), status1), hspc(&frameWidth1));
    // frame2 = frame(&col2, scrollY(&dy2, wid(2, text(&str2))), text(&status2));
    // frame0 = over(vcatr(text(&str0), over(text(&status0), vspc(&st.font.lineSkip))), color(&colGreen, over(box(), hspc(&frameWidth0))));
    // frame1 = color(&colBlack, over(box(), hspc(&frameWidth1)));
    // frame2 = color(&colBlue, box());
    // gui = hcat(frame0, hcat(frame1, frame2));
    frame0 = over(vcatr(wid(0, text(&str0)), over(text(&status0), vspc(&st->font.lineSkip))), color(&colBlack, over(box(), hspc(&frameWidth0))));
    frame1 = over(vcatr(wid(1, text(&str1)), over(text(&status2), vspc(&st->font.lineSkip))), color(&colBlue, over(box(), hspc(&frameWidth1))));
    frame2 = over(vcatr(wid(2, text(&str2)), over(text(&status2), vspc(&st->font.lineSkip))), color(&colGreen, over(box(), hspc(&frameWidth1))));
    gui = hcat(frame0, hcat(frame1, frame2));
    stResize();
    //      hcat(frame0, hcat(frame1, frame2));
    // over(box(), over(hspc(&frameWidth0), vspc(&framwWidth0)));

    /* widget_t *a = wid(42, over(text(&str0), color(&col1, box()))); */
    /* widget_t *b = wid(17, over(text(&str1), color(&col2, box()))); */
    /* gui = vcat(a,b); */

    /* keysymInit(); */

    /* for(int i = 0; i < NUM_BUILTIN_BUFFERS; ++i) */
    /* { */
    /*     doc_t *doc = arrayPushUninit(&st->docs); */
    /*     docInit(doc, builtinBufferTitle[i]); */
    /*     viewInit(arrayPushUninit(&st->views), i); */
    /* } */

    /* for(int i = 0; i < argc; ++i) { */
    /*     doc_t *doc = arrayPushUninit(&st->docs); */
    /*     docInit(doc, argv[i]); */
    /*     docRead(doc); */
    /*     viewInit(arrayPushUninit(&st->views), NUM_BUILTIN_BUFFERS + i); */
    /* } */

    /* for(int i = 0; i < NUM_FRAMES; ++i) */
    /* { */
    /*     frame_t *v = arrayPushUninit(&st->frames); */
    /*     frameInit(v, i); */
    /* } */

    /* buffersBufInit(st); */

    /* message(st, "hello from the beyond\n"); */

    /* stSetFrameFocus(st, SECONDARY_FRAME); */
    /* stSetViewFocus(st, MESSAGE_BUF); */
    /* stSetFrameFocus(st, BUILTINS_FRAME); */
    /* stSetViewFocus(st, BUFFERS_BUF); */
    /* stSetFrameFocus(st, MAIN_FRAME); */
    /* stSetViewFocus(st, NUM_BUILTIN_BUFFERS); */

    //    stResize(st);
}

void quitEvent(state_t *st)
{
    for(int i = NUM_BUILTIN_BUFFERS; i < st->docs.numElems; ++i)
    {
        doc_t *doc = arrayElemAt(&st->docs, i);
assert(doc);
        docWrite(doc);
    }
    TTF_Quit();
    SDL_Quit();
    exit(0);
}

void windowEvent(state_t *st)
{
    switch (st->event.window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            stResize();
            break;
        default:
            st->dirty = NOT_DIRTY;
            break;
    }
}

void setCursorFromXYMotion(cursor_t *cursor, state_t  *st, int x, int y)
{
    frame_t *frame = stFrameFocus(st);
    view_t *view = stViewFocus(st);
    cursor->column =
      clamp(0, (x - frame->rect.x) / st->font.charSkip, frameColumns(frame, st));
    cursor->row = (y - frame->scrollY - frame->rect.y) / st->font.lineSkip;
    // BAL: don't do bounds checking until mouse button up for speed
    // when this is more efficient, remove
}

void setCursorFromXY(cursor_t *cursor, state_t *st, int x, int y)
{
  setCursorFromXYMotion(cursor, st, x, y);
  cursorSetRowCol(cursor, cursor->row, cursor->column, stDocFocus(st));
}

bool cursorEq(cursor_t *a, cursor_t *b)
{
  return (a->row == b->row && a->column == b->column);
}

void selectChars(state_t *st)
{
  view_t *view = stViewFocus(st);
  cursorCopy(&view->selection, &view->cursor);
  view->selectionInProgress = true;
  view->selectionActive = true;
  view->selectionLines = false;
}

void selectLines(state_t *st)
{
  view_t *view = stViewFocus(st);
  selectChars(st);
  view->selectionLines = true;
}

void selectionEnd(state_t *st)
{
  view_t *view = stViewFocus(st);
  view->selectionInProgress = false;
  view->selectionActive = !(cursorEq(&view->cursor, &view->selection));
  view->selectionLines = false;
}
void selectionCancel(state_t *st)
{
  view_t *view = stViewFocus(st);
  view->selectionInProgress = false;
  view->selectionActive = false;
  view->selectionLines = false;
}
void mouseButtonUpEvent(state_t *st)
{
  /* view_t *view = stViewFocus(st); */
  /* setCursorFromXY(&view->selection, st, st->event.button.x, st->event.button.y); */
  /*   selectionEnd(st); */
  st->mouseMoveInProgress = false;
  printf("up event %d dx=%d dy=%d\n", context.wid, st->event.button.x - st->downX, st->event.button.y - st->downY);
}

void mouseButtonDownEvent(state_t *st)
{
  contextReinit();
  context.x = st->event.button.x;
  context.y = st->event.button.y;
  widget_t *p = widgetAt(gui);
  st->downX = st->event.button.x; 
  st->downY = st->event.button.y;
  st->mouseMoveInProgress = true;
  printf("down event %d x=%d y=%d\n", context.wid, context.x, context.y);
    /* int i = 0; */
    /* frame_t *frame; */

    /* while (true) // find selected frame */
    /* { */
    /*     SDL_Point p; */
    /*     p.x = st->event.button.x; */
    /*     p.y = st->event.button.y; */
    /*     frame = arrayElemAt(&st->frames, i); */
    /*     if (SDL_PointInRect(&p, &frame->rect)) break; */
    /*     i++; */
    /*     if (i == NUM_FRAMES) return; */
    /* }; */

    /* stSetFrameFocus(st, i); */
    /* view_t *view = stViewFocus(st); */
    /* setCursorFromXY(&view->cursor, st, st->event.button.x, st->event.button.y); */
    /* selectChars(st); */
}

void mouseMotionEvent(state_t *st)
{
  //    view_t *view = stViewFocus(st);
    if (st->mouseMoveInProgress)
    {
      // setCursorFromXYMotion(&view->selection, st, st->event.motion.x, st->event.motion.y);
      printf("move event %d dx=%d dy=%d\n", context.wid, st->event.button.x - st->downX, st->event.button.y - st->downY);
        return;
    }
    // st->dirty = NOT_DIRTY;
}

int docHeight(doc_t *doc, state_t *st)
{
    return doc->numLines * st->font.lineSkip;
}

void frameScrollY(frame_t *frame, int dR, state_t *st)
{
    view_t *view = arrayElemAt(&st->views, frame->refView);
    doc_t *doc = arrayElemAt(&st->docs, view->refDoc);

    frame->scrollY += dR * st->font.lineSkip;

    frame->scrollY = clamp(frameHeight(frame) - docHeight(doc,st) - st->font.lineSkip, frame->scrollY, 0);
}

void mouseWheelEvent(state_t *st)
{
  dy0 += st->event.wheel.y;
  //    frameScrollY(stFrameFocus(st), st->event.wheel.y, st);
}

void doKeyPress(state_t *st, uchar c)
{
    keyHandler[stViewFocus(st)->mode][c](st, c);
}

void recordKeyPress(state_t *st, uchar c)
{
    builtinsPushFocus(st, MACRO_BUF);
    insertChar(st, c);
    builtinsPopFocus(st);
}

void keyDownEvent(state_t *st)
{
    SDL_Keycode sym = st->event.key.keysym.sym;
    if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) return;
    uchar c = getKeyChar(sym);
    bool wasRecording = st->isRecording;
    doKeyPress(st, c);
    if (wasRecording && st->isRecording)
    {
        recordKeyPress(st, c);
    }
}

cursor_t *activeCursor(state_t *st)
{
  view_t *view = stViewFocus(st);
  if (view->selectionInProgress || view->selectionActive)
    {
      return &view->selection;
    }
  return &view->cursor;
}

void stMoveCursorOffset(state_t *st, int offset)
{
    cursorSetOffset(activeCursor(st), offset, stDocFocus(st));
    frameTrackSelection(stFrameFocus(st), st);
}

void stMoveCursorRowCol(state_t *st, int row, int col)
{
    cursorSetRowCol(activeCursor(st), row, col, stDocFocus(st));
    frameTrackSelection(stFrameFocus(st), st);
}

void backwardChar(state_t *st)
{
  stMoveCursorOffset(st, activeCursor(st)->offset - 1);
}

void forwardChar(state_t *st)
{
  stMoveCursorOffset(st, activeCursor(st)->offset + 1);
}

void setNavigateMode(state_t *st)
{
    stViewFocus(st)->mode = NAVIGATE_MODE;
}

void setInsertMode(state_t *st)
{
    stViewFocus(st)->mode = INSERT_MODE;
}

void setNavigateModeAndDoKeyPress(state_t *st, uchar c)
{
    setNavigateMode(st);
    doKeyPress(st, c);
}

void moveLines(state_t *st, int dRow)
{
  cursor_t *cursor = activeCursor(st);
    int col = cursor->preferredColumn;

    stMoveCursorRowCol(st, cursor->row + dRow, col);
    cursor->preferredColumn = col;
}

void backwardLine(state_t *st)
{
    moveLines(st, -1);
}

void forwardLine(state_t *st)
{
    moveLines(st, 1);
}

void backwardPage(state_t *st)
{
    moveLines(st, -max(1, frameRows(stFrameFocus(st), st) - AUTO_SCROLL_HEIGHT));
}

void forwardPage(state_t *st)
{
    moveLines(st, max(1, frameRows(stFrameFocus(st), st) - AUTO_SCROLL_HEIGHT));
}

void backwardSOF(state_t *st)
{
    stMoveCursorOffset(st, 0);
}

void forwardEOF(state_t *st)
{
    stMoveCursorOffset(st, INT_MAX);
}

void backwardSOL(state_t *st)
{
    stMoveCursorRowCol(st, activeCursor(st)->row, 0);
}

void forwardEOL(state_t *st)
{
    stMoveCursorRowCol(st, activeCursor(st)->row, INT_MAX);
}

void insertString(state_t *st, char *s, uint len)
{
    if (!s || len == 0) return;
    st->dirty = DOC_DIRTY;
    docInsert(stDocFocus(st), activeCursor(st)->offset, s, len);
}

void insertCString(state_t *st, char *s)
{
    if (!s) return;
    insertString(st, s, (uint)strlen(s));
}

void insertChar(state_t *st, uchar c)
{
    insertString(st, (char*)&c, 1);
    forwardChar(st);
}

void pasteBefore(state_t *st)
{
    builtinsPushFocus(st, COPY_BUF);
    backwardSOL(st);
    doc_t *doc = stDocFocus(st);
    view_t *view = stViewFocus(st);
    char *s = doc->contents.start + view->cursor.offset;
    builtinsPopFocus(st);
    insertCString(st, s);
}

void getOffsetAndLength(state_t *st, int *offset, int *length)
{
    view_t *view = stViewFocus(st);
    cursor_t *cur = &view->cursor;
    cursor_t *sel = &view->selection;

    if (cur->offset > sel->offset)
    {
        swap(cursor_t *, cur, sel);
    }

    if (view->selectionLines)
      {
        cursorSetRowCol(cur, cur->row, 0, stDocFocus(st));
        cursorSetRowCol(sel, sel->row, INT_MAX, stDocFocus(st));
      }

    int off = cur->offset;
    int len = sel->offset - off + 1;

    uint n = stDocFocus(st)->contents.numElems;
    *offset = min(off, n);
    *length = min(len, n - off);
}

void copy(state_t *st, char *s, uint len)
{
    builtinsPushFocus(st, COPY_BUF);
    backwardSOF(st);
    insertString(st, "\0\n", 2);
    backwardSOF(st);
    insertString(st, s, len);
    builtinsPopFocus(st);
}

void delete(state_t *st)
{
    // BAL: add to undo

    int offset;
    int length;

    getOffsetAndLength(st, &offset, &length);
    selectionCancel(st);
    if (length <= 0) return;
    st->dirty = DOC_DIRTY;
    doc_t *doc = stDocFocus(st);

    if (doc->filepath[0] != '*')
      copy(st, doc->contents.start + offset, length); // BAL: this was causing problems with search...

    docDelete(doc, offset, length);
}

void playMacroString(state_t *st, char *s, uint len)
{
    char *done = s + len;

    while (s < done)
    {
        doKeyPress(st, *s);
        s++;
    }
}

void playMacroCString(state_t *st, char *s)
{
    playMacroString(st, s, (uint)strlen(s));
}

void doNothing(state_t *st, uchar c)
{
    if (macro[c])
    {
        playMacroCString(st, macro[c]);
        return;
    }
    st->dirty = NOT_DIRTY;
}

void playMacro(state_t *st)
{
    if (st->isRecording) return;
    builtinsPushFocus(st, MACRO_BUF);
    backwardSOL(st);
    doc_t *doc = stDocFocus(st);
    view_t *view = stViewFocus(st);
    if (!doc->contents.start)
      {
          builtinsPopFocus(st);
          return;
      }
    char *s = doc->contents.start + view->cursor.offset;
    builtinsPopFocus(st);

    playMacroCString(st, s);
}

void setCursorToSearch(state_t *st)
{
    frame_t *frame = stFrameFocus(st);
    if (noActiveSearch(frame, st)) return;
    view_t *view = stViewFocus(st);
    doc_t *doc = stDocFocus(st);
    uint *off = arrayFocus(&st->results);
    cursorSetOffset(&view->cursor, *off, doc);
    cursorSetOffset(&view->selection, *off + st->searchLen - 1, doc);
}

void forwardSearch(state_t *st)
{
    st->results.offset++;
    st->results.offset %= st->results.numElems;
    setCursorToSearch(st);
}

void backwardSearch(state_t *st)
{
    st->results.offset += st->results.numElems - 1;
    st->results.offset %= st->results.numElems;
    setCursorToSearch(st);
}

void setCursorToSearch(state_t *st);

void computeSearchResults(state_t *st)
{
    view_t *view = arrayElemAt(&st->views, st->searchRefView);
    doc_t *doc = arrayElemAt(&st->docs, view->refDoc);

    builtinsPushFocus(st, SEARCH_BUF);

    view_t *viewFind = stViewFocus(st);
    doc_t *find = stDocFocus(st);

    cursor_t cursorFind;
    cursorCopy(&cursorFind, &viewFind->cursor);
    cursorSetRowCol(&cursorFind, cursorFind.row, 0, find);

    char *search = find->contents.start + cursorFind.offset;
    if (!search) goto nofree;
    char *replace = dieIfNull(strdup(search));
    char *needle = strsep(&replace, "/");
    st->searchLen = (uint)strlen(needle);
    if (st->searchLen == 0) goto done;

    char *haystack = doc->contents.start;
    char *p = haystack;
    arrayReinit(&st->results);
    while ((p = strcasestr(p, needle)))
    {
        uint *off = arrayPushUninit(&st->results);
        *off = (uint)(p - haystack);
        p++;
    }

    // copy replace string to copy buffer // BAL: don't use copy buffer ...
    if (replace) copy(st, replace, (uint)strlen(replace));
done:
    free(needle);
nofree:
    builtinsPopFocus(st);
}

void setSearchMode(state_t *st)
{
    if ((stFrameFocus(st)->refView) == SEARCH_BUF)
    {
      // builtinsPopFocus(st);
      //  computeSearchResults(st);

      stSetFrameFocus(st, MAIN_FRAME);

      //  stSetViewFocus(st, st->searchRefView);
      //  setCursorToSearch(st);
        return;
    }

    st->searchRefView = stFrameFocus(st)->refView;

    // insert null at the end of the doc
    doc_t *doc = stDocFocus(st);
    docInsert(doc, doc->contents.numElems, "", 1);
    doc->contents.numElems--;

    builtinsPushFocus(st, SEARCH_BUF);
    setInsertMode(st);
    backwardSOF(st);
    insertString(st, "\0\n", 2);
    backwardSOF(st);
}

void gotoView(state_t *st)
{
    int frame = st->frames.offset;

    if (frame == BUILTINS_FRAME) frame = MAIN_FRAME;

    stSetFrameFocus(st, BUILTINS_FRAME);
    stSetViewFocus(st, BUFFERS_BUF);

    view_t *view = stViewFocus(st);

    stSetFrameFocus(st, frame);
    stSetViewFocus(st, view->cursor.row);
}

void startStopRecording(state_t *st)
{
    st->isRecording ^= true;
    builtinsPushFocus(st, MACRO_BUF);
    if (st->isRecording)
    {
        backwardSOF(st);
        insertString(st, "\0\n", 2);
    }
    backwardSOF(st);
    builtinsPopFocus(st);

    // stopped recording
    // if 1st line is empty delete it
}

void frameTrackSelection(frame_t *frame, state_t *st)
{
    view_t *view = arrayElemAt(&st->views, frame->refView);
    int scrollR = frame->scrollY / st->font.lineSkip;
    int dR = view->selection.row + scrollR;
    if (dR < AUTO_SCROLL_HEIGHT)
    {
        frameScrollY(frame, AUTO_SCROLL_HEIGHT - dR, st);
    } else {
        int lastRow = frameRows(frame, st) - AUTO_SCROLL_HEIGHT;
        if (dR > lastRow) {
            frameScrollY(frame, lastRow - dR, st);
        }
    }
}

int main(int argc, char **argv)
{
    assert(sizeof(char) == 1);
    assert(sizeof(int) == 4);
    assert(sizeof(unsigned int) == 4);
    assert(sizeof(void*) == 8);

    stInit(&st, argc, argv);

    while (SDL_WaitEvent(&st.event))
    {
      //  stRender(&st);

      stDraw();

        st.dirty = FOCUS_DIRTY;
        switch (st.event.type) {
            case SDL_WINDOWEVENT:
                windowEvent(&st);
                break;
            case SDL_MOUSEWHEEL:
                mouseWheelEvent(&st);
                break;
            case SDL_MOUSEBUTTONDOWN:
                mouseButtonDownEvent(&st);
                break;
            case SDL_MOUSEBUTTONUP:
                mouseButtonUpEvent(&st);
                break;
            case SDL_MOUSEMOTION:
                mouseMotionEvent(&st);
                break;
            /* case SDL_KEYDOWN: */
            /*     keyDownEvent(&st); */
            /*     break; */
            case SDL_QUIT:
                quitEvent(&st);
                break;
            default:
                st.dirty = NOT_DIRTY;
                break;
        }
        /* if (st.dirty & DOC_DIRTY) // BAL: noActiveSearch? */
        /* { */
        /*     computeSearchResults(&st); */
        /* } */
        /* if (st.dirty & WINDOW_DIRTY || st.dirty & DOC_DIRTY) */
        /* { */
        /*   stDraw(); */
        /*   stRenderFull(&st); */
        /* } else if (st.dirty & FOCUS_DIRTY || st.dirty & DOC_DIRTY) */
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

