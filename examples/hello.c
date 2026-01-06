#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ttykit.h"

int main(void) {
    if (tty_enable_raw_mode() == -1) {
        perror("tty_enable_raw_mode");
        return 1;
    }

    printf("Raw mode enabled. Press 'q' to quit.\r\n");
    printf("Press any key to see its code:\r\n");

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 'q') break;
        printf("Key: %c (0x%02x)\r\n", (c >= 32 && c < 127) ? c : '.', c);
    }

    tty_disable_raw_mode();
    printf("Raw mode disabled.\n");
    return 0;
}
