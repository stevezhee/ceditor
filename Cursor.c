//
//  Cursor.c
//  editordebugger
//
//  Created by Brett Letner on 5/27/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include "Cursor.h"

void cursorCopy(cursor_t *dst, cursor_t *src)
{
  memcpy(dst, src, sizeof(cursor_t));
}

void cursorInit(cursor_t *c)
{
  memset(c, 0, sizeof(cursor_t));
}

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

void cursorRender(view_t *view, frame_t *frame, state_t *st)
{
  SDL_Rect rect;

  rect.x = view->cursor.column * st->font.charSkip + frame->scrollX;
  rect.y = 2 + view->cursor.row * st->font.lineSkip + frame->scrollY;
  rect.w = st->font.charSkip;
  rect.h = st->font.lineSkip;

  if (view->mode == NAVIGATE_MODE)
    {
      setDrawColor(CURSOR_BACKGROUND_COLOR);
      // BAL: fillRect(st->renderer, &rect);
    }

  setDrawColor(CURSOR_COLOR);
  rect.w = CURSOR_WIDTH;
  // BAL: fillRect(st->renderer, &rect);
}

void fillSelectionRect(state_t *st, frame_t *frame, uint row, uint col, uint h, uint w)
{
    SDL_Rect rect;
    rect.x = col * st->font.charSkip + frame->scrollX;
    rect.y = row * st->font.lineSkip + SELECTION_RECT_GAP + frame->scrollY;
    rect.h = h * st->font.lineSkip - SELECTION_RECT_GAP;
    rect.w = w * st->font.charSkip;
    // BAL: fillRect(st->renderer, &rect);
}

void selectionRender(int aRow, int aCol, int bRow, int bCol, frame_t *frame, state_t *st)
{
    assert(frame);
    assert(st);

    int n = bRow - aRow;
    int vCols = frameColumns(frame, st);

    if (n == 0) // single, partial line
    {
        fillSelectionRect(st, frame, aRow, aCol, 1, 1 + bCol - aCol);
        return;
    }

    // partial line, full lines, partial line
    fillSelectionRect(st, frame, aRow, aCol, 1, vCols - aCol); // 1st partial line

    n += aRow - 1;
    while (aRow < n) {
        aRow++;
        fillSelectionRect(st, frame, aRow, 0, 1, vCols); // full lines
    }

    fillSelectionRect(st, frame, aRow + 1, 0, 1, bCol + 1); // partial line
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
