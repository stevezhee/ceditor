//
//  Doc.c
//  ceditor
//
//  Created by Brett Letner on 5/27/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#include "Doc.h"
#include "DynamicArray.h"

int docDelete(doc_t *doc, int offset, int len) {
  int n = numLinesString(arrayElemAt(&doc->contents, offset), len);
  docIncNumLines(doc, -n);
  doc->modified = true;
  return arrayDelete(&doc->contents, offset, len);
}

void docInsert(doc_t *doc, int offset, char *s, int len) {
  int n = numLinesString(s, len);
  docIncNumLines(doc, n);
  doc->modified = true;
  arrayInsert(&doc->contents, offset, s, len);
}

void docWrite(doc_t *doc) {
  if (DEMO_MODE)
    return;
  if (!doc->modified)
    return;
  FILE *fp = fopen(doc->filepath, "w");
  if (!fp)
    die("unable to open file for write");

  if (fwrite(doc->contents.start, sizeof(char), doc->contents.numElems, fp) !=
      doc->contents.numElems)
    die("unable to write file");

  if (fclose(fp) != 0)
    die("unable to close file");

  doc->modified = false;
}

void docInit(doc_t *doc, char *filepath, bool isUserDoc, bool isReadOnly) {
  memset(doc, 0, sizeof(doc_t));
  doc->filepath = filepath;
  doc->isUserDoc = isUserDoc;
  doc->isReadOnly = isReadOnly;
  arrayInit(&doc->contents, sizeof(char));
  arrayInit(&doc->undoStack, sizeof(command_t));
  arrayInit(&doc->searchResults, sizeof(int));
}

void docRead(doc_t *doc) {
  FILE *fp = fopen(doc->filepath, "a+"); // create file if it doesn't

  if (!fp)
    die("unable to open file");
  fseek(fp, 0, SEEK_SET);

  struct stat stat;
  if (fstat(fileno(fp), &stat) != 0)
    die("unable to get file size");

  doc->contents.numElems = (int)stat.st_size;
  arrayGrow(&doc->contents,
            clamp(4096, 2 * doc->contents.numElems, INT_MAX - 1));

  if (fread(doc->contents.start, sizeof(char), stat.st_size, fp) !=
      stat.st_size)
    die("unable to read in file");

  if (fclose(fp) != 0)
    die("unable to close file");

  docIncNumLines(doc,
                 numLinesString(doc->contents.start, doc->contents.numElems));
}
int docNumLines(doc_t *doc) {
  assert(doc->numLines >= 0);
  return doc->numLines;
}
void docIncNumLines(doc_t *doc, int n) {
  doc->numLines += n;
  assert(doc->numLines >= 0);
}
