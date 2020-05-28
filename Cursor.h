//
//  Cursor.h
//  editordebugger
//
//  Created by Brett Letner on 5/27/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#ifndef Cursor_h
#define Cursor_h

#include "Util.h"
void cursorCopy(cursor_t *dst, cursor_t *src);
void cursorInit(cursor_t *c);
void cursorSetOffsetString(cursor_t *cursor, int offset, char *s0, int len);
void cursorSetOffset(cursor_t *cursor, int offset, doc_t *doc);
void cursorSetRowColString(cursor_t *cursor, int row0, int col0, char *s0, int len);
void cursorSetRowCol(cursor_t *cursor, int row, int col, doc_t *doc);
void cursorRender(view_t *view, viewport_t *viewport, state_t *st);
void fillSelectionRect(state_t *st, viewport_t *viewport, uint row, uint col, uint h, uint w);
void selectionRender(int aRow, int aCol, int bRow, int bCol, viewport_t *viewport, state_t *st);

#endif /* Cursor_h */

