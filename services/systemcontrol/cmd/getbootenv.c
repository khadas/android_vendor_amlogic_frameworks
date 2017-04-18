#include <stdio.h>
#include <stdlib.h>

#include "../ubootenv/Ubootenv.h"

int main(int argc, char *argv[])
{
    Ubootenv ubootenv = new Ubootenv();
    if (argc == 1) {
        list_bootenvs();
    } else {
        //printf("key:%s\n", argv[1]);
        const char* p_value = ubootenv->getValue(argv[1]);
    	if (p_value) {
            printf("%s\n", p_value);
    	}
    }
    return 0;
}
