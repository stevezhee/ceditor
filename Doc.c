//
//  Doc.c
//  editordebugger
//
//  Created by Brett Letner on 5/27/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include "Doc.h"
#include "DynamicArray.h"

static int countLines(char *s, int len);

void docDelete(doc_t *doc, int offset, int len)
{
  int n = countLines(arrayElemAt(&doc->contents, offset), len);
  doc->numLines -= n;
  arrayDelete(&doc->contents, offset, len);
  if (doc->filepath[0] != '*' && n > 0) docWrite(doc);  // BAL: hacky to use the '*' and this shouldn't be here, just mark it as dirty or something
}

void docInsert(doc_t *doc, int offset, char *s, int len)
{
  int n = countLines(s, len);
  doc->numLines += n;

  arrayInsert(&doc->contents, offset, s, len);

  if (doc->filepath[0] != '*' && n > 0) // BAL: hacky to use the '*' and this shouldn't be here, just mark it as dirty or something
    {
      docWrite(doc);
    }
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

}

void docInit(doc_t *doc, char *filepath)
{
  doc->filepath = filepath;
  arrayInit(&doc->contents, sizeof(char));
  arrayInit(&doc->undoStack, sizeof(command_t));
}

void docRead(doc_t *doc)
{
  // BAL: FILE *fp = fopen(doc->filepath, "a+"); // create file if it doesn't exist
  FILE *fp = fopen(doc->filepath, "r"); // create file if it doesn't exist

  if (!fp) die("unable to open file");
  fseek(fp, 0, SEEK_SET);

  struct stat stat;
  if (fstat(fileno(fp), &stat) != 0) die("unable to get file size");

  doc->contents.numElems = (int)stat.st_size;
  arrayGrow(&doc->contents, clamp(4096, 2*doc->contents.numElems, INT_MAX - 1));

  if (fread(doc->contents.start, sizeof(char), stat.st_size, fp) != stat.st_size) die("unable to read in file");

  if (fclose(fp) != 0) die("unable to close file");

  doc->numLines = countLines(doc->contents.start, doc->contents.numElems);
}

static int countLines(char *s, int len)
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
