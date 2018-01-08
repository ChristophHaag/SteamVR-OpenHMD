#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "driverlog.h"

/** returns hmddisplay, hmdtracker, leftcontroller, rightcontroller */
int *get_configvalues() {
    int *vals = (int*) malloc(4*sizeof(int));

    char *home = getenv("HOME");
    char filename[255];
    sprintf(filename, "%s/%s", home, ".ohmd_config.txt");
    FILE* file = fopen(filename, "r");
    if (file) {
        DriverLog("opened config file %s\n", filename);
        char line[256];

        while (fgets(line, sizeof(line), file)) {
            char* option = strtok(line, " ");
            char* value = strtok(NULL, " ");
            //printf("%s: %s\n", option, value);
            if (strcmp(option, "hmddisplay") == 0) {
                vals[0] = strtol(value, NULL, 10);
            }
            if (strcmp(option, "hmdtracker") == 0) {
                vals[1] = strtol(value, NULL, 10);
            }
            if (strcmp(option, "leftcontroller") == 0) {
                vals[2] = strtol(value, NULL, 10);
            }
            if (strcmp(option, "rightcontroller") == 0) {
                vals[3] = strtol(value, NULL, 10);
            }
        }
        fclose(file);
        
    } else {
        DriverLog("could not open config file %s, using default headset 0 with no controller\n", filename);
        vals[0] = 0;
        vals[1] = 0;
        vals[2] = -1;
        vals[3] = -1;
    }
    return vals;
}
