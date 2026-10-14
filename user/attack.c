#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int main(int argc, char *argv[]) {
  int size = 32 * 4096; 
  char *mem = sbrk(size);
  if (mem == (char*)-1) exit(1);

  char *msg = "Here it is: ";
  int msg_len = strlen(msg);

  for (int i = 0; i < size - msg_len; i++) {
    int match = 1;
    for (int j = 0; j < msg_len; j++) {
      if (mem[i + j] != msg[j]) {
        match = 0;
        break;
      }
    }
    
    if (match) {
      // Chỉ in nếu chuỗi secret dài lớn hơn 0 và là ký tự hợp lệ
      if (mem[i + msg_len] != '\0') {
        printf("%s\n", mem + i + msg_len);
        break;
      }
    }
  }
  exit(0);
}