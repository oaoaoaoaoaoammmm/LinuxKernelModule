#include <stdio.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    char* pid;

    if (argc < 2) {
        fprintf( stderr, "not enough args\n" );
        return -1;
    }
    pid = argv[1];
    printf("pid = %s\n", pid);

    FILE *kmod_args = fopen("/sys/kernel/debug/kmod/kmod_args", "w");
    fprintf(kmod_args, "%s\n ", pid);
    fclose(kmod_args);



    FILE *kmod_result = fopen("/sys/kernel/debug/kmod/kmod_result", "r");
    char c;
    while (fscanf(kmod_result, "%c", &c) != EOF) {
        printf("%c", c);
    }

    fclose(kmod_result);
    return 0;
}