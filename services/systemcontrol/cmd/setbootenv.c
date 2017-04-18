#include <stdio.h>
#include <stdlib.h>

#include "../ubootenv/Ubootenv.h"

int main(int argc, char *argv[])
{
    if(argc != 3) {
        fprintf(stderr,"usage: setbootenv <key> <value>\n");
        return 1;
    }

    Ubootenv ubootenv = new Ubootenv();
    if (ubootenv->updateValue(argv[1], argv[2])) {
        fprintf(stderr,"could not set boot env\n");
        return 1;
    }

    return 0;
}
