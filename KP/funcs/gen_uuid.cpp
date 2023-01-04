#include "gen_uuid.h"

char* gen_uuid() {
    char *uuid = static_cast<char *>(malloc(sizeof(char) * 37));
    uuid_t uu;
    uuid_generate_random(uu);
    uuid_unparse(uu, uuid);
    return uuid;
}