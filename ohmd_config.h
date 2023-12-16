#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "driverlog.h"

typedef struct {
    int autodetect;
    int hmddisplay_idx;
    int hmdtracker_idx;
    int lcontroller_idx;
    int rcontroller_idx;
} configvalues_t;

/** returns hmddisplay, hmdtracker, leftcontroller, rightcontroller */
void get_configvalues(configvalues_t *cfg) {
    char *home = getenv("HOME");
    char filename[255];
    sprintf(filename, "%s/%s", home, ".ohmd_config.txt");
    FILE* file = fopen(filename, "r");
    if (file) {
        DriverLog("opened config file %s\n", filename);
        char line[256];
        cfg->autodetect = 0;

        while (fgets(line, sizeof(line), file)) {
            char* option = strtok(line, " ");
            char* value = strtok(NULL, " ");
            //printf("%s: %s\n", option, value);
            if (strcmp(option, "autodetect") == 0) {
                cfg->autodetect = strtol(value, NULL, 10);
            }
            if (strcmp(option, "hmddisplay") == 0) {
                cfg->hmddisplay_idx = strtol(value, NULL, 10);
            }
            if (strcmp(option, "hmdtracker") == 0) {
                cfg->hmdtracker_idx = strtol(value, NULL, 10);
            }
            if (strcmp(option, "leftcontroller") == 0) {
                cfg->lcontroller_idx = strtol(value, NULL, 10);
            }
            if (strcmp(option, "rightcontroller") == 0) {
                cfg->rcontroller_idx = strtol(value, NULL, 10);
            }
        }
        fclose(file);
        
    } else {
        DriverLog("could not open config file %s, using default headset 0 with no controller\n", filename);
    }
}

