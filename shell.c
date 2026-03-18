#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

const int SUCCESS = 0;
const int ERROR = -1;

#define BUILTIN_ERROR 1
#define BUILTIN_OK    0

typedef struct {
  int len;
  char *history[64];
} History;

void append (History *self, char *tokens[64]) {
  if (self->len > 64) { return; };

  char buf[1024] = {0};

  for (int i = 0; tokens[i] != NULL; i++) {
    strncat(buf, tokens[i], sizeof(buf) - strlen(buf) - 1);
    if (tokens[i + 1] != NULL) {
      strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
    }
  }
  self->history[self->len++] = strdup(buf);
}


int exec_cmd(char *tokens[64], char buffer[1024], History *history);

int main() {
  char *tokens[64];
  History *history = malloc(sizeof(*history));

  while (2) {
  char buffer[1024];

  printf("\n$ ");

  if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    int result = exec_cmd(tokens, buffer, history);
    switch(result) {
      case BUILTIN_ERROR:
        exit(1);
      case BUILTIN_OK:
        continue;
    }
   }
  }
  for (int i = 0; i < history->len; i++) {
    free(history->history[i]);
  }
  return 0;
}

int exec_cmd(char *tokens[64], char buffer[1024], History *history) {
  char *token = strtok(buffer, " ");

  int count = 0;

  while (token != NULL) {
    token[strcspn(token, "\n")] = '\0';
    tokens[count++] = token;
    token = strtok(NULL, " ");
  }
  tokens[count] = NULL;

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
      append(history,tokens);
    } else if (strcmp(tokens[0], "pwd") == SUCCESS) {
      char *buf = malloc(512);
      char *result = getcwd(buf, 512);
      if (result == NULL) {
        perror("Error printing directory\n");
        return BUILTIN_ERROR;
      } else {
        printf("%s\n", result);
        return BUILTIN_OK;
      }
      append(history, tokens);
      free(buf);
    } else if (strcmp(tokens[0], "echo") == SUCCESS) {
      int i = 1;
      while (tokens[i] != NULL) {
        printf("%s ", tokens[i++]);
      }
      printf("\n");
      return BUILTIN_OK;
    } else if(strcmp(tokens[0], "history") == 0) {
      for (int i = 0; i < history->len; i++) {
        printf("%s\n", history->history[i]);
      }
      append(history, tokens);
      return BUILTIN_OK;
    } else {
      pid_t pid = fork();
      if (pid == 0) {
        if (execvp(tokens[0], tokens) == ERROR) {
          fprintf(stderr, "%s: command not found\n", tokens[0]);
          exit(BUILTIN_ERROR);
        }
      } else if (pid > 0){
        waitpid(pid, NULL, 0);
        append(history, tokens);
        return BUILTIN_OK;
      } else {
        append(history, tokens);
        perror("fork failed\n");
        return BUILTIN_ERROR;
      }
    }

    return 0;
}
