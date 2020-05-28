//
//  Doc.h
//  editordebugger
//
//  Created by Brett Letner on 5/27/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#ifndef Doc_h
#define Doc_h

#include "Util.h"

void docDelete(doc_t *doc, uint offset, uint len); // BAL: update cursors?
void docInsert(doc_t *doc, uint offset, char *s, uint len);
void docWrite(doc_t *doc);
void docInit(doc_t *doc, char *filepath);
void docRead(doc_t *doc);

#endif /* Doc_h */
