
#define MasterProgramsLen 3
const unsigned char *MasterPrograms[] = {
    "l steps 1\0d 38000\0t 440 5\0\0",
    "t play\0d 30000\0\0",
    "t 500 2\0d 30000\0run 2\0\0",
    "t keys\0d 500\0run keys\0\0",
    "serial on\0d 50\0serial off\0\0"
};
