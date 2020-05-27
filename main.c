// stuff // hello, world

#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <assert.h>
#include "Font.h"
#include "Util.h"
#include "Keysym.h"
#include <sys/stat.h>
#include <limits.h>

void insertCString(state_t *st, char *s);
void insertString(state_t *st, char *s, uint len);

char *builtinBufferTitle[NUM_BUILTIN_BUFFERS] = { "*help", "*messages", "*buffers", "*macros", "*copy buffer", "*searches", "*config" };

color_t viewColors[] = { VIEW_COLOR, FOCUS_VIEW_COLOR };

void setBlendMode(SDL_Renderer *renderer, SDL_BlendMode m)
{
    if (SDL_SetRenderDrawBlendMode(renderer, m) != 0) die(SDL_GetError());
}

void setDrawColor(SDL_Renderer *renderer, color_t c)
{
    color_t alpha = c & 0xff;
    if (alpha == 0xff)
    {
        setBlendMode(renderer, SDL_BLENDMODE_NONE);
    } else
    {
        setBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }
    if (SDL_SetRenderDrawColor(renderer, c >> 24, (c >> 16) & 0xff, (c >> 8) & 0xff, alpha) != 0) die(SDL_GetError());
}

void clearViewport(SDL_Renderer *renderer)
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

struct dynamicArray_s {
    void *start;
    uint numElems;
    uint maxElems;
    uint elemSize;
    uint offset;
};

typedef struct dynamicArray_s dynamicArray_t;

void *dieIfNull(void *p)
{
    if (!p) die("unexpected null pointer");
    return p;
}

void arrayGrow(dynamicArray_t *arr, uint maxElems)
{
    maxElems = max(1, maxElems);
    if (maxElems <= arr->maxElems) return;
    arr->maxElems = maxElems;
    arr->start = dieIfNull(realloc(arr->start, arr->elemSize * maxElems));
}

void arrayReinit(dynamicArray_t *arr)
{
    arr->offset = 0;
    arr->numElems = 0;
}

void arrayInit(dynamicArray_t *arr, uint elemSize)
{
    memset(arr, 0, sizeof(dynamicArray_t));
    arr->elemSize = elemSize;
}

void *arrayPushUninit(dynamicArray_t *arr)
{
    if (arr->numElems == arr->maxElems)
    {
        arrayGrow(arr, arr->maxElems * 2);
    }
    assert(arr->start);
    void *p = arr->start + arr->numElems * arr->elemSize;
    arr->numElems++;
    return p;
}

void arrayPush(dynamicArray_t *arr, void *elem)
{
    memcpy(arrayPushUninit(arr), elem, sizeof(arr->elemSize));
}

void *arrayPop(dynamicArray_t *arr)
{
    assert(arr->start);
    if (arr->numElems == 0) return NULL;
    arr->numElems--;
    return (arr->start + arr->numElems * arr->elemSize);
}

void *arrayElemAt(dynamicArray_t *arr, int i)
{
    if (i < 0 || !arr->start || i >= arr->numElems)
    {
        return NULL;
    }

    return arr->start + i * arr->elemSize;
}

void *arrayPeek(dynamicArray_t *arr)
{
    return arrayElemAt(arr, arr->numElems - 1);
}

void *arrayFocus(dynamicArray_t *arr)
{
    return arrayElemAt(arr, arr->offset);
}

void arraySetFocus(dynamicArray_t *arr, uint i)
{
    arr->offset = clamp(0, i, (int)arr->numElems - 1);
}
uint arrayGetFocus(dynamicArray_t *arr)
{
    return arr->offset;
}
typedef dynamicArray_t string_t; // contains characters

typedef enum { INSERT, DELETE } commandTag_t;

struct command_s {
    commandTag_t tag;
    string_t arg;
};

typedef struct command_s command_t;

typedef dynamicArray_t undoStack_t; // contains commands

struct doc_s {
    char *filepath;
    string_t contents;
    undoStack_t undoStack;
    int numLines;
};

int countLines(char *s, uint len)
{
    int n = 0;
    char *end = s + len;
    
    while(s < end)
    {
        if (*s == '\n') n++;
        s++;
    }
    return n;
}

typedef struct doc_s doc_t;

struct cursor_s {
    int offset;
    int row;
    int column;
    int preferredColumn;
};

typedef struct cursor_s cursor_t;

void cursorSetOffsetString(cursor_t *cursor, int offset, char *s0, int len)
{
    offset = clamp(0, offset, len);
    
    int row = 0;
    
    char *s = s0;
    char *done = s + offset;
    
    char *nl = s;
    
    while (s < done)
    {
        if (*s == '\n') {
            nl = s + 1;
            row++;
        }
        s++;
    }
    
    cursor->offset = offset;
    cursor->row = row;
    int col = (int)(s - nl);
    cursor->column = col;
    cursor->preferredColumn = col;
}

void cursorSetOffset(cursor_t *cursor, int offset, doc_t *doc)
{
    cursorSetOffsetString(cursor, offset, doc->contents.start, doc->contents.numElems);
}

void cursorSetRowColString(cursor_t *cursor, int row0, int col0, char *s0, int len)
{
    int row = 0;
    
    char *s = s0;
    char *eof = s + len;
    char *lastEOL = s - 1;
    
    while (s < eof && row < row0)
    {
        if (*s == '\n') {
            row++;
            lastEOL = s;
        }
        s++;
    }
    s = min(eof, lastEOL + 1);
    char *done = min(eof, s + col0);
    while (s < done && *s != '\n')
    {
        s++;
    }
    
    cursor->offset = (int)(s - s0);
    cursor->row = row;
    int col = (int)(s - lastEOL - 1);
    cursor->column = col;
    cursor->preferredColumn = col;
}

void cursorSetRowCol(cursor_t *cursor, int row, int col, doc_t *doc)
{
    cursorSetRowColString(cursor, row, col, doc->contents.start, doc->contents.numElems);
}

void cursorTest()
{
    cursor_t cursor;
    
    typedef struct {
        int offset;
        char *s;
        int expectedOffset;
        int expectedRow;
        int expectedCol;
    } ctTest_t;
    
    ctTest_t test[] = {
        { 0, "", 0, 0, 0 },
        { 1, "", 0, 0, 0 },
        
        { 0, "a", 0, 0, 0 },
        { 1, "a", 1, 0, 1 },
        { 2, "a", 1, 0, 1 },
        
        { 0, "\n", 0, 0, 0 },
        { 1, "\n", 1, 1, 0 },
        { 2, "\n", 1, 1, 0 },
        
        { 99, "kdkd\n\nkdkd\ndd", 13, 3, 2 },
        { 5, "kdkd\n\nkdkd\ndd", 5, 1, 0 },
        { 10, "kdkd\n\nkdkd\ndd", 10, 2, 4 },
    };


    for(int i = 0; i < sizeof(test) / sizeof(ctTest_t); ++i)
    {
        cursorSetOffsetString(&cursor, test[i].offset, test[i].s, (int)strlen(test[i].s));
        assert(cursor.offset == test[i].expectedOffset);
        assert(cursor.row == test[i].expectedRow);
        assert(cursor.column == test[i].expectedCol);
        cursorSetOffsetString(&cursor, 0, "", 0);
        cursorSetRowColString(&cursor, test[i].expectedRow, test[i].expectedCol, test[i].s, (int)strlen(test[i].s));
        assert(cursor.offset == test[i].expectedOffset);
        assert(cursor.row == test[i].expectedRow);
        assert(cursor.column == test[i].expectedCol);
    }
    
    typedef struct {
        int row;
        int col;
        char *s;
        int expectedOffset;
        int expectedRow;
        int expectedCol;
    } ctTest2_t;

    ctTest2_t test2[] = {
        { 55, 55, "kdkd\n\nkdkd\ndd", 13, 3, 2 },
        { 55, 0, "kdkd\n\nkdkd\ndd", 11, 3, 0 },
        { 2, 55, "kdkd\n\nkdkd\ndd", 10, 2, 4 },
        { 3, 55, "kdkd\n\nkdkd\ndd\n", 13, 3, 2 },
    };

    for(int i = 0; i < sizeof(test2) / sizeof(ctTest2_t); ++i)
    {
        cursorSetRowColString(&cursor, test2[i].row, test2[i].col, test2[i].s, (int)strlen(test2[i].s));
        assert(cursor.offset == test2[i].expectedOffset);
        assert(cursor.row == test2[i].expectedRow);
        assert(cursor.column == test2[i].expectedCol);
        cursorSetOffsetString(&cursor, test2[i].expectedOffset, test2[i].s, (int)strlen(test2[i].s));
        assert(cursor.offset == test2[i].expectedOffset);
        assert(cursor.row == test2[i].expectedRow);
        assert(cursor.column == test2[i].expectedCol);
    }
}

struct view_s // BAL: this needs to be renamed
{
    cursor_t cursor;
    cursor_t selection;
    editorMode_t mode;
    uint refDoc;
};

typedef struct view_s view_t;

struct viewport_s // BAL: this should be a "view"
{
    SDL_Rect rect;
    int scrollX;
    int scrollY;
    uint refView;
};

typedef struct viewport_s viewport_t;
void viewportTrackSelection(viewport_t *viewport, state_t *st);
void viewportScrollY(viewport_t *viewport, int dR, state_t *st);

typedef dynamicArray_t docsBuffer_t;
typedef dynamicArray_t viewsBuffer_t;
//typedef dynamicArray_t messageBuffer_t;

struct window_s
{
    SDL_Window *window;
    int width;
    int height;
};

typedef struct window_s window_t;

typedef dynamicArray_t viewportBuffer_t;
typedef dynamicArray_t searchBuffer_t; // contains result offsets

struct state_s
{
    docsBuffer_t docs;
    viewsBuffer_t views;
    viewportBuffer_t viewports;
    searchBuffer_t results;
    window_t window;
    SDL_Renderer *renderer;
    font_t font;
    SDL_Event event;
    bool isRecording;
    bool selectionInProgress;
    int pushedFocus;
    windowDirty_t dirty;
    uint searchLen;
    uint searchRefView;
    bool searchDirty;
};

typedef struct state_s state_t;

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
    i = clamp(0, i, st->viewports.numElems - 1);
    int j = arrayGetFocus(&st->viewports);
    if (i != j)
    {
        arraySetFocus(&st->viewports, i);
        st->dirty = WINDOW_DIRTY;
    }
}

void stSetViewFocus(state_t *st, uint i)
{
    i = clamp(0, i, st->views.numElems - 1);
    viewport_t *viewport = stViewportFocus(st);
    viewport->refView = i;
}

void cursorRender(view_t *view, viewport_t *viewport, state_t *st)
{
    SDL_Rect rect;
    
    rect.x = view->cursor.column * st->font.charSkip + viewport->scrollX;
    rect.y = 2 + view->cursor.row * st->font.lineSkip + viewport->scrollY;
    rect.w = st->font.charSkip;
    rect.h = st->font.lineSkip;

    if (view->mode == NAVIGATE_MODE)
    {
        setDrawColor(st->renderer, CURSOR_BACKGROUND_COLOR);
        fillRect(st->renderer, &rect);
    }
    
    setDrawColor(st->renderer, CURSOR_COLOR);
    rect.w = CURSOR_WIDTH;
    fillRect(st->renderer, &rect);
}

void viewportInit(viewport_t *v, uint i)
{
    memset(v, 0, sizeof(viewport_t));
    v->refView = i;
}

void cursorInit(cursor_t *c)
{
    memset(c, 0, sizeof(cursor_t));
}

void *arrayBoundary(dynamicArray_t *arr)
{
    return arr->start + arr->numElems * arr->elemSize;
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

#define SELECTION_RECT_GAP 8

void fillSelectionRect(state_t *st, viewport_t *viewport, uint row, uint col, uint h, uint w)
{
    SDL_Rect rect;
    rect.x = col * st->font.charSkip + viewport->scrollX;
    rect.y = row * st->font.lineSkip + SELECTION_RECT_GAP + viewport->scrollY;
    rect.h = h * st->font.lineSkip - SELECTION_RECT_GAP;
    rect.w = w * st->font.charSkip;
    fillRect(st->renderer, &rect);
}

void selectionRender(int aRow, int aCol, int bRow, int bCol, viewport_t *viewport, state_t *st)
{
    assert(viewport);
    assert(st);
    
    int n = bRow - aRow;
    int vCols = viewport->rect.w / st->font.charSkip;
    
    if (n == 0) // single, partial line
    {
        fillSelectionRect(st, viewport, aRow, aCol, 1, 1 + bCol - aCol);
        return;
    }

    // partial line, full lines, partial line
    fillSelectionRect(st, viewport, aRow, aCol, 1, vCols - aCol); // 1st partial line

    n += aRow - 1;
    while (aRow < n) {
        aRow++;
        fillSelectionRect(st, viewport, aRow, 0, 1, vCols); // full lines
    }

    fillSelectionRect(st, viewport, aRow + 1, 0, 1, bCol + 1); // partial line
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
    
    int aCol = view->cursor.column;
    int bCol = view->selection.column;
    int aRow = view->cursor.row;
    int bRow = view->selection.row;

    if (aRow == bRow && aCol == bCol) {
        cursorRender(view, viewport, st);
        return;
    }

    if ((bRow < aRow) || ((aRow == bRow) && (bCol < aCol)))
    {
        swap(int, aCol, bCol);
        swap(int, aRow, bRow);
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

void docWrite(doc_t *doc);

void docDelete(doc_t *doc, uint offset, uint len) // BAL: update cursors?
{
    char *p = doc->contents.start + offset;
    int n = countLines(p, len);
    doc->numLines -= n;
    doc->contents.numElems -= len;
    memmove(p, p + len, doc->contents.numElems - offset);
    if (doc->filepath[0] != '*' && n > 0) docWrite(doc);
}

void docInsert(doc_t *doc, uint offset, char *s, uint len)
{
    // BAL: add to undo
    uint n = doc->contents.numElems + len;
    if (n > doc->contents.maxElems) arrayGrow(&doc->contents, n);
    
    char *p = doc->contents.start + offset;
    uint nl = countLines(s, len);
    doc->numLines += nl;
    memmove(p + len, p, doc->contents.numElems - offset);
    memcpy(p, s, len);
    doc->contents.numElems = n;
    
    if (doc->filepath[0] != '*' && nl > 0) // BAL: hacky to use the '*'
    {
        docWrite(doc);
    }
}

char systemBuf[1024];  // BAL: use string_t

void gitInit(void)
{
    if (system("git inint") != 0) die("git init failed");
}
void docGitAdd(doc_t *doc)
{
    if (DEMO_MODE) return;
    sprintf(systemBuf, "git add %s", doc->filepath);
    if (system(systemBuf) != 0) die("git add failed");
}

void docGitCommit(doc_t *doc)
{
    if (DEMO_MODE) return;
    docGitAdd(doc);
    sprintf(systemBuf, "git commit -m\"cp\" %s", doc->filepath);
    
    if (system(systemBuf) != 0)
         die("git commit failed");
}

void docWrite(doc_t *doc)
{
    if (DEMO_MODE) return;
    FILE *fp = fopen(doc->filepath, "w");
    if (fp == NULL) die("unable to open file for write");

    if (fwrite(doc->contents.start, sizeof(char), doc->contents.numElems, fp) != doc->contents.numElems)
        die("unable to write file");

    if (fclose(fp) != 0)
        die("unable to close file");
    
    docGitCommit(doc);
}

void docRead(doc_t *doc)
{
   // BAL: FILE *fp = fopen(doc->filepath, "a+"); // create file if it doesn't exist
    FILE *fp = fopen(doc->filepath, "r"); // create file if it doesn't exist

    if (!fp) die("unable to open file");
    fseek(fp, 0, SEEK_SET);

    struct stat stat;
    if (fstat(fileno(fp), &stat) != 0) die("unable to get file size");

    doc->contents.numElems = (uint)stat.st_size;
    arrayGrow(&doc->contents, clamp(4096, 2*doc->contents.numElems, UINT_MAX - 1));

    if (fread(doc->contents.start, sizeof(char), stat.st_size, fp) != stat.st_size) die("unable to read in file");
    
    if (fclose(fp) != 0) die("unable to close file");
    
    doc->numLines = countLines(doc->contents.start, doc->contents.numElems);
    
}

void docInit(doc_t *doc, char *filepath)
{
    doc->filepath = filepath;
    arrayInit(&doc->contents, sizeof(char));
    arrayInit(&doc->undoStack, sizeof(command_t));
}

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
    st->pushedFocus = arrayGetFocus(&st->viewports);
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

#define AUTO_SCROLL_HEIGHT 4

void setSelectFromXYMotion(state_t  *st, int x, int y)
{
//    if (y < st->font.lineSkip * AUTO_SCROLL_HEIGHT)
//    {
//        viewportScrollY(st, 1);
//    }
//    if (y > st->window.height - AUTO_SCROLL_HEIGHT * st->font.lineSkip)
//    {
//        viewportScrollY(st, -1);
//    }
    viewport_t *v = stViewportFocus(st);
    view_t *view = stViewFocus(st);
    view->selection.column = clamp(0, (x - v->rect.x) / st->font.charSkip, v->rect.w / st->font.charSkip);
    view->selection.row = (y - v->scrollY - v->rect.y) / st->font.lineSkip;
    // BAL: don't do bounds checking until mouse button up for speed
    // when this is more efficient, remove
}

void setSelectFromXY(state_t *st, int x, int y)
{
    setSelectFromXYMotion(st, x, y);

    view_t *view = stViewFocus(st);
    
    cursorSetRowCol(&view->selection, view->selection.row, view->selection.column, stDocFocus(st));
}

void setCursorFromXY(state_t *st, int x, int y)
{
    setSelectFromXY(st, x, y);
    view_t *view = stViewFocus(st);
    memcpy(&view->cursor, &view->selection, sizeof(cursor_t));
}

void mouseButtonUpEvent(state_t *st)
{
    st->selectionInProgress = false;
    setSelectFromXY(st, st->event.button.x, st->event.button.y);
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
    st->selectionInProgress = true;
    setCursorFromXY(st, st->event.button.x, st->event.button.y);
}

void mouseMotionEvent(state_t *st)
{
    if (st->selectionInProgress)
    {
        setSelectFromXYMotion(st, st->event.motion.x, st->event.motion.y);
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

void stMoveCursorOffset(state_t *st, int offset)
{
    view_t *view = stViewFocus(st);
    doc_t *doc = stDocFocus(st);
    cursorSetOffset(&view->cursor, offset, doc);
    memcpy(&view->selection, &view->cursor, sizeof(cursor_t));
    viewportTrackSelection(stViewportFocus(st), st);
}

void stMoveCursorRowCol(state_t *st, int row, int col)
{
    view_t *focus = stViewFocus(st);
    cursorSetRowCol(&focus->cursor, row, col, stDocFocus(st));
    memcpy(&focus->selection, &focus->cursor, sizeof(cursor_t));
    viewportTrackSelection(stViewportFocus(st), st);
}

void backwardChar(state_t *st)
{
    stMoveCursorOffset(st, stViewFocus(st)->cursor.offset - 1);
}

void forwardChar(state_t *st)
{
    stMoveCursorOffset(st, stViewFocus(st)->cursor.offset + 1);
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
    view_t *focus = stViewFocus(st);
    int col = focus->cursor.preferredColumn;

    stMoveCursorRowCol(st, focus->cursor.row + dRow, col);
    focus->cursor.preferredColumn = col;
    focus->selection.preferredColumn = col;
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
    stMoveCursorRowCol(st, stViewFocus(st)->cursor.row, 0);
}

void forwardEOL(state_t *st)
{
    stMoveCursorRowCol(st, stViewFocus(st)->cursor.row, INT_MAX);
}

void insertString(state_t *st, char *s, uint len)
{
    if (!s || len == 0) return;
    st->dirty = DOC_DIRTY;
    docInsert(stDocFocus(st), stViewFocus(st)->cursor.offset, s, len);
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
    
    int off = cur->offset;
    int len = sel->offset - off + 1;
            
    uint n = stDocFocus(st)->contents.numElems;
    *offset = min(off, n);
    *length = min(len, n - off);
    
    memcpy(sel, cur, sizeof(cursor_t));
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
    if (length <= 0) return;
    st->dirty = DOC_DIRTY;
    doc_t *doc = stDocFocus(st);
    
    if (doc->filepath[0] != '*') copy(st, doc->contents.start + offset, length); // BAL: this was causing problems with search...

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
    memcpy(&cursorFind, &viewFind->cursor, sizeof(cursor_t));
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
      //  computeSearchResults(st);
      //  stSetViewportFocus(st, MAIN_VIEWPORT);
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

    cursorTest();
        
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

