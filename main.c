#include "Cursor.h"
#include "Doc.h"
#include "DynamicArray.h"
#include "Font.h"
#include "Keysym.h"
#include "Util.h"
#include "Search.h"

void insertCString(state_t *st, char *s);
void insertString(state_t *st, char *s, uint len);

void viewportTrackSelection(viewport_t *viewport, state_t *st);
void viewportScrollY(viewport_t *viewport, int dR, state_t *st);
void viewportRender(viewport_t *viewport, state_t *st); // BAL: viewportFocusRender
void stRender(state_t *st);

viewport_t *stViewportFocus(state_t *st)
{
    return arrayFocus(&st->viewports);
}

view_t *stViewFocus(state_t *st)
{
    return arrayElemAt(&st->views, stViewportFocus(st)->refView);
}

doc_t *stDocFocus(state_t *st)
{
    return arrayElemAt(&st->docs, stViewFocus(st)->refDoc);
}

int viewportHeight(viewport_t *viewport)
{
    return viewport->rect.h;
}

uint viewportRows(viewport_t *viewport, state_t *st)
{
    return viewportHeight(viewport) / st->font.lineSkip;
}

void stSetViewportFocus(state_t *st, int i)
{
    int j = arrayFocusOffset(&st->viewports);
    if (i != j)
    {
        arraySetFocus(&st->viewports, i);
        st->dirty = WINDOW_DIRTY;
        printf("file:%s\n", stDocFocus(st)->filepath);
    }
}

void stSetViewFocus(state_t *st, uint i)
{
    i = clamp(0, i, st->views.numElems - 1);
    viewport_t *viewport = stViewportFocus(st);
    viewport->refView = i;
    printf("file:%s\n", stDocFocus(st)->filepath);
}

void viewportInit(viewport_t *v, uint i)
{
    memset(v, 0, sizeof(viewport_t));
    v->refView = i;
}

void viewInit(view_t *view, uint i)
{
    memset(view, 0, sizeof(view_t));
    view->refDoc = i;
}

void docRender(doc_t *doc, viewport_t *viewport, state_t *st)
{
    resetCharRect(&st->font, viewport->scrollX, viewport->scrollY);

    if(!doc->contents.start) return;

    for(char *p = doc->contents.start; p < (char*)(doc->contents.start) + doc->contents.numElems; p++)
    {
        renderAndAdvChar(&st->font, *p);
    }

    renderEOF(&st->font);
}

bool noActiveSearch(viewport_t *viewport, state_t *st)
{
    return
      viewport->refView != st->searchRefView ||
      st->searchLen == 0 ||
      st->results.numElems == 0;
}

void searchRender(viewport_t *viewport, state_t *st)
{
    if (noActiveSearch(viewport, st)) return;

    setDrawColor(st->renderer, SEARCH_COLOR);

    view_t *view = arrayElemAt(&st->views, viewport->refView);
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
        selectionRender(aRow, aCol, bRow, bCol, viewport, st);
    }
}

void viewRender(view_t *view, viewport_t *viewport, state_t *st)
{
    assert(view);

    if (!view->selectionInProgress && !view->selectionActive) {
      cursorRender(view, viewport, st);
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
        bCol = viewportColumns(viewport, st);
      }
    setDrawColor(st->renderer, SELECTION_COLOR);
    selectionRender(aRow, aCol, bRow, bCol, viewport, st);
}

void viewportRender(viewport_t *viewport, state_t *st)
{
    setViewport(st->renderer, &viewport->rect);

    SDL_Rect bkgd;
    bkgd.x = 0;
    bkgd.y = 0;
    bkgd.w = viewport->rect.w;
    bkgd.h = viewport->rect.h;

    color_t color = viewColors[viewport == stViewportFocus(st)];
    setDrawColor(st->renderer, color);
    fillRect(st->renderer, &bkgd); // background

    view_t *view = arrayElemAt(&st->views, viewport->refView);
    searchRender(viewport, st);
    viewRender(view, viewport, st);
    docRender(arrayElemAt(&st->docs, view->refDoc), viewport, st);
}

void stRender(state_t *st) // BAL: rename stFocusRender
{
    viewportRender(stViewportFocus(st), st);
    SDL_RenderPresent(st->renderer);
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
    setDrawColor(st->renderer, BACKGROUND_COLOR);
    clearViewport(st->renderer);

    for(int i = 0; i < st->viewports.numElems; ++i)
    {
        viewportRender(arrayElemAt(&st->viewports, i), st);
    }

    SDL_RenderPresent(st->renderer);
}

void stResize(state_t *st)
{
    int w;
    int h;

    SDL_GetWindowSize(st->window.window, &w, &h);
    st->window.width = w;
    st->window.height = h;

    w = (w - BORDER_WIDTH) / 3;
    h = h - 2*BORDER_WIDTH;

    viewport_t *v0 = arrayElemAt(&st->viewports, SECONDARY_VIEWPORT);
    v0->rect.x = BORDER_WIDTH;
    v0->rect.y = BORDER_WIDTH;
    v0->rect.w = w - BORDER_WIDTH;
    v0->rect.h = h;

    viewport_t *v1 = arrayElemAt(&st->viewports, MAIN_VIEWPORT);
    v1->rect.x = BORDER_WIDTH + v0->rect.x + v0->rect.w;
    v1->rect.y = BORDER_WIDTH;
    v1->rect.w = w + w / 2 - BORDER_WIDTH;
    v1->rect.h = h;

    viewport_t *v2 = arrayElemAt(&st->viewports, BUILTINS_VIEWPORT);
    v2->rect.x = BORDER_WIDTH + v1->rect.x + v1->rect.w;
    v2->rect.y = BORDER_WIDTH;
    v2->rect.w = st->window.width - v2->rect.x - BORDER_WIDTH;
    v2->rect.h = h;

    st->dirty = WINDOW_DIRTY;
}

void builtinsPushFocus(state_t *st, int i)
{
  st->pushedFocus = arrayFocusOffset(&st->viewports);
  stSetViewportFocus(st, BUILTINS_VIEWPORT);
  stSetViewFocus(st, i);
}

void builtinsPopFocus(state_t *st)
{
    stSetViewportFocus(st, st->pushedFocus);
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

void stInit(state_t *st, int argc, char **argv)
{
    argc--;
    argv++;

    initMacros();

    if (argc < 1) die("expected filepath(s)");
    memset(st, 0, sizeof(state_t));

    arrayInit(&st->docs, sizeof(doc_t));
    arrayInit(&st->views, sizeof(view_t));
    arrayInit(&st->viewports, sizeof(viewport_t));
    arrayInit(&st->results, sizeof(uint));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) die(SDL_GetError());

    windowInit(&st->window, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);
    st->renderer = dieIfNull(SDL_CreateRenderer(st->window.window, -1, SDL_RENDERER_SOFTWARE));

    initFont(&st->font, st->renderer, INIT_FONT_FILE, INIT_FONT_SIZE);

    keysymInit();

    for(int i = 0; i < NUM_BUILTIN_BUFFERS; ++i)
    {
        doc_t *doc = arrayPushUninit(&st->docs);
        docInit(doc, builtinBufferTitle[i]);
        viewInit(arrayPushUninit(&st->views), i);
    }

    for(int i = 0; i < argc; ++i) {
        doc_t *doc = arrayPushUninit(&st->docs);
        docInit(doc, argv[i]);
        docRead(doc);
        viewInit(arrayPushUninit(&st->views), NUM_BUILTIN_BUFFERS + i);
    }

    for(int i = 0; i < NUM_VIEWPORTS; ++i)
    {
        viewport_t *v = arrayPushUninit(&st->viewports);
        viewportInit(v, i);
    }

    buffersBufInit(st);

    message(st, "hello from the beyond\n");

    stSetViewportFocus(st, SECONDARY_VIEWPORT);
    stSetViewFocus(st, MESSAGE_BUF);
    stSetViewportFocus(st, BUILTINS_VIEWPORT);
    stSetViewFocus(st, BUFFERS_BUF);
    stSetViewportFocus(st, MAIN_VIEWPORT);
    stSetViewFocus(st, NUM_BUILTIN_BUFFERS);

    stResize(st);
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
            stResize(st);
            break;
        default:
            st->dirty = NOT_DIRTY;
            break;
    }
}

void setCursorFromXYMotion(cursor_t *cursor, state_t  *st, int x, int y)
{
    viewport_t *viewport = stViewportFocus(st);
    view_t *view = stViewFocus(st);
    cursor->column =
      clamp(0, (x - viewport->rect.x) / st->font.charSkip, viewportColumns(viewport, st));
    cursor->row = (y - viewport->scrollY - viewport->rect.y) / st->font.lineSkip;
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
  view_t *view = stViewFocus(st);
  setCursorFromXY(&view->selection, st, st->event.button.x, st->event.button.y);
    selectionEnd(st);
}

void mouseButtonDownEvent(state_t *st)
{
    int i = 0;
    viewport_t *viewport;

    while (true) // find selected viewport
    {
        SDL_Point p;
        p.x = st->event.button.x;
        p.y = st->event.button.y;
        viewport = arrayElemAt(&st->viewports, i);
        if (SDL_PointInRect(&p, &viewport->rect)) break;
        i++;
        if (i == NUM_VIEWPORTS) return;
    };

    stSetViewportFocus(st, i);
    view_t *view = stViewFocus(st);
    setCursorFromXY(&view->cursor, st, st->event.button.x, st->event.button.y);
    selectChars(st);
}

void mouseMotionEvent(state_t *st)
{
    view_t *view = stViewFocus(st);
    if (view->selectionInProgress)
    {
      setCursorFromXYMotion(&view->selection, st, st->event.motion.x, st->event.motion.y);
        return;
    }
    st->dirty = NOT_DIRTY;
}

int docHeight(doc_t *doc, state_t *st)
{
    return doc->numLines * st->font.lineSkip;
}

void viewportScrollY(viewport_t *viewport, int dR, state_t *st)
{
    view_t *view = arrayElemAt(&st->views, viewport->refView);
    doc_t *doc = arrayElemAt(&st->docs, view->refDoc);

    viewport->scrollY += dR * st->font.lineSkip;

    viewport->scrollY = clamp(viewportHeight(viewport) - docHeight(doc,st) - st->font.lineSkip, viewport->scrollY, 0);
}

void mouseWheelEvent(state_t *st)
{
    viewportScrollY(stViewportFocus(st), st->event.wheel.y, st);
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
    viewportTrackSelection(stViewportFocus(st), st);
}

void stMoveCursorRowCol(state_t *st, int row, int col)
{
    cursorSetRowCol(activeCursor(st), row, col, stDocFocus(st));
    viewportTrackSelection(stViewportFocus(st), st);
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
    moveLines(st, -max(1, viewportRows(stViewportFocus(st), st) - AUTO_SCROLL_HEIGHT));
}

void forwardPage(state_t *st)
{
    moveLines(st, max(1, viewportRows(stViewportFocus(st), st) - AUTO_SCROLL_HEIGHT));
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
    viewport_t *viewport = stViewportFocus(st);
    if (noActiveSearch(viewport, st)) return;
    view_t *view = arrayElemAt(&st->views, viewport->refView);
    doc_t *doc = arrayElemAt(&st->docs, view->refDoc);
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
    if ((stViewportFocus(st)->refView) == SEARCH_BUF)
    {
      // builtinsPopFocus(st);
      //  computeSearchResults(st);

      stSetViewportFocus(st, MAIN_VIEWPORT);

      //  stSetViewFocus(st, st->searchRefView);
      //  setCursorToSearch(st);
        return;
    }

    st->searchRefView = stViewportFocus(st)->refView;

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
    int frame = st->viewports.offset;

    if (frame == BUILTINS_VIEWPORT) frame = MAIN_VIEWPORT;

    stSetViewportFocus(st, BUILTINS_VIEWPORT);
    stSetViewFocus(st, BUFFERS_BUF);

    view_t *view = stViewFocus(st);

    stSetViewportFocus(st, frame);
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

void viewportTrackSelection(viewport_t *viewport, state_t *st)
{
    view_t *view = arrayElemAt(&st->views, viewport->refView);
    int scrollR = viewport->scrollY / st->font.lineSkip;
    int dR = view->selection.row + scrollR;
    if (dR < AUTO_SCROLL_HEIGHT)
    {
        viewportScrollY(viewport, AUTO_SCROLL_HEIGHT - dR, st);
    } else {
        int lastRow = viewportRows(viewport, st) - AUTO_SCROLL_HEIGHT;
        if (dR > lastRow) {
            viewportScrollY(viewport, lastRow - dR, st);
        }
    }
}

int main(int argc, char **argv)
{
    assert(sizeof(char) == 1);
    assert(sizeof(int) == 4);
    assert(sizeof(unsigned int) == 4);
    assert(sizeof(void*) == 8);

    state_t st;

    stInit(&st, argc, argv);
    stRenderFull(&st);

    while (SDL_WaitEvent(&st.event))
    {
        st.dirty = FOCUS_DIRTY;
        switch (st.event.type) {
            case SDL_QUIT:
                quitEvent(&st);
                break;
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
            case SDL_KEYDOWN:
                keyDownEvent(&st);
                break;
            default:
                st.dirty = NOT_DIRTY;
                break;
        }
        if (st.dirty & DOC_DIRTY) // BAL: noActiveSearch?
        {
            computeSearchResults(&st);
        }
        if (st.dirty & WINDOW_DIRTY || st.dirty & DOC_DIRTY)
        {
            stRenderFull(&st);
        } else if (st.dirty & FOCUS_DIRTY || st.dirty & DOC_DIRTY)
        {
            stRender(&st);
        }
    }

  return 0;
}

/*
TODO:
CORE:
    command line args/config to set config items (e.g. demo mode)
    status bar that has filename, row/col, modified, etc.
    cursor per viewport
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
    status bar per viewport
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
    switch between viewports
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

