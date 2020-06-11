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
void frameRender(frame_t *frame); // BAL: frameFocusRender
void stRender();

color_t unfocusedFrameColor = 0x303030ff;
color_t focusedFrameColor = 0x101010ff;

typedef enum { BOX, VIEW, HSPC, VSPC, TEXT, COLOR, FONT, SCROLL_Y, WID, OVER, HCAT, VCAT, HCATR, VCATR, NUM_WIDGET_TAGS } widgetTag_t;

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
    view_t **view;
    char **text;
    widget_t *child;
    void *data;
  } a;
  union {
    widget_t *child;
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

frame_t *stFrameFocus()
{
    return arrayFocus(&st.frames);
}

view_t *stViewFocus()
{
    return arrayElemAt(&st.views, stFrameFocus()->refView);
}

doc_t *stDocFocus()
{
    return arrayElemAt(&st.docs, stViewFocus()->refDoc);
}

int frameHeight(frame_t *frame)
{
    return frame->rect.h;
}

uint frameRows(frame_t *frame)
{
    return frameHeight(frame) / st.font.lineSkip;
}

void stSetFrameFocus(int i)
{
    int j = arrayFocusOffset(&st.frames);
    if (i != j)
    {
      frame_t *frame = stFrameFocus();
      frame->color = unfocusedFrameColor;
      arraySetFocus(&st.frames, i);
      frame = stFrameFocus();
      frame->color = focusedFrameColor;
        //        st.dirty = WINDOW_DIRTY;
        // printf("file:%s\n", stDocFocus()->filepath);
    }
}
frame_t *frameOf(int i)
{
  return arrayElemAt(&st.frames, i);
}
frame_t *frameFocus()
{
  return arrayFocus(&st.frames);
}
view_t *viewOf(frame_t *frame)
{
  return arrayElemAt(&st.views, frame->refView);
}
view_t *viewFocus()
{
  return viewOf(frameFocus());
}

doc_t *docOf(view_t *view)
{
  return arrayElemAt(&st.docs, view->refDoc);
}
doc_t *docFocus()
{
  return docOf(viewFocus());
}

void stSetFrameView(int i, int j)
{
  frame_t *frame = frameOf(i);
  frame->refView = j;

  view_t *view = viewOf(frame);
  frame->scrollY = &view->scrollY;
  frame->view = view;

  doc_t *doc = docOf(view);
  frame->text = docCString(doc);
  frame->filepath = &doc->filepath;
}

void stSetViewFocus(uint i)
{
    i = clamp(0, i, st.views.numElems - 1);
    frame_t *frame = stFrameFocus();
    frame->refView = i;
    printf("file:%s\n", stDocFocus()->filepath);
}

void frameInit(frame_t *v)
{
    memset(v, 0, sizeof(frame_t));
    v->color = unfocusedFrameColor;
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

void searchRender(frame_t *frame)
{
    /* if (noActiveSearch(frame)) return; */

    /* setDrawColor(SEARCH_COLOR); */

    /* view_t *view = arrayElemAt(&st.views, frame->refView); */
    /* doc_t *doc = arrayElemAt(&st.docs, view->refDoc); */

    /* for(int i = 0; i < st.results.numElems; ++i) */
    /* { */
    /*     cursor_t cursor; */
    /*     uint *off = arrayElemAt(&st.results, i); */
    /*     cursorSetOffset(&cursor, *off, doc); // BAL: don't start over every time ... */
    /*     int aRow = cursor.row; */
    /*     int aCol = cursor.column; */
    /*     cursorSetOffset(&cursor, *off + st.searchLen - 1, doc); // BAL: don't start over every time... */
    /*     int bRow = cursor.row; */
    /*     int bCol = cursor.column; */
    /*     selectionRender(aRow, aCol, bRow, bCol, frame); */
    /* } */
}

void viewRender(view_t *view, frame_t *frame)
{
    /* assert(view); */

    /* if (!view->selectionInProgress && !view->selectionActive) { */
    /*   cursorRender(view, frame); */
    /*   return; */
    /* } */

    /* int aCol = view->cursor.column; */
    /* int bCol = view->selection.column; */
    /* int aRow = view->cursor.row; */
    /* int bRow = view->selection.row; */

    /* if ((bRow < aRow) || ((aRow == bRow) && (bCol < aCol))) */
    /* { */
    /*     swap(int, aCol, bCol); */
    /*     swap(int, aRow, bRow); */
    /* } */

    /* if (view->selectionLines) */
    /*   { */
    /*     aCol = 0; */
    /*     bCol = frameColumns(frame); */
    /*   } */
    /* setDrawColor(SELECTION_COLOR); */
    /* selectionRender(aRow, aCol, bRow, bCol, frame); */
}

void frameRender(frame_t *frame)
{
    setViewport(&frame->rect);

    SDL_Rect bkgd;
    bkgd.x = 0;
    bkgd.y = 0;
    bkgd.w = frame->rect.w;
    bkgd.h = frame->rect.h;

    color_t color = viewColors[frame == stFrameFocus()];
    setDrawColor(color);
    // fillRect(st.renderer, &bkgd); // background

    view_t *view = arrayElemAt(&st.views, frame->refView);
    searchRender(frame);
    viewRender(view, frame);
    docRender(arrayElemAt(&st.docs, view->refDoc), frame);
}

void stRender() // BAL: rename stFocusRender
{
    frameRender(stFrameFocus());
    SDL_RenderPresent(renderer);
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
#define box() leaf(BOX, NULL)
#define view(v) leaf(VIEW, v)
#define text(s) leaf(TEXT, s)

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
  printf("number of lines%d\n", n);
  return n;
}

void drawCStringSelection(int x, int y, char *s, int offset, int len)
{
  assert(s);
  int w = context.font->charSkip;
  int h = context.font->lineSkip;
  s += offset;
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

void drawCString(char *s)
{
  assert(s);
  uchar c;
  SDL_Rect rect;
  rect.x = 0; // context.dx;
  rect.y = context.dy;
  rect.w = context.font->charSkip;
  rect.h = context.font->lineSkip;
  while((c = *s) != '\0')
    {
      switch(c) {
        case '\n':
          rect.x = 0; // context.dx;
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
  default: // TEXT, VSPC, BOX, VIEW
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
    default: // TEXT, HSPC, BOX, VIEW
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

void drawSelection(view_t *view)
{

  cursor_t *a = &view->cursor;
  cursor_t *b = &view->selection;
  if (a->offset > b->offset)
    {
      swap(cursor_t*, a, b);
    }

  setDrawColor(SELECTION_COLOR);
  drawCStringSelection(columnToX(a->column), rowToY(a->row), docCString(docFocus()), a->offset, b->offset - a->offset + 1);
  setDrawColor(context.color);
}

bool cursorEq(cursor_t *a, cursor_t *b)
{
  return (a->row == b->row && a->column == b->column);
}

bool selectionActive(view_t *view)
{
  return !(cursorEq(&view->cursor, &view->selection));
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

widget_t *widgetAt(widget_t *widget)
{
  assert(context.w >= 0);
  assert(context.h >= 0);
  assert(context.x >= 0);
  assert(context.y >= 0);
  //  printf("at: %d %d\n", context.x, context.y);
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
    context.y -= *widget->a.dy;
    return widgetAt(widget->b.child);
  default: // BOX, HSPC, VSPC, TEXT, VIEW
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
  case VIEW:
    drawCursorOrSelection(*widget->a.view);
    return;
  default: // HSPC/VSPC
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

void stRenderFull()
{
    setDrawColor(BACKGROUND_COLOR);
    rendererClear();

    for(int i = 0; i < st.frames.numElems; ++i)
    {
        frameRender(arrayElemAt(&st.frames, i));
    }

    SDL_RenderPresent(renderer);
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
    for(int i = 0; i < NUM_FRAMES; ++i)
      {
        frameResize(frameOf(i));
      }
    /* w = (w - BORDER_WIDTH) / 3; */
    /* h = h - 2*BORDER_WIDTH; */

    /* frame_t *v0 = arrayElemAt(&st.frames, SECONDARY_FRAME); */
    /* v0->rect.x = BORDER_WIDTH; */
    /* v0->rect.y = BORDER_WIDTH; */
    /* v0->rect.w = w - BORDER_WIDTH; */
    /* v0->rect.h = h; */

    /* frame_t *v1 = arrayElemAt(&st.frames, MAIN_FRAME); */
    /* v1->rect.x = BORDER_WIDTH + v0->rect.x + v0->rect.w; */
    /* v1->rect.y = BORDER_WIDTH; */
    /* v1->rect.w = w + w / 2 - BORDER_WIDTH; */
    /* v1->rect.h = h; */

    /* frame_t *v2 = arrayElemAt(&st.frames, BUILTINS_FRAME); */
    /* v2->rect.x = BORDER_WIDTH + v1->rect.x + v1->rect.w; */
    /* v2->rect.y = BORDER_WIDTH; */
    /* v2->rect.w = st.window.width - v2->rect.x - BORDER_WIDTH; */
    /* v2->rect.h = h; */

    st.dirty = WINDOW_DIRTY;
}

void builtinsPushFocus(int i)
{
  st.pushedFocus = arrayFocusOffset(&st.frames);
  stSetFrameFocus(BUILTINS_FRAME);
  stSetViewFocus(i);
}

void builtinsPopFocus()
{
    stSetFrameFocus(st.pushedFocus);
}

void message(char *s)
{
    builtinsPushFocus(MESSAGE_BUF);
    insertCString(s);
    builtinsPopFocus();
}

void buffersBufInit()
{
    for(int i = 0; i < st.docs.numElems; ++i)
    {
      doc_t *doc = arrayElemAt(&st.docs, i);
      builtinsPushFocus(BUFFERS_BUF);
      forwardEOF();
      insertString("\0\n", 2);
      insertCString(doc->filepath);
    }
}

widget_t *frame(frame_t *frame, int i)
{
  widget_t *textarea = wid(i, scrollY(frame->scrollY, over(text(&frame->text), view(&frame->view))));
  widget_t *status = over(text(frame->filepath), vspc(&st.font.lineSkip));
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
  renderer = dieIfNull(SDL_CreateRenderer(st.window.window, -1, SDL_RENDERER_SOFTWARE));

  initFont(&st.font, INIT_FONT_FILE, INIT_FONT_SIZE);

  //  initMacros();


    arrayInit(&st.docs, sizeof(doc_t));
    arrayInit(&st.views, sizeof(view_t));
    arrayInit(&st.frames, sizeof(frame_t));
    /* arrayInit(&st.results, sizeof(uint)); */

    /* keysymInit(); */

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
        stSetFrameView(i, NUM_BUILTIN_BUFFERS + argc - i - 1);
    }

    stSetFrameFocus(MAIN_FRAME);

    gui = hcat(frame(arrayElemAt(&st.frames, 0), 0),
               hcat(frame(arrayElemAt(&st.frames, 1), 1),
                    frame(arrayElemAt(&st.frames, 2), 2)));

    stResize();

    /* buffersBufInit(); */

    /* message("hello from the beyond\n"); */

    /* stSetFrameFocus(SECONDARY_FRAME); */
    /* stSetViewFocus(MESSAGE_BUF); */
    /* stSetFrameFocus(BUILTINS_FRAME); */
    /* stSetViewFocus(BUFFERS_BUF); */
    /* stSetFrameFocus(MAIN_FRAME); */
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
            st.dirty = NOT_DIRTY;
            break;
    }
}

void setCursorFromXYMotion(cursor_t *cursor, int x, int y)
{
    /* frame_t *frame = stFrameFocus(); */
    /* view_t *view = stViewFocus(); */
    /* cursor->column = */
    /*   clamp(0, (x - frame->rect.x) / st.font.charSkip, frameColumns(frame)); */
    /* cursor->row = (y - frame->scrollY - frame->rect.y) / st.font.lineSkip; */
    /* // BAL: don't do bounds checking until mouse button up for speed */
    /* // when this is more efficient, remove */
}

void setCursorFromXY(cursor_t *cursor, int x, int y)
{
  setCursorFromXYMotion(cursor, x, y);
  cursorSetRowCol(cursor, cursor->row, cursor->column, stDocFocus());
}

void selectChars()
{
  view_t *view = stViewFocus();
  cursorCopy(&view->selection, &view->cursor);
  view->selectionInProgress = true;
  view->selectionActive = true;
  view->selectionLines = false;
}

void selectLines()
{
  view_t *view = stViewFocus();
  selectChars();
  view->selectionLines = true;
}

void selectionEnd()
{
  view_t *view = stViewFocus();
  view->selectionInProgress = false;
  view->selectionActive = !(cursorEq(&view->cursor, &view->selection));
  view->selectionLines = false;
}
void selectionCancel()
{
  view_t *view = stViewFocus();
  view->selectionInProgress = false;
  view->selectionActive = false;
  view->selectionLines = false;
}

void selectionSetRowCol()
{
  view_t *view = stViewFocus();
  cursorSetRowCol(&view->selection, view->cursor.row + yToRow(st.event.button.y - st.downY), view->cursor.column + xToColumn(st.event.button.x - st.downX), docFocus());
}

void mouseButtonUpEvent()
{
  /* setCursorFromXY(&view->selection, st, st.event.button.x, st.event.button.y); */
  /*   selectionEnd(); */
  st.mouseMoveInProgress = false;
  selectionSetRowCol();
  // view->selectionActive = !(cursorEq(&view->cursor, &view->selection));
  // printf("up event %d dx=%d dy=%d\n", context.wid, st.event.button.x - st.downX, st.event.button.y - st.downY);
}
void mouseButtonDownEvent()
{
  contextReinit();
  context.x = st.event.button.x;
  context.y = st.event.button.y;
  widget_t *p = widgetAt(gui);
  if (context.wid < 0 || context.wid > NUM_FRAMES) return;
  stSetFrameFocus(context.wid);

  st.downX = st.event.button.x;
  st.downY = st.event.button.y;

  st.mouseMoveInProgress = true;

  view_t *view = viewFocus();
  cursorSetRowCol(&view->cursor, yToRow(context.y), xToColumn(context.x), docFocus());
  memcpy(&view->selection, &view->cursor, sizeof(cursor_t));
  // printf("down event %d x=%d y=%d\n", context.wid, context.x, context.y);
    /* int i = 0; */
    /* frame_t *frame; */

    /* while (true) // find selected frame */
    /* { */
    /*     SDL_Point p; */
    /*     p.x = st.event.button.x; */
    /*     p.y = st.event.button.y; */
    /*     frame = arrayElemAt(&st.frames, i); */
    /*     if (SDL_PointInRect(&p, &frame->rect)) break; */
    /*     i++; */
    /*     if (i == NUM_FRAMES) return; */
    /* }; */

    /* stSetFrameFocus(i); */
    /* view_t *view = stViewFocus(); */
    /* setCursorFromXY(&view->cursor, st, st.event.button.x, st.event.button.y); */
    /* selectChars(); */
}

void mouseMotionEvent()
{
    if (st.mouseMoveInProgress)
    {
      // setCursorFromXYMotion(&view->selection, st, st.event.motion.x, st.event.motion.y);
      selectionSetRowCol();
      return;
    }
    // st.dirty = NOT_DIRTY;
}

int docHeight(doc_t *doc)
{
    return doc->numLines * st.font.lineSkip;
}

void focusScrollY(int dR)
{
  frame_t *frame = frameFocus();
  doc_t *doc = docFocus();

  *frame->scrollY += dR * st.font.lineSkip;

  *frame->scrollY = clamp(frame->height - docHeight(doc) - st.font.lineSkip, *frame->scrollY, 0);
}

void mouseWheelEvent()
{
  focusScrollY(st.event.wheel.y);
}

void doKeyPress(uchar c)
{
    keyHandler[stViewFocus()->mode][c](c);
}

void recordKeyPress(uchar c)
{
    builtinsPushFocus(MACRO_BUF);
    insertChar(c);
    builtinsPopFocus();
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

cursor_t *activeCursor()
{
  view_t *view = stViewFocus();
  if (view->selectionInProgress || view->selectionActive)
    {
      return &view->selection;
    }
  return &view->cursor;
}

void stMoveCursorOffset(int offset)
{
    cursorSetOffset(activeCursor(), offset, stDocFocus());
    frameTrackSelection(stFrameFocus());
}

void stMoveCursorRowCol(int row, int col)
{
    cursorSetRowCol(activeCursor(), row, col, stDocFocus());
    frameTrackSelection(stFrameFocus());
}

void backwardChar()
{
  stMoveCursorOffset(activeCursor()->offset - 1);
}

void forwardChar()
{
  stMoveCursorOffset(activeCursor()->offset + 1);
}

void setNavigateMode()
{
    stViewFocus()->mode = NAVIGATE_MODE;
}

void setInsertMode()
{
    stViewFocus()->mode = INSERT_MODE;
}

void setNavigateModeAndDoKeyPress(uchar c)
{
    setNavigateMode();
    doKeyPress(c);
}

void moveLines(int dRow)
{
  cursor_t *cursor = activeCursor();
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
    moveLines(-max(1, frameRows(stFrameFocus()) - AUTO_SCROLL_HEIGHT));
}

void forwardPage()
{
    moveLines(max(1, frameRows(stFrameFocus()) - AUTO_SCROLL_HEIGHT));
}

void backwardSOF()
{
    stMoveCursorOffset(0);
}

void forwardEOF()
{
    stMoveCursorOffset(INT_MAX);
}

void backwardSOL()
{
    stMoveCursorRowCol(activeCursor()->row, 0);
}

void forwardEOL()
{
    stMoveCursorRowCol(activeCursor()->row, INT_MAX);
}

void insertString(char *s, uint len)
{
    if (!s || len == 0) return;
    st.dirty = DOC_DIRTY;
    docInsert(stDocFocus(), activeCursor()->offset, s, len);
}

void insertCString(char *s)
{
    if (!s) return;
    insertString(s, (uint)strlen(s));
}

void insertChar(uchar c)
{
    insertString((char*)&c, 1);
    forwardChar();
}

void pasteBefore()
{
    builtinsPushFocus(COPY_BUF);
    backwardSOL();
    doc_t *doc = stDocFocus();
    view_t *view = stViewFocus();
    char *s = doc->contents.start + view->cursor.offset;
    builtinsPopFocus();
    insertCString(s);
}

void getOffsetAndLength(int *offset, int *length)
{
    view_t *view = stViewFocus();
    cursor_t *cur = &view->cursor;
    cursor_t *sel = &view->selection;

    if (cur->offset > sel->offset)
    {
        swap(cursor_t *, cur, sel);
    }

    if (view->selectionLines)
      {
        cursorSetRowCol(cur, cur->row, 0, stDocFocus());
        cursorSetRowCol(sel, sel->row, INT_MAX, stDocFocus());
      }

    int off = cur->offset;
    int len = sel->offset - off + 1;

    uint n = stDocFocus()->contents.numElems;
    *offset = min(off, n);
    *length = min(len, n - off);
}

void copy(char *s, uint len)
{
    builtinsPushFocus(COPY_BUF);
    backwardSOF();
    insertString("\0\n", 2);
    backwardSOF();
    insertString(s, len);
    builtinsPopFocus();
}

void delete()
{
    // BAL: add to undo

    int offset;
    int length;

    getOffsetAndLength(&offset, &length);
    selectionCancel();
    if (length <= 0) return;
    st.dirty = DOC_DIRTY;
    doc_t *doc = stDocFocus();

    if (doc->filepath[0] != '*')
      copy(doc->contents.start + offset, length); // BAL: this was causing problems with search...

    docDelete(doc, offset, length);
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
    st.dirty = NOT_DIRTY;
}

void playMacro()
{
    if (st.isRecording) return;
    builtinsPushFocus(MACRO_BUF);
    backwardSOL();
    doc_t *doc = stDocFocus();
    view_t *view = stViewFocus();
    if (!doc->contents.start)
      {
          builtinsPopFocus();
          return;
      }
    char *s = doc->contents.start + view->cursor.offset;
    builtinsPopFocus();

    playMacroCString(s);
}

void setCursorToSearch()
{
    frame_t *frame = stFrameFocus();
    if (noActiveSearch(frame)) return;
    view_t *view = stViewFocus();
    doc_t *doc = stDocFocus();
    uint *off = arrayFocus(&st.results);
    cursorSetOffset(&view->cursor, *off, doc);
    cursorSetOffset(&view->selection, *off + st.searchLen - 1, doc);
}

void forwardSearch()
{
    st.results.offset++;
    st.results.offset %= st.results.numElems;
    setCursorToSearch();
}

void backwardSearch()
{
    st.results.offset += st.results.numElems - 1;
    st.results.offset %= st.results.numElems;
    setCursorToSearch();
}

void setCursorToSearch();

void computeSearchResults()
{
    view_t *view = arrayElemAt(&st.views, st.searchRefView);
    doc_t *doc = arrayElemAt(&st.docs, view->refDoc);

    builtinsPushFocus(SEARCH_BUF);

    view_t *viewFind = stViewFocus();
    doc_t *find = stDocFocus();

    cursor_t cursorFind;
    cursorCopy(&cursorFind, &viewFind->cursor);
    cursorSetRowCol(&cursorFind, cursorFind.row, 0, find);

    char *search = find->contents.start + cursorFind.offset;
    if (!search) goto nofree;
    char *replace = dieIfNull(strdup(search));
    char *needle = strsep(&replace, "/");
    st.searchLen = (uint)strlen(needle);
    if (st.searchLen == 0) goto done;

    char *haystack = doc->contents.start;
    char *p = haystack;
    arrayReinit(&st.results);
    while ((p = strcasestr(p, needle)))
    {
        uint *off = arrayPushUninit(&st.results);
        *off = (uint)(p - haystack);
        p++;
    }

    // copy replace string to copy buffer // BAL: don't use copy buffer ...
    if (replace) copy(replace, (uint)strlen(replace));
done:
    free(needle);
nofree:
    builtinsPopFocus();
}

void setSearchMode()
{
    if ((stFrameFocus()->refView) == SEARCH_BUF)
    {
      // builtinsPopFocus();
      //  computeSearchResults();

      stSetFrameFocus(MAIN_FRAME);

      //  stSetViewFocus(st.searchRefView);
      //  setCursorToSearch();
        return;
    }

    st.searchRefView = stFrameFocus()->refView;

    // insert null at the end of the doc
    doc_t *doc = stDocFocus();
    docInsert(doc, doc->contents.numElems, "", 1);
    doc->contents.numElems--;

    builtinsPushFocus(SEARCH_BUF);
    setInsertMode();
    backwardSOF();
    insertString("\0\n", 2);
    backwardSOF();
}

void gotoView()
{
    int frame = st.frames.offset;

    if (frame == BUILTINS_FRAME) frame = MAIN_FRAME;

    stSetFrameFocus(BUILTINS_FRAME);
    stSetViewFocus(BUFFERS_BUF);

    view_t *view = stViewFocus();

    stSetFrameFocus(frame);
    stSetViewFocus(view->cursor.row);
}

void startStopRecording()
{
    st.isRecording ^= true;
    builtinsPushFocus(MACRO_BUF);
    if (st.isRecording)
    {
        backwardSOF();
        insertString("\0\n", 2);
    }
    backwardSOF();
    builtinsPopFocus();

    // stopped recording
    // if 1st line is empty delete it
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
        st.dirty = FOCUS_DIRTY;
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
            /* case SDL_KEYDOWN: */
            /*     keyDownEvent(); */
            /*     break; */
            case SDL_QUIT:
                quitEvent();
                break;
            default:
                st.dirty = NOT_DIRTY;
                break;
        }
        stDraw();
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

