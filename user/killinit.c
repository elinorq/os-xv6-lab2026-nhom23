#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  write(1, "PoC: calling kill(1)\n", 21);

  int result = kill(1);
  if (result < 0)
    write(1, "blocked\n", 8);
  else
    write(1, "accepted\n", 9);

  pause(20);
  write(1, "alive\n", 6);
  exit(0);
}
