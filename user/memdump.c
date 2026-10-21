#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *)&s, sizeof(s));

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      int nn = read(0, data + n, sizeof(data) - n);
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

void
memdump(char *fmt, char *data, int len)
{
  for (int i = 0; fmt[i] != '\0'; i++) {
    char f = fmt[i];
    if (f == 'i') {
      if (len < 4) {
        printf("memdump: not enough data for '%c'\n", f);
        break;
      }
      int val = *((int *)data);
      printf("%d\n", val);
      data += 4;
      len -= 4;
    } else if (f == 'p') {
      if (len < 8) {
        printf("memdump: not enough data for '%c'\n", f);
        break;
      }
      uint64 val = *((uint64 *)data);
      printf("%lx\n", val);
      data += 8;
      len -= 8;
    } else if (f == 'h') {
      if (len < 2) {
        printf("memdump: not enough data for '%c'\n", f);
        break;
      }
      ushort val = *((ushort *)data);
      printf("%d\n", val);
      data += 2;
      len -= 2;
    } else if (f == 'c') {
      if (len < 1) {
        printf("memdump: not enough data for '%c'\n", f);
        break;
      }
      char val = *((char *)data);
      printf("%c\n", val);
      data += 1;
      len -= 1;
    } else if (f == 's') {
      if (len < 8) {
        printf("memdump: not enough data for '%c'\n", f);
        break;
      }
      char *str = *((char **)data);
      printf("%s\n", str);
      data += 8;
      len -= 8;
    } else if (f == 'S') {
      while (len > 0 && *data != '\0') {
        printf("%c", *data);
        data++;
        len--;
      }
      printf("\n");
      if (len > 0 && *data == '\0') {
        data++;
        len--;
      }
    }
  }
}
