//
//  Util.c
//  editordebugger
//
//  Created by Brett Letner on 4/23/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "Util.h"

void die(const char *msg)
{
  fprintf(stdout, "error: %s\n", msg);
  exit(1);
}
