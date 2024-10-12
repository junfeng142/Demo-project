#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void parse_drv(char *path)
{
  char buf[255] = {0};
  int i = 0, nb = 0, fd = open(path, O_RDONLY);

  printf("{\n");
  memset(buf, 0, sizeof(buf));
  read(fd, buf, 32);
  printf("  \"%s\", ", buf);

  memset(buf, 0, sizeof(buf));
  read(fd, buf, 32);
  printf("\"%s\", ", buf);

  memset(buf, 0, sizeof(buf));
  read(fd, buf, 128);
  printf("\"%s\", ", buf);

  memset(buf, 0, sizeof(buf));
  read(fd, buf, 4);
  printf("%d, \n", *((uint32_t *)buf));

  printf("  ");
  for(i = 0; i < 10; i++) {
    memset(buf, 0, sizeof(buf));
    read(fd, buf, 4);
    printf("0x%08x, ", *((uint32_t *)buf));
  }
  memset(buf, 0, sizeof(buf));
  read(fd, buf, 4);
  nb = *((uint32_t *)buf);
  printf("%d, \n", nb);

  for(i = 0; i < nb; i++) {
    memset(buf, 0, sizeof(buf));
    read(fd, buf, 32);
    printf("  \"%s\", ", buf);

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 1);
    printf("%d, ", *buf);

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 4);
    printf("0x%08x, ", *((uint32_t *)buf));

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 4);
    printf("0x%08x, ", *((uint32_t *)buf));

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 4);
    printf("0x%08x, ", *((uint32_t *)buf));

    memset(buf, 0, sizeof(buf));
    read(fd, buf, 4);
    printf("0x%08x, \n", *((uint32_t *)buf));
  }
  printf("},\n");
}

int main(int argc, char **argv)
{
  parse_drv(argv[1]);
  return 0;
}
