//
//  Doc.h
//  ceditor
//
//  Created by Brett Letner on 5/27/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#ifndef Doc_h
#define Doc_h

#include "Util.h"

int docDelete(doc_t *doc, int offset, int len); // BAL: update cursors?
void docInsert(doc_t *doc, int offset, char *s, int len);
void docWrite(doc_t *doc);
void docInit(doc_t *doc, char *filepath, bool isUserDoc, bool isReadOnly);
void docRead(doc_t *doc);
char *docCString(doc_t *doc);
void docPushInsert(doc_t *doc, int offset, char *s, int len);
int docPushDelete(doc_t *doc, int offset, int len);
void docMakeAll(void);
int docNumLines(doc_t *doc);
void docIncNumLines(doc_t *doc, int n);

#endif /* Doc_h */
