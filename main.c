//
//  main.c
//  ceditor
//
//  Created by Brett Letner on 4/23/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#include "Cursor.h"
#include "Doc.h"
#include "DynamicArray.h"
#include "Font.h"
#include "Keysym.h"
#include "Search.h"
#include "Util.h"
#include "Widget.h"
#include "Syntax.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void insertCString(char *s);
void insertString(char *s, uint len);
void builtinInsertCString(char *s);
void builtinInsertString(char *s, uint len);
void builtinInsertChar(uchar c);
void directoryBufInit();
void docLoad(char *filepath);
bool isFile(char *filename);
bool isDirectory(char *filename);
void recomputeSearch();
bool searchActive(); // BAL: remove?
int searchFrameRef = 0;
bool isSearchFocus();

state_t st;
widget_t *gui;

int xToColumn(int x) { return x / st.font.charSkip; }
int yToRow(int x) { return x / st.font.lineSkip; }

int numFrames() { return st.frames.numElems; }

int numDocs() { return st.docs.numElems; }

uint frameRows(frame_t *frame) { return frame->height / st.font.lineSkip; }

int focusFrameRef() { return st.frames.offset; }

frame_t *focusFrame() { return arrayFocus(&st.frames); }

frame_t *frameOf(int frameRef) { return arrayElemAt(&st.frames, frameRef); }

int focusViewRef() { return focusFrame()->views.offset; }

view_t *viewOf(frame_t *frame) { return arrayFocus(&frame->views); }

view_t *focusView() {
  frame_t *frame = focusFrame();
  assert(frame);
  return viewOf(frame);
}

int numViews() { return focusFrame()->views.numElems; }

doc_t *docOf(view_t *view) { return arrayElemAt(&st.docs, view->refDoc); }

int focusDocRef() { return focusView()->refDoc; }

doc_t *focusDoc() { return docOf(focusView()); }

cursor_t *focusCursor() { return &focusView()->cursor; }

cursor_t *focusSelection() { return &focusView()->selection; }

bool selectionActive(view_t *view) { return (view->selectMode != NO_SELECT); }

int searchDocRef = 0;

void setupSearchFocus()
{
  searchDocRef = focusDocRef();
  searchFrameRef = focusFrameRef();
}

void frameResize(int frameRef) {
  frame_t *frame = frameOf(frameRef);
  int focusRef = focusFrameRef();
  int w = st.window.width / 3;

  if (frameRef == focusRef && frameRef != BUILTINS_FRAME)
    w += w / 2;
  else if (frameRef != focusRef && frameRef == BUILTINS_FRAME)
    w /= 2;

  frame->width = w;

  int h = st.window.height - st.font.lineSkip;

  frame->height = h;
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

void setFrameView(int frameRef, int refView) {
  frame_t *frame = frameOf(frameRef);
  assert(frame);
  assert(refView < frame->views.numElems);

  arraySetFocus(&frame->views, refView);
}

void setFocusView(int refView) {
  setFrameView(focusFrameRef(), refView);
}

void setFocusBuiltinsView(int ref) {
  setFocusFrame(BUILTINS_FRAME);
  setFocusView(ref);
}

void frameInit(frame_t *frame) {
  myMemset(frame, 0, sizeof(frame_t));
  frame->color = FRAME_COLOR;

  arrayInit(&frame->views, sizeof(view_t));
}

void viewInit(view_t *view, uint refDoc) {
  assert(view);
  myMemset(view, 0, sizeof(view_t));
  view->refDoc = refDoc;
}

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

// BAL: not needed??
// bool searchActive() {
// return true;
  //return searchIsActive;
  // return (focusFrameRef() == BUILTINS_FRAME && focusViewRef() == SEARCH_BUF);
// }

bool isSearchDocRef(int docRef)
{
  return docRef == searchDocRef;
}

void drawCursorOrSelection(int frameRef) {
  frame_t *frame = frameOf(frameRef);
  view_t *view = viewOf(frame);
  if (isSearchDocRef(view->refDoc)) {
    drawSearch(view);
    }
  if (selectionActive(view)) {
    drawSelection(view);
    return;
  }
  drawCursor(view);
}

void stDraw(void) {
  setDrawColor(WHITE);
  rendererClear();
  contextReinit(&st.font, st.window.width, st.window.height);
  widgetDraw(gui);
  rendererPresent();
}

void windowInit(window_t *win, int width, int height) {
  myMemset(win, 0, sizeof(window_t));
  win->window = dieIfNull(
      SDL_CreateWindow("Editor", 0, 0, width, height,
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));

  win->width = width;
  win->height = height;
}

void stResize(void) {
  int w;
  int h;

  SDL_GetWindowSize(st.window.window, &w, &h);
  st.window.width = w;
  st.window.height = h;
  for (int i = 0; i < numFrames(); ++i) {
    frameResize(i);
  }
}

void insertNewElem() {
  backwardSOF();
  builtinInsertString("\0\n", 2);
}

void drawFrameCursor(int frameRef) { drawCursorOrSelection(frameRef); }

void drawFrameDoc(int frameRef) {
  drawString(&docOf(viewOf(frameOf(frameRef)))->contents);
}

int frameScrollY(int frameRef) {
  view_t *view = viewOf(frameOf(frameRef));
  return view->scrollY;
}

void drawFrameStatus(int frameRef) {
  frame_t *frame = frameOf(frameRef);
  view_t *view = viewOf(frame);
  doc_t *doc = docOf(view);
  char buf[1024];
  size_t n = sizeof(buf) - 1;
  buf[n] = '\0';
  snprintf(buf, n, "<%s> %3d:%2d %s", editorModeDescr[view->mode], view->cursor.row + 1, view->cursor.column, cstringOf(&doc->filepath));
  drawCString(buf, strlen(buf));
}

int scrollBarColor = 0x000000ff;
int scrollBarHeight = 5;
int scrollBarWidth = 5;

widget_t *frameWidget(int frameRef) {
  frame_t *frame = frameOf(frameRef); // BAL: remove
  widget_t *textarea = scrollY(frameScrollY, frameRef,
                                      over(draw(drawFrameDoc, frameRef),
                                           draw(drawFrameCursor, frameRef)));
  widget_t *status =
    over(draw(drawFrameStatus, frameRef), vspc(&st.font.lineSkip));
  widget_t *background = color(&frame->color, over(box(), hspc(&frame->width)));

  widget_t *hScrollBar = color(&scrollBarColor, over(box(), vspc(&scrollBarHeight)));
  widget_t *vScrollBar = color(&scrollBarColor, over(box(), hspc(&scrollBarWidth)));
  return wid(frameRef, over(vcatr(hcatr(vcatr(textarea, hScrollBar), vScrollBar), status), background));
}

void message(char *s) {
  int frameRef = focusFrameRef();
  setFocusBuiltinsView(MESSAGE_BUF);
  insertNewElem();
  builtinInsertCString(s);
  setFocusFrame(frameRef);
}

void helpBufInit(void);

void pushViewInit(int frameRef, int docRef)
{
  frame_t *frame = frameOf(frameRef);
  assert(frame);
  view_t *view = arrayPushUninit(&frame->views);
  assert(view);
  viewInit(view, docRef);
}

void builtinAppendCString(char *s) {
  forwardEOF();
  builtinInsertCString(s);
}

void buffersBufInit() {
  setFocusBuiltinsView(BUFFERS_BUF);
  doc_t *doc = focusDoc();
  arrayReinit(&doc->contents);
  doc_t *buffer = arrayElemAt(&st.docs, 0);
  builtinAppendCString(cstringOf(&buffer->filepath));
  for(int i = 1; i < st.docs.numElems; ++i)
    {
      builtinAppendCString("\n");
      buffer = arrayElemAt(&st.docs, i);
      builtinAppendCString(cstringOf(&buffer->filepath));
    }
  cstringOf(&focusDoc()->contents);
}

void docLoad(char *filepath)
{
  // BAL: if filepath already loaded, do nothing
  int i = st.docs.numElems;
  doc_t *doc = arrayPushUninit(&st.docs);
  docInit(doc, filepath, true, false);
  docRead(doc);
  pushViewInit(MAIN_FRAME, i);
  pushViewInit(SECONDARY_FRAME, i);
}

void stInit(int argc, char **argv) {
  argc--;
  argv++;
  if (argc == 0) {
    die("no input files\n");
  }

  myMemset(&st, 0, sizeof(state_t));
  arrayInit(&st.docs, sizeof(doc_t));
  arrayInit(&st.frames, sizeof(frame_t));
  arrayInit(&st.replace, sizeof(char));

  for (int i = 0; i < NUM_FRAMES; ++i) {
    frame_t *frame = arrayPushUninit(&st.frames);
    frameInit(frame);
  }

  for (int i = 0; i < NUM_BUILTIN_BUFFERS; ++i) {
    doc_t *doc = arrayPushUninit(&st.docs);
    docInit(doc, builtinBufferTitle[i], false, builtinBufferReadOnly[i]);
  }

  for (int i = 0; i < argc; ++i) {
    docLoad(argv[i]);
  }

  for(int i = 0; i < NUM_BUILTIN_BUFFERS; ++i)
    {
      pushViewInit(BUILTINS_FRAME, i);
    }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    die(SDL_GetError());

  windowInit(&st.window, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);

  rendererInit(st.window.window);

  initFont(&st.font, INIT_FONT_FILE, INIT_FONT_SIZE);

  gui = hcat(frameWidget(SECONDARY_FRAME), hcat(frameWidget(MAIN_FRAME), frameWidget(BUILTINS_FRAME)));

  stResize();

  keysymInit();
  macrosInit();
  helpBufInit();
  buffersBufInit();
  directoryBufInit();

  setFocusBuiltinsView(HELP_BUF);
  setFocusFrame(MAIN_FRAME);

}

void saveAll() {
  if (DEMO_MODE)
    return;
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

void cancelSelection() {
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
  assert(st.font.lineSkip > 0);
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
  contextReinit(&st.font, st.window.width, st.window.height);
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
    // BAL: fix it so that window will scroll when selecting
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
  int frameRef = focusFrameRef();
  if (frameRef == BUILTINS_FRAME)
    return;
  setFocusBuiltinsView(MACRO_BUF);
  builtinInsertChar(c);
  setFocusFrame(frameRef);
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

void doSearch(char *search) {
  assert(search);
  frame_t *frame = frameOf(searchFrameRef);
  assert(frame);
  view_t *view = viewOf(frame);
  doc_t *doc = docOf(view);
  cursor_t *cursor = &view->cursor;

  char *haystack = cstringOf(&doc->contents);
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

view_t *builtinsViewOf(int viewRef) {
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

  doSearch(search);

}

void updateBuiltinsState(bool isModify) {
  if (isModify) {
    resetSearch();
  }

  if (isSearchFocus()) {
    recomputeSearch();
    return;
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

void forwardEOF() {
  stMoveCursorOffset(INT_MAX);
}

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

void enter() {
  if (focusFrameRef() == BUILTINS_FRAME && focusViewRef() == BUFFERS_BUF)
    {
      int i = focusCursor()->row;
      if (i < NUM_BUILTIN_BUFFERS)
        {
          setFocusView(i);
          return;
        }
      setFocusFrame(MAIN_FRAME);
      setFocusView(i - NUM_BUILTIN_BUFFERS);
      return;
    }

  if (focusFrameRef() == BUILTINS_FRAME && focusViewRef() == DIRECTORY_BUF)
    {
      backwardSOL();
      char *s = focusDoc()->contents.start + focusCursor()->offset;
      int n = distanceToEOL(s);
      char filename[PATH_MAX + 1];
      if (filename != strncpy(filename, s, n)) die("unrecognized filename");
      filename[n] = '\0';
      if (isDirectory(filename))
        {
          chdir(filename);
          directoryBufInit();
          return;
        }
      if (isFile(filename))
        {
          docLoad(filename);
          buffersBufInit();
          setFocusBuiltinsView(DIRECTORY_BUF);
          setFocusFrame(MAIN_FRAME);
          setFocusView(focusFrame()->views.numElems - 1);
          return;
        }
      // otherwise
      message(filename);
      message("not a directory or regular file");
      return;
    }

  setInsertMode();
  insertChar('\n');
}

void insertNewline() {
  /* if (focusFrameRef() == BUILTINS_FRAME && focusViewRef() == SEARCH_BUF) // BAL: SEARCH remove? */
  /*   { */
  /*     setFocusFrame(MAIN_FRAME); */
  /*     setNavigateMode(); */
  /*     forwardSearch(); */
  /*     return; */
  /*   } */
  insertChar('\n');
}

void builtinInsertChar(uchar c) {
  builtinInsertString((char *)&c, 1);
  forwardChar();
}

void helpAppendKeysym(uchar c, char *s) {
  #define HELP_BUF_SIZE 1024
  char buf[HELP_BUF_SIZE];
  snprintf(buf, sizeof(buf), "'%s': %s\n", keysymName(c), s);
  builtinAppendCString(buf);
}

int fileMode(char *filename)
{
  struct stat fnst;
  if (stat(filename, &fnst) != 0)
    {
      return -1;
    }
  return fnst.st_mode;
}

bool isDirectory(char *filename)
{
  return S_ISDIR(fileMode(filename));
}

bool isFile(char *filename)
{
  return S_ISREG(fileMode(filename));
}

void appendDirBufEntry(char *fn) {
  builtinAppendCString(fn);
  if (isDirectory(fn)) builtinAppendCString("/");
}

int direntSort(const struct dirent **d1, const struct dirent **d2)
{
  return strcasecmp((*d1)->d_name, (*d2)->d_name);
}

void directoryBufInit() {
  char dir[PATH_MAX + 1];
  dieIfNull(getwd(dir));
  struct dirent **namelist;

  int n = scandir(dir, &namelist, NULL, direntSort);

  if (n < 0) die("unable to scan directory");

  setFocusBuiltinsView(DIRECTORY_BUF);
  doc_t *doc = focusDoc();
  arrayReinit(&doc->contents);
  builtinAppendCString(dir);

  int i = 0;
  while(i < n)
    {
      builtinAppendCString("\n");
      appendDirBufEntry(namelist[i]->d_name);
      free(namelist[i]);
      i++;
    }
  free(namelist);
  cstringOf(&focusDoc()->contents);
  backwardSOF();
}

void helpBufInit() {
  setFocusBuiltinsView(HELP_BUF);
  builtinAppendCString("Builtin Keys:\n");
  for (int i = 0; i < NUM_MODES; ++i) {
    if (i == NAVIGATE_MODE) builtinAppendCString("Navigate Mode:\n");
    if (i == INSERT_MODE) builtinAppendCString("Insert Mode:\n");
    for (int c = 0; c < NUM_KEYS; ++c) {
      if (!keyHandlerHelp[i][c])
        continue;
      helpAppendKeysym(c, keyHandlerHelp[i][c]);
    }
  }

  builtinAppendCString("Macros:\n");
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
  if (len <= 0 || doc->isReadOnly)
    return;
  docPushCommand(INSERT, doc, offset, s, len);
  docInsert(doc, offset, s, len);
  updateBuiltinsState(true);
}

void docDoCommand(doc_t *doc, commandTag_t tag, int offset, string_t *string) {
  cancelSelection();
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
  int frameRef = focusFrameRef();
  setFocusBuiltinsView(COPY_BUF);
  insertNewElem();
  builtinInsertString(s, len);
  setClipboardText(focusDoc()->contents.start);
  setFocusFrame(frameRef);
}

void cut() {
  int column;
  int row;
  int offset;
  int length;

  view_t *view = focusView();
  doc_t *doc = focusDoc();

  getSelectionCoords(view, &column, &row, &offset, &length);

  cancelSelection();

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
  int frameRef = focusFrameRef();
  st.isRecording = false;
  setFocusBuiltinsView(MACRO_BUF);
  backwardStartOfElem();
  setFocusFrame(frameRef);
}

void startOrStopRecording() {
  int frameRef = focusFrameRef();

  if (st.isRecording || frameRef == BUILTINS_FRAME) {
    stopRecording();
    return;
  }

  // start recording
  st.isRecording = true;
  setFocusBuiltinsView(MACRO_BUF);
  insertNewElem();
  // BAL: if 1st line in macro buf is empty delete it (or reuse it)
  setFocusFrame(frameRef);
}

void stopRecordingOrPlayMacro() {
  int frameRef = focusFrameRef();

  if (st.isRecording || frameRef == BUILTINS_FRAME ||
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
    setFocusFrame(frameRef);
    return;
  }

  setFocusFrame(frameRef);
  playMacroCString(doc->contents.start + view->cursor.offset);
}

void forwardSearch() {
  if (isSearchFocus()) {
    setFocusFrame(searchFrameRef);
  }

  setupSearchFocus();

  int docRef = focusDocRef();
  doc_t *doc = focusDoc();
  searchBuffer_t *results = &doc->searchResults;

  recomputeSearch(); // BAL: do only when necessary

  if (results->numElems == 0) return;

  cursor_t *cursor = focusCursor();

  for (int i = 0; i < results->numElems; ++i) {
    int *offset = arrayElemAt(results, i);
    if (*offset > cursor->offset) {
      stMoveCursorOffset(*offset);
      return;
    }
  }
  int *offset = arrayElemAt(results, 0);
  stMoveCursorOffset(*offset);
}

void backwardSearch() {
  if (isSearchFocus()) {
    setFocusFrame(searchFrameRef);
  }

  setupSearchFocus();

  int docRef = focusDocRef();
  doc_t *doc = focusDoc();
  searchBuffer_t *results = &doc->searchResults;

  recomputeSearch(); // BAL: do only when necessary

  if (results->numElems == 0) return;

  cursor_t *cursor = focusCursor();

  int last = results->numElems - 1;
  for (int i = last; i >= 0; --i) {
    int *offset = arrayElemAt(results, i);
    if (*offset < cursor->offset) {
      stMoveCursorOffset(*offset);
      return;
    }
  }
  int *offset = arrayElemAt(results, last);
  stMoveCursorOffset(*offset);
}

void replace() {
  if (!st.isReplace) return;
  int offset = focusCursor()->offset;
  doc_t *doc = focusDoc();
  docPushDelete(doc, offset, st.searchLen);
  docPushInsert(doc, offset, st.replace.start, st.replace.numElems);
  forwardSearch(); // skip over where we are at
  forwardSearch();
}

bool isSearchFocus()
{
  return (focusFrameRef() == BUILTINS_FRAME && focusViewRef() == SEARCH_BUF);
}

void newSearch() {
  if (!isSearchFocus())
    {
      setupSearchFocus();
      setFocusBuiltinsView(SEARCH_BUF);
    }
  setInsertMode();
  insertNewElem();
}

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

int indentLine() {
  string_t *s = &focusDoc()->contents;
  backwardSOL();
  int offset = focusCursor()->offset;
  int x = distanceToIndent(s->start + offset, arrayTop(s));
  int n = INDENT_LENGTH - x % INDENT_LENGTH;
  char buf[INDENT_LENGTH];
  myMemset(buf, ' ', n);
  insertString(buf, n);
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

// BAL: needed?
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
    cancelSelection();
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

void resizeFont(int dx)
{
  st.font.size += dx;
  reinitFont(&st.font);
  stResize();
}

void increaseFont()
{
  if (st.font.size >= 140) return;
  resizeFont(2);
}

void decreaseFont()
{
  if (st.font.size <= 4) return;
  resizeFont(-1);
}

void doEscapeInsert() {
  setNavigateMode();
}

void doEscapeNavigate() {
  if (isSearchFocus()) {
    return;
  }
  resetSearch();
  cancelSelection();
}

int main(int argc, char **argv) {
  assert(sizeof(char) == 1);
  assert(sizeof(int) == 4);
  assert(sizeof(unsigned int) == 4);
  assert(sizeof(void *) == 8);

  stInit(argc, argv);
  stDraw();
  // BAL: needed?
  //    SDL_AddTimer(1000, timerInit, NULL);
  while (SDL_WaitEvent(&st.event)) {
    switch (st.event.type) {
    case SDL_KEYDOWN:
      keyDownEvent();
      break;
    case SDL_QUIT:
      quitEvent();
      break;
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
    case SDL_USEREVENT:
      //                timerEvent();
      break;
    default:
      break;
    }
    stDraw(); // BAL: only do when needed
  }
  return 0;
}

/*
TODO:
CORE:
    allow searching in builtin buffers
    make sure that the copy buffer works
    reload file when changed outside of editor (inotify?  polling? I think polling, it's simpler)
    periodically save all (modified) files
    add pretty-print hotkey
    do brace insertion on different lines
    remember your place in the file on close (restore all settings/state?)
    Scroll when mouse selection goes off screen command line
    cut/copy/paste with mouse
    create new file
    Hook Cut/Copy/Paste into system Cut/Copy/Paste (done?)
    Goto line
    tab completion
    sort lines
    check spelling
    modify frame widths based on columns (e.g. focus doc frame is 80/120 chars)
    args/config to set config items (e.g. demo mode)
    status bar that has modified, etc.
    input screen (for search, paste, load, tab completion, etc.)
    support editor API
    Name macro
    highlight compile errors/warnings
    jump to next error
    jump to prev/next placeholders
    jump to prev/next change
    collapse/expand selection (code folding)
    make builtin buffers readonly (except config and search?)
    vertical and horizontal scroll bars
    (tons of) optimizations

VIS:
    make font smaller in non-focused frames
    italics, underline and bold in syntax highlighting
    reserved words syntax highlighting
    context sensitive syntax highlighting (parser)
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
Put border on frames
sort directories by name
Search and replace
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
