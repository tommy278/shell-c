#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

const int SUCCESS = 0;
const int ERROR = -1;

#define EXTERNAL_OK   5
#define FORK_ERROR    4
#define FILE_ERROR    3
#define BUILTIN_ERROR 1
#define BUILTIN_OK    0

#define MAX_LINE_LENGTH 256

static char path[256];

typedef struct {
  int len;
  char *history[64];
} History;

void append (History *self, char *tokens[64]) {
  if (self->len >= 64) { return; };

  char buf[1024] = {0};

  for (int i = 0; tokens[i] != NULL; i++) {
    strncat(buf, tokens[i], sizeof(buf) - strlen(buf) - 1);
    if (tokens[i + 1] != NULL) {
      strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
    }
  }

  FILE *fptr;
  self->history[self->len] = strdup(buf);
  fptr = fopen(path, "a");
  fprintf(fptr, "%s\n", self->history[self->len]);
  fclose(fptr);
  self->len++;
}

int find_op(char *tokens[64], const char* op);
int exec_cmd(char *tokens[64], char buffer[1024], History *history, FILE *fptr);

int main() {
  char *tokens[64];
  History *history = malloc(sizeof(*history));

  FILE *fptr;
  char line[MAX_LINE_LENGTH];

  snprintf(path, sizeof(path), "%s/.shell_history", getenv("HOME"));
  fptr = fopen(path, "r");

  if (fptr) { 
    while (fgets(line, MAX_LINE_LENGTH, fptr) != NULL) {
      line[strcspn(line, "\n")] = 0;
      history->history[history->len++] = strdup(line);
    }
    fclose(fptr);
  }

  while (1) {
  char buffer[1024];

  printf("\n$ ");

  if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    int result = exec_cmd(tokens, buffer, history, fptr);
    switch(result) {
      case BUILTIN_ERROR:
        exit(1);
      case BUILTIN_OK:
        continue;
      case EXTERNAL_OK:
        continue;
      }
    }
  }
  for (int i = 0; i < history->len; i++) {
    free(history->history[i]);
  }
  return 0;
}

int exec_cmd(char *tokens[64], char buffer[1024], History *history, FILE *fptr) {
  char *token = strtok(buffer, " ");

  int count = 0;

  while (token != NULL) {
    token[strcspn(token, "\n")] = '\0';
    tokens[count++] = token;
    token = strtok(NULL, " ");
  }
  tokens[count] = NULL;

  int fork_index =  find_op(tokens, "|");
  if (fork_index > 0) {
    append(history, tokens);
    tokens[fork_index] = NULL;
    char **left = tokens;
    char **right = tokens + fork_index + 1;

    int fds[2];
    pipe(fds);

    pid_t process_id_1, process_id_2;

    process_id_1 = fork();
    if (process_id_1 == 0) {
      dup2(fds[1], STDOUT_FILENO);
      close(fds[0]);
      close(fds[1]);
      execvp(left[0], left);
      exit(1);
    } else {
      process_id_2 = fork();
      if (process_id_2 < 0) {
        fprintf(stderr, "fork failed\n");
        return FORK_ERROR;
      }
      
      if (process_id_2 == 0) {
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);
        execvp(right[0], right);
        exit(1);
      }
    }

    close(fds[0]);
    close(fds[1]);
    waitpid(process_id_2, NULL, 0);
    waitpid(process_id_1, NULL, 0);
    return 0;
  }
  
  if (strcmp(tokens[0], "exit") == SUCCESS) {
    exit(BUILTIN_OK);
  } else if (strcmp(tokens[0], "cd") == SUCCESS) {
      if (tokens[1] == NULL) {
        fprintf(stderr, "directory not found: ");
      }
      if (chdir(tokens[1]) == ERROR) {
        perror("Invalid directory\n");
        return BUILTIN_ERROR;
      }
      append(history, tokens);
    } else if (strcmp(tokens[0], "pwd") == SUCCESS) {
      char *buf = malloc(512);
      char *result = getcwd(buf, 512);
      if (result == NULL) {
        perror("Error printing directory\n");
        return BUILTIN_ERROR;
      } else {
        printf("%s\n", result);
        append(history, tokens);
        free(buf);
        return BUILTIN_OK;
      }
    } else if(strcmp(tokens[0], "history") == 0) {
      printf("\n");
      for (int i = 0; i < history->len; i++) {
        printf("%s\n", history->history[i]);
      }
      printf("\n");
      append(history, tokens);
      return BUILTIN_OK;
    } else {
      pid_t pid = fork();
      if (pid == 0) {
        int r_index = find_op(tokens, ">");
        if (r_index > 0) {
          const char *file_name = tokens[r_index + 1];
          tokens[r_index] = NULL;
          tokens[r_index + 1] = NULL;
          int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);

          if (fd == -1) {
            perror("error opening file");
            exit(FILE_ERROR);
          }
          
          dup2(fd, STDOUT_FILENO);
          close(fd);
        }

        int a_index = find_op(tokens, ">>");
        if (a_index > 0) {
          const char *file_name = tokens[a_index + 1];
          tokens[a_index] = NULL;
          tokens[a_index + 1] = NULL;
          int fd = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0644);

          if (fd == -1) {
            perror("error opening file");
            exit(FILE_ERROR);
          }
          dup2(fd, STDOUT_FILENO);
          close(fd);
        }

        int ir_index = find_op(tokens, "<");
        if (ir_index > 0) {
          const char *file_name = tokens[ir_index + 1];
          tokens[ir_index] = NULL;
          tokens[ir_index + 1] = NULL;

          int fd = open(file_name, O_RDONLY, 0644);
          if (fd == -1) {
            perror("error opening file");
            exit(FILE_ERROR);
          }
          
          dup2(fd, STDIN_FILENO);
          close(fd); 
        }

        execvp(tokens[0], tokens);
        exit(1);
       } else if (pid > 0){
        waitpid(pid, NULL, 0);
        append(history, tokens);
        return EXTERNAL_OK;
      } else if (strcmp(tokens[0], "echo") == SUCCESS) {
        int i = 1;
        while (tokens[i] != NULL) {
          printf("%s ", tokens[i++]);
        }
        printf("\n");
        return BUILTIN_OK;
    } else {
        append(history, tokens);
        perror("fork failed\n");
        return BUILTIN_ERROR;
      }
    }

    return 0;
}

int find_op(char *tokens[64], const char *op) {
  int i = 0, idx = 0;
  while (tokens[i] != NULL) {
    if (strcmp(tokens[i], op) == 0) {
      idx = i;
    }
    i++;
  }
  return idx;  
}
