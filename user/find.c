#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
void find(char *path, char *target, int argc, char *argv[]);
char *get_filename(char *path);
void run_cmd(char *argv[], int argc, char *path);
int
main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("usage: find <path> <filename>\n");
    exit(1);
  }
  find(argv[1], argv[2], argc, argv);
  exit(0);
}
char *
get_filename(char *path)
{
  char *p;
  for (p = path + strlen(path); p >= path && *p != '/'; p--) {
    ;
  }
  p++;
  return p;
}
void
find(char *path, char *target, int argc, char *argv[])
{
  int fd;
  struct dirent de;
  struct stat st;
  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  switch (st.type) {
  case T_DEVICE:
  case T_FILE:
    if (strcmp(get_filename(path), target) == 0) {
      if (argc > 3 && strcmp(argv[3], "-exec") == 0) {
        run_cmd(argv, argc, path);
      } else {
        printf("%s\n", path);
      }
    }
    break;
  case T_DIR:
    if (strcmp(get_filename(path), target) == 0) {
      if (argc > 3 && strcmp(argv[3], "-exec") == 0) {
        run_cmd(argv, argc, path);
      } else {
        printf("%s\n", path);
      }
    }
    char buf[512];
    char *p;
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p = '/';
    p++;
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, target, argc, argv);
    }
    break;
  }
  close(fd);
}
void
run_cmd(char *argv[], int argc, char *path)
{
  char *cmd[MAXARG];
  int k = 0;
  for (int i = 4; i < argc; i++) {
    cmd[k++] = argv[i];
  }
  cmd[k++] = path;
  cmd[k] = 0;
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "find: fork failed\n");
    return;
  }
  if (pid == 0) {
    exec(cmd[0], cmd);
    fprintf(2, "find: exec %s failed\n", cmd[0]);
    exit(1);
  }
  if (pid > 0) {
    wait(0);
  }
}