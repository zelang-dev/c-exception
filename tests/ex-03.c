#include <stdio.h>
#include "exception.h"

EXCEPTION(division_by_zero);

static int idiv(int a, int b)
{
  if (b == 0)
    THROW(division_by_zero);

  return a/b;
}

int main(void)
{
  TRY {
    idiv(1,0);
    printf("never reached\n");
  }
  CATCH(bad_alloc) {
    printf("catch: exception %s (%s:%d) caught\n", ex, ex_file, ex_line);
  }
  CATCH_ANY() {
    printf("catch_any: exception %s (%s:%d) caught\n", ex, ex_file, ex_line);
  }
  ENDTRY

  return 0;
}