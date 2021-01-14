//
//  Syntax.h
//  ceditor
//
//  Created by Brett Letner on 4/23/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#ifndef Syntax_h
#define Syntax_h

#include "Util.h"

#define preprocColor BRMAGENTA
#define literalColor BRRED
#define commentColor BRGREEN
#define unknownColor MAGENTA
#define uidentColor CYAN
#define lidentColor BRCYAN
#define symbolColor YELLOW
#define specialColor BLUE

typedef enum {
  TOKBEGIN,
  IN_LIDENT,
  IN_UIDENT,
  IN_NUMBER,
  IN_PREPROC,
  IN_SLCOMMENT,
  IN_MLCOMMENT,
  IN_MLCOMMENT_END,
  IN_STRING,
  IN_STRING_ESC,
  IN_CHAR,
  IN_CHAR_ESC
} tokSt_t;

void drawCString(char *s, int n);
void drawString(string_t *s);

#endif /* Syntax_h */

