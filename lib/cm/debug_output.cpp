
#include <stdio.h>
#include <stdlib.h>
#include "global_setting.h"

const char DEBUG_FNAME[] =  "/users/xuemei/openvswitch-2.3.0/log/debug.txt";
const char WARNING_FNAME[] = "/users/xuemei/openvswitch-2.3.0/log/warning.txt";
const char ERROR_FNAME[] = "/users/xuemei/openvswitch-2.3.0/log/error.txt";

void DEBUG(char* buffer);
void WARNING(char* buffer);
void ERROR(char* buffer);

void DEBUG(const char* buffer) {
    FILE * fp;
    if (!OPEN_DEBUG) {
        return;
    }
    fp = fopen(DEBUG_FNAME, "a+");
    if (fp == NULL) {
        printf("open file failed");
        return;
    }
    fputs(buffer, fp);
    fputc('\n', fp);
    fclose(fp);
}

void WARNING(char* buffer) {
    FILE * fp;
    if (!OPEN_WARNING) {
        return;
    }
    fp = fopen(WARNING_FNAME, "a+");
    fputs(buffer, fp);
    fputc('\r', fp);
    fclose(fp);
}

void ERROR(char* buffer) {
    FILE * fp;
    if (!OPEN_ERROR) {
        return;
    }
    fp = fopen(ERROR_FNAME, "a+");
    fputs(buffer, fp);
    fputc('\r', fp);
    fclose(fp);
}

int main(int argc, char* argv[]) {
    DEBUG("fdsafdsa");
}