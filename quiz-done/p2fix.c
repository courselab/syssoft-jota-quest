#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  int verified = 0;
  char user_key[10];

  printf("Enter password: ");
  fgets(user_key, sizeof(user_key), stdin);  // replaced scanf with fgets for safe bounded input
  user_key[strcspn(user_key, "\n")] = '\0';  // removes the newline character left by fgets

  if (!strcmp(user_key, "foo"))
    verified = 1;

  if (!verified) {
    printf("Access denied\n");
    exit(1);
  }

  printf("Access granted.\n");
  return 0;
}
