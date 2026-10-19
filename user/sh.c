// Shell.

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10

#define HISTORY_SIZE 10
#define HISTORY_LINE_SIZE 100

static char cmd_history[HISTORY_SIZE][HISTORY_LINE_SIZE];
static int history_count = 0;
static int history_first = 1;

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void); // Fork but panics on failure.
void panic(char *);
struct cmd *parsecmd(char *);
void runcmd(struct cmd *) __attribute__((noreturn));

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0)
    exit(1);

  switch (cmd->type) {
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd *)cmd;
    if (ecmd->argv[0] == 0)
      exit(1);
    exec(ecmd->argv[0], ecmd->argv);
    fprintf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd *)cmd;
    close(rcmd->fd);
    if (open(rcmd->file, rcmd->mode) < 0) {
      fprintf(2, "open %s failed\n", rcmd->file);
      exit(1);
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd *)cmd;
    if (fork1() == 0)
      runcmd(lcmd->left);
    wait(0);
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd *)cmd;
    if (pipe(p) < 0)
      panic("pipe");
    if (fork1() == 0) {
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if (fork1() == 0) {
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(0);
    wait(0);
    break;

  case BACK:
    bcmd = (struct backcmd *)cmd;
    if (fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit(0);
}

// Find the next directory entry whose name starts with prefix.
// name must have room for DIRSIZ + 1 bytes.
static int
nextmatch(int fd, char *prefix, int plen, char *name)
{
  struct dirent de;

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    // An unused directory entry.
    if (de.inum == 0)
      continue;

    // Directory-entry names may not end with '\0'.
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = '\0';

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

    if ((int)strlen(name) < plen)
      continue;

    if (memcmp(name, prefix, plen) == 0)
      return 1;
  }

  return 0;
}

static void
complete(char *buf, int *len, int capacity)
{
  int start = *len;
  int first_word = 1;
  int fd;
  int count = 0;
  int plen;
  int oldlen;
  int extra;
  char *dir;
  char name[DIRSIZ + 1];
  char only[DIRSIZ + 1];
  struct stat st;

  // Locate the beginning of the word being completed.
  while (start > 0 &&
         buf[start - 1] != ' ' &&
         buf[start - 1] != '\t')
    start--;

  // Is there another word before this one?
  for (int i = 0; i < start; i++) {
    if (buf[i] != ' ' && buf[i] != '\t') {
      first_word = 0;
      break;
    }
  }

  dir = first_word ? "/" : ".";
  plen = *len - start;

  fd = open(dir, O_RDONLY);
  if (fd < 0)
    return;

  if (fstat(fd, &st) < 0 || st.type != T_DIR) {
    close(fd);
    return;
  }

  // First pass: count matches and remember the first name.
  while (nextmatch(fd, buf + start, plen, name)) {
    if (count == 0)
      strcpy(only, name);
    count++;
  }

  close(fd);

  // No matches: leave the line unchanged.
  if (count == 0)
    return;

  // One match: append its missing suffix and a space.
  if (count == 1) {
    extra = (int)strlen(only) - plen;

    // Reserve room for the space, newline, and '\0'.
    if (*len + extra + 1 > capacity - 2)
      return;

    oldlen = *len;

    memmove(buf + *len, only + plen, extra);
    *len += extra;

    buf[(*len)++] = ' ';
    buf[*len] = '\0';

    write(2, buf + oldlen, *len - oldlen);
    return;
  }

  // Multiple matches: list them without modifying the input line.
  fd = open(dir, O_RDONLY);
  if (fd < 0)
    return;

  write(2, "\n", 1);

  int printed = 0;
  while (nextmatch(fd, buf + start, plen, name)) {
    if (printed)
      write(2, " ", 1);

    write(2, name, strlen(name));
    printed = 1;
  }

  close(fd);

  // Redraw the prompt and the line already entered.
  write(2, "\n$ ", 3);
  write(2, buf, *len);
}

// Replace the displayed input line, keeping the prompt intact.
static void
replace_input(char *buf, int *len, int capacity, char *text)
{
  int n = 0;

  // History entries include a newline; do not copy it into the editor.
  while (text[n] != '\0' &&
         text[n] != '\n' &&
         text[n] != '\r' &&
         n < capacity - 2)
    n++;

  // Erase the old input from the screen.
  while (*len > 0) {
    write(2, "\b \b", 3);
    (*len)--;
  }

  // Install and display the replacement.
  memmove(buf, text, n);
  buf[n] = '\0';
  *len = n;

  write(2, buf, n);
}

// Read and edit a line from the console.
// Reserve space for a trailing newline and a terminating '\0'.
int
readconsoleline(char *buf, int nbuf)
{
  int len = 0;
  int cc;
  int escape = 0;
  int history_pos = history_count;
  char draft[HISTORY_LINE_SIZE];
  char c;

  if (nbuf < 2)
    return -1;

  buf[0] = '\0';
  draft[0] = '\0';

  if (consolemode(1) < 0) {
    fprintf(2, "sh: cannot enable console input mode\n");
    return -1;
  }

  write(2, "$ ", 2);

  for (;;) {
    cc = read(0, &c, 1);

    if (cc < 1) {
      consolemode(0);
      return -1;
    }

    // ESC was received. Expect '[' or 'O'.
    if (escape == 1) {
      escape = 0;

      if (c == '[' || c == 'O') {
        escape = 2;
        continue;
      }

      // Otherwise, process this byte normally below.
    }

    // Handle the rest of an escape sequence.
    if (escape == 2) {
      // Skip optional numeric parameters in terminal sequences.
      if ((c >= '0' && c <= '9') || c == ';')
        continue;

      escape = 0;

      if (c == 'A') {
        // Up: move toward older commands.
        if (history_pos > 0) {
          if (history_pos == history_count)
            strcpy(draft, buf);

          history_pos--;
          replace_input(buf, &len, nbuf,
                        cmd_history[history_pos]);
        }
      } else if (c == 'B') {
        // Down: move toward newer commands, then restore the draft.
        if (history_pos < history_count) {
          history_pos++;

          if (history_pos == history_count)
            replace_input(buf, &len, nbuf, draft);
          else
            replace_input(buf, &len, nbuf,
                          cmd_history[history_pos]);
        }
      }

      // Ignore other escape sequences in this basic editor.
      continue;
    }

    // Start an escape sequence.
    if (c == 27) {
      escape = 1;
      continue;
    }

    // Enter: finish the line.
    if (c == '\n' || c == '\r') {
      write(2, "\n", 1);
      break;
    }

    // Ctrl+D: exit on an empty line, otherwise submit the line.
    if (c == 4) {
      write(2, "\n", 1);

      if (len == 0) {
        consolemode(0);
        return -1;
      }

      break;
    }

    // Backspace.
    if (c == 8 || c == 127) {
      if (len > 0) {
        buf[--len] = '\0';
        write(2, "\b \b", 3);
      }
      continue;
    }

    // Ctrl+U: erase the current line.
    if (c == 21) {
      while (len > 0) {
        buf[--len] = '\0';
        write(2, "\b \b", 3);
      }
      continue;
    }

    // Tab completion.
    if (c == '\t') {
      complete(buf, &len, nbuf);
      continue;
    }

    // Ignore other control characters.
    if ((unsigned char)c < 32)
      continue;

    // Ordinary character.
    if (len < nbuf - 2) {
      buf[len++] = c;
      buf[len] = '\0';
      write(2, &c, 1);
    }
  }

  buf[len++] = '\n';
  buf[len] = '\0';

  consolemode(0);
  return 0;
}

int
getcmd(char *buf, int nbuf)
{
  struct stat st;
  int interactive;

  interactive = fstat(0, &st) == 0 && st.type == T_DEVICE;

  memset(buf, 0, nbuf);

  if (interactive)
    return readconsoleline(buf, nbuf);

  // Input from a file or pipe keeps the original behavior.
  gets(buf, nbuf);

  if (buf[0] == 0)
    return -1;

  return 0;
}

static void
add_history(char *line)
{
  if ((int)strlen(line) >= HISTORY_LINE_SIZE)
    return;

  if (history_count == HISTORY_SIZE) {
    // Drop the oldest command and shift the others left.
    memmove(cmd_history[0], cmd_history[1],
            (HISTORY_SIZE - 1) * sizeof(cmd_history[0]));

    history_count--;
    history_first++;
  }

  strcpy(cmd_history[history_count], line);
  history_count++;
}

static void
show_history(void)
{
  for (int i = 0; i < history_count; i++) {
    printf("%d %s", history_first + i, cmd_history[i]);

    // Stored commands normally already contain a newline.
    int n = strlen(cmd_history[i]);
    if (n == 0 || cmd_history[i][n - 1] != '\n')
      printf("\n");
  }
}

// Return 0 on success, -1 on an invalid history request.
// Ordinary commands are left unchanged.
static int
expand_history(char *cmd, int capacity)
{
  int number;
  int last;
  int i;
  int digit;
  int length;
  char saved[HISTORY_LINE_SIZE];

  if (cmd[0] != '!')
    return 0;

  if (history_count == 0) {
    fprintf(2, "sh: history is empty\n");
    return -1;
  }

  last = history_first + history_count - 1;

  if (cmd[1] == '!') {
    // !! means the most recent command.
    number = last;
    i = 2;
  } else {
    // !n means the command whose displayed number is n.
    number = 0;
    i = 1;

    if (cmd[i] < '0' || cmd[i] > '9')
      goto invalid;

    while (cmd[i] >= '0' && cmd[i] <= '9') {
      digit = cmd[i] - '0';

      // Reject numbers above last without overflowing int.
      if (number > last / 10 ||
          (number == last / 10 && digit > last % 10))
        goto invalid;

      number = number * 10 + digit;
      i++;
    }
  }

  // This version accepts only !! or !n, with optional trailing spaces.
  while (cmd[i] == ' ' || cmd[i] == '\t')
    i++;

  if (cmd[i] == '\n')
    i++;

  if (cmd[i] != '\0')
    goto invalid;

  if (number < history_first || number > last)
    goto invalid;

  // Copy before add_history() can shift or overwrite the history array.
  strcpy(saved, cmd_history[number - history_first]);

  length = strlen(saved);
  if (length >= capacity) {
    fprintf(2, "sh: history command is too long\n");
    return -1;
  }

  strcpy(cmd, saved);

  // Show the expanded command before executing it.
  fprintf(2, "%s", cmd);
  if (length == 0 || cmd[length - 1] != '\n')
    fprintf(2, "\n");

  return 0;

invalid:
  fprintf(2, "sh: invalid or unavailable history number\n");
  return -1;
}

int
main(void)
{
  static char buf[100];
  int fd;

  // Ensure that three file descriptors are open.
  while ((fd = open("console", O_RDWR)) >= 0) {
    if (fd >= 3) {
      close(fd);
      break;
    }
  }

  // Read and run input commands.
  while (getcmd(buf, sizeof(buf)) >= 0) {
    char *cmd = buf;
    while (*cmd == ' ' || *cmd == '\t')
      cmd++;
    if (*cmd == '\n') // is a blank command
      continue;

    // Expand before saving the command into history.
    if (expand_history(cmd, sizeof(buf) - (cmd - buf)) < 0)
      continue;

    // Save the command before executing it.
    add_history(cmd);

    // Handle history in the shell itself.
    if (strcmp(cmd, "history\n") == 0 ||
        strcmp(cmd, "history") == 0) {
      show_history();
      continue;
    }

    if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
      // Chdir must be called by the parent, not the child.
      cmd[strlen(cmd) - 1] = 0; // chop \n
      if (chdir(cmd + 3) < 0)
        fprintf(2, "cannot cd %s\n", cmd + 3);
    } else {
      if (fork1() == 0)
        runcmd(parsecmd(cmd));
      wait(0);
    }
  }
  exit(0);
}

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if (pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd *
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd *)cmd;
}

struct cmd *
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd *)cmd;
}

struct cmd *
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd *)cmd;
}

struct cmd *
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd *)cmd;
}

struct cmd *
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd *)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;
  if (q)
    *q = s;
  ret = *s;
  switch (*s) {
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if (*s == '>') {
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if (eq)
    *eq = s;

  while (s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);
struct cmd *nulterminate(struct cmd *);

struct cmd *
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if (s != es) {
    fprintf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd *
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while (peek(ps, es, "&")) {
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if (peek(ps, es, ";")) {
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd *
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if (peek(ps, es, "|")) {
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd *
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while (peek(ps, es, "<>")) {
    tok = gettoken(ps, es, 0, 0);
    if (gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch (tok) {
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE | O_TRUNC, 1);
      break;
    case '+': // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd *
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if (!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if (!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd *
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if (peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd *)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while (!peek(ps, es, "|)&;")) {
    if ((tok = gettoken(ps, es, &q, &eq)) == 0)
      break;
    if (tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if (argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd *
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0)
    return 0;

  switch (cmd->type) {
  case EXEC:
    ecmd = (struct execcmd *)cmd;
    for (i = 0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd *)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd *)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd *)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd *)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}
