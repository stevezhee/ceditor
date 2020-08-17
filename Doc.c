//
//  Doc.c
//  editordebugger
//
//  Created by Brett Letner on 5/27/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include "Doc.h"
#include "DynamicArray.h"

void docDelete(doc_t *doc, int offset, int len)
{
  int n = numLinesString(arrayElemAt(&doc->contents, offset), len);
  docIncNumLines(doc, -n);
  arrayDelete(&doc->contents, offset, len);
  if (doc->isUserDoc && n > 0)
    {
      docWriteAndMake(doc); // BAL: this shouldn't be here, just mark it as dirty or something
    }
}

void docInsert(doc_t *doc, int offset, char *s, int len)
{
  int n = numLinesString(s, len);
  docIncNumLines(doc, n);

  arrayInsert(&doc->contents, offset, s, len);

  if (doc->isUserDoc && n > 0)
    {
      docWriteAndMake(doc); // BAL: this shouldn't be here, just mark it as dirty or something
    }
}

char systemBuf[1024];  // BAL: use string_t

void gitInit(void)
{
  if (DEMO_MODE || NO_GIT) return;
  if (system("git init") != 0) die("git init failed");
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

void docGitCommitAll(void)
{
  if (DEMO_MODE || NO_GIT) return;
  sprintf(systemBuf, "git commit -am\"cp\"");
  system(systemBuf);
}
void docMakeAll(void)
{

  if (DEMO_MODE || NO_COMPILE) return;
  system("make&");
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

void docWriteAndMake(doc_t *doc)
{
  docWrite(doc);
  docMakeAll();
}

void docInit(doc_t *doc, char *filepath, bool isUserDoc, bool isReadOnly)
{
  memset(doc, 0, sizeof(doc_t));
  doc->filepath = filepath;
  doc->isUserDoc = isUserDoc;
  doc->isReadOnly = isReadOnly;
  arrayInit(&doc->contents, sizeof(char));
  arrayInit(&doc->undoStack, sizeof(command_t));
  arrayInit(&doc->searchResults, sizeof(int));
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

  docIncNumLines(doc, numLinesString(doc->contents.start, doc->contents.numElems));
}
int docNumLines(doc_t *doc)
{
  assert(doc->numLines >= 0);
  return doc->numLines;
}
void docIncNumLines(doc_t *doc, int n)
{
  doc->numLines += n;
  assert(doc->numLines >= 0);
}
