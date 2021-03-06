//
//  Cursor.h
//  ceditor
//
//  Created by Brett Letner on 5/27/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#ifndef Cursor_h
#define Cursor_h

#include "Util.h"
void cursorCopy(cursor_t *dst, cursor_t *src);
void cursorInit(cursor_t *c);
void cursorSetOffsetString(cursor_t *cursor, int offset, char *s0, int len);
void cursorSetOffset(cursor_t *cursor, int offset, doc_t *doc);
void cursorSetRowColString(cursor_t *cursor, int row0, int col0, char *s0,
                           int len);
void cursorSetRowCol(cursor_t *cursor, int row, int col, doc_t *doc);

#endif /* Cursor_h */
