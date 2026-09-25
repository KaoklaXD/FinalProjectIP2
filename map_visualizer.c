#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "map_visualizer.h"

#define NUM_ROWS 29
#define ROW_WIDTH 160

/* Pure ASCII template for Level 3 (Third Floor) */
static const char* BASE_MAP_L3[NUM_ROWS] = {
    "  ____________________________________________________________                    ____________________________________________________________",
    " |                                                            |                  |                                                            |",
    " |  [ 4stair ] --- [  343  ] --- [  342  ] --- [ 3wing4 ]     |                  |     [ 3wing2 ] --- [  322  ] --- [  323  ] --- [  324  ]   |",
    " |                                                      \\     |__________________|     /                                                      |",
    " |                                                       \\                            /                                                       |",
    " |                           WING 4                       \\   [   arc   ] -- [  lift3  ] /                                WING 2                  |",
    " |                                                         \\         ||             /                                                         |",
    " |                                                          \\        ||            /                                                          |",
    " |                                                           \\       ||           /                                                           |",
    " |                                                            \\      ||          /                                                            |",
    " |____________________________________________________________ \\_____||_________/ ____________________________________________________________|",
    "                                                              |      ||         |",
    "                                                              |      ||         |",
    "                                                              |      ||         |",
    "                                                              |      ||         |",
    "                                                              |     /  \\        |",
    "                                                              |    /    \\       |",
    "                                                              |   /      \\      |",
    "  ____________________________________________________________|__/________\\_____|___________________________________________________________",
    " |                                                              /          \\                                                                 |",
    " |                                                             /            \\                                                                |",
    " |  [ 3stair ] --- [  332  ] --- [  331  ] -------------------/              \\-------------------- [  312  ] --- [  313  ] --- [ 1stair ]    |",
    " |                                                                                                                                           |",
    " |                           WING 3                                                            WING 1                                        |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |___________________________________________________________________________________________________________________________________________|"
};

/* Pure ASCII template for Level 2 (Second Floor) */
static const char* BASE_MAP_L2[NUM_ROWS] = {
    "  ____________________________________________________________                    ____________________________________________________________",
    " |                                                            |                  |                                                            |",
    " |  [ 4stair ] --- [  243  ] --- [  242  ] --- [ 2wing4 ]     |                  |     [ 2wing2 ] --- [  222  ] --- [  223  ] --- [  224  ]   |",
    " |                                                      \\     |__________________|     /                                                      |",
    " |                                                       \\   [  atrium ] -- [  lift2  ]                                                       |",
    " |                           WING 4                       \\         \\        /        /                               WING 2                  |",
    " |                                                         \\        [  entry  ]      /                                                        |",
    " |                                                          \\            ||         /                                                         |",
    " |                                                           \\           ||        /                                                          |",
    " |                                                            \\          ||       /                                                           |",
    " |____________________________________________________________ \\_________||______/ ___________________________________________________________|",
    "                                                              |          ||     |",
    "                                                              |          ||     |",
    "                                                              |          ||     |",
    "                                                              |          ||     |",
    "                                                              |         /  \\    |",
    "                                                              |        /    \\   |",
    "                                                              |       /      \\  |",
    "  ____________________________________________________________|______/__      \\_|___________________________________________________________",
    " |                                                                  /          \\                                                             |",
    " |                                                                 /            \\                                                            |",
    " |  [ 3stair ] --- [  233  ] --- [  232  ] <----------------------/              \\---> [  audi2  ] --- [  212  ] --- [  213  ] --- [ 1stair ] |",
    " |                                                                                                                                           |",
    " |                           WING 3                                                            WING 1                                        |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |___________________________________________________________________________________________________________________________________________|"
};

/* Pure ASCII template for Level 1 (First Floor) */
static const char* BASE_MAP_L1[NUM_ROWS] = {
    "  ____________________________________________________________                    ____________________________________________________________",
    " |                                                            |                  |                                                            |",
    " |  [ 4stair ] --- [ 143art ] --- [  142  ] --- [  141  ] --+ |                  |                  [  122  ] --- [  123  ]                   |",
    " |                                  |                   |     |__________________|                  /                                         |",
    " |                                  |                   +--------- [  atrium ] -- [  lift1  ]       /                                          |",
    " |                           WING 4 |                                 ||            |             /                   WING 2                  |",
    " |                                  |                           [ 103dance] -- [pingpong] <------+                                            |",
    " |                                  |                                               ||                                                        |",
    " |                                  |                                           [ ground ] ---+                                               |",
    " |                                  |                                               ||        |                                               |",
    " |__________________________________|_________________________                      ||________|_______________________________________________|",
    "                                    |                         |                     ||        | |",
    "                                [ taobin ] --- [canteen]      |                     ||        | |",
    "                                    |                         |                     ||        | |",
    "                                [bathrm]                      |                     ||        | |",
    "                                    |                         |                    /  \\       | |",
    "                                    |                         |                   /    \\      | |",
    "                                    |                         |                  /      \\     | |",
    "  __________________________________|_________________________|_________________/________\\____|_|___________________________________________",
    " |                                  |                                          /          \\   |                                              |",
    " |                                  |                                         /            \\  |                                              |",
    " |  [ 3stair ] <--------------------+                        /              \\ +---> [  audi1  ] - [112dream] - [  113  ] - [ 1stair ]        |",
    " |                                                                                                                                           |",
    " |                           WING 3                                                            WING 1                                        |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |                                                                                                                                           |",
    " |___________________________________________________________________________________________________________________________________________|"
};

typedef struct NodeBoxDef {
    const char* code;
    const char* base_str;
    const char* star_str;
    const char* plus_str;
    const char* minus_str;
} NodeBoxDef;

static const NodeBoxDef NODE_BOXES[] = {
    {"4stair",   "[ 4stair ]",  "[*4stair*]",  "[+4stair+]",  "[-4stair-]"},
    {"3stair",   "[ 3stair ]",  "[*3stair*]",  "[+3stair+]",  "[-3stair-]"},
    {"1stair",   "[ 1stair ]",  "[*1stair*]",  "[+1stair+]",  "[-1stair-]"},
    {"lift3",    "[  lift3  ]",  "[* lift3 *]",  "[+ lift3 +]",  "[- lift3 -]"},
    {"lift2",    "[  lift2  ]",  "[* lift2 *]",  "[+ lift2 +]",  "[- lift2 -]"},
    {"lift1",    "[  lift1  ]",  "[* lift1 *]",  "[+ lift1 +]",  "[- lift1 -]"},
    {"arc",      "[   arc   ]",  "[*  arc  *]",  "[+  arc  +]",  "[-  arc  -]"},
    {"entry",    "[  entry  ]",  "[* entry *]",  "[+ entry +]",  "[- entry -]"},
    {"atrium",   "[  atrium ]",  "[* atrium*]",  "[+ atrium+]",  "[- atrium-]"},
    {"3wing4",   "[ 3wing4 ]",  "[*3wing4*]",  "[+3wing4+]",  "[-3wing4-]"},
    {"3wing2",   "[ 3wing2 ]",  "[*3wing2*]",  "[+3wing2+]",  "[-3wing2-]"},
    {"2wing4",   "[ 2wing4 ]",  "[*2wing4*]",  "[+2wing4+]",  "[-2wing4-]"},
    {"2wing2",   "[ 2wing2 ]",  "[*2wing2*]",  "[+2wing2+]",  "[-2wing2-]"},
    {"143art",   "[ 143art ]",  "[*143art*]",  "[+143art+]",  "[-143art-]"},
    {"112dream", "[112dream]",  "[*112drm*]",  "[+112drm+]",  "[-112drm-]"},
    {"103dance", "[ 103dance]", "[*103dnc* ]", "[+103dnc+ ]", "[-103dnc- ]"},
    {"pingpong", "[pingpong]",  "[*p-pong*]",  "[+p-pong+]",  "[-p-pong-]"},
    {"ground",   "[ ground ]",  "[*ground*]",  "[+ground+]",  "[-ground-]"},
    {"audi1",    "[  audi1  ]",  "[* audi1 *]",  "[+ audi1 +]",  "[- audi1 -]"},
    {"audi2",    "[  audi2  ]",  "[* audi2 *]",  "[+ audi2 +]",  "[- audi2 -]"},
    {"taobin",   "[ taobin ]",  "[*taobin*]",  "[+taobin+]",  "[-taobin-]"},
    {"canteen",  "[canteen]",   "[*cantn*]",   "[+cantn+]",   "[-cantn-]"},
    {"bathrm",   "[bathrm]",    "[*bath*]",    "[+bath+]",    "[-bath-]"},
    {"343",      "[  343  ]",   "[* 343 *]",   "[+ 343 +]",   "[- 343 -]"},
    {"342",      "[  342  ]",   "[* 342 *]",   "[+ 342 +]",   "[- 342 -]"},
    {"322",      "[  322  ]",   "[* 322 *]",   "[+ 322 +]",   "[- 322 -]"},
    {"323",      "[  323  ]",   "[* 323 *]",   "[+ 323 +]",   "[- 323 -]"},
    {"324",      "[  324  ]",   "[* 324 *]",   "[+ 324 +]",   "[- 324 -]"},
    {"331",      "[  331  ]",   "[* 331 *]",   "[+ 331 +]",   "[- 331 -]"},
    {"332",      "[  332  ]",   "[* 332 *]",   "[+ 332 +]",   "[- 332 -]"},
    {"312",      "[  312  ]",   "[* 312 *]",   "[+ 312 +]",   "[- 312 -]"},
    {"313",      "[  313  ]",   "[* 313 *]",   "[+ 313 +]",   "[- 313 -]"},
    {"243",      "[  243  ]",   "[* 243 *]",   "[+ 243 +]",   "[- 243 -]"},
    {"242",      "[  242  ]",   "[* 242 *]",   "[+ 242 +]",   "[- 242 -]"},
    {"222",      "[  222  ]",   "[* 222 *]",   "[+ 222 +]",   "[- 222 -]"},
    {"223",      "[  223  ]",   "[* 223 *]",   "[+ 223 +]",   "[- 223 -]"},
    {"224",      "[  224  ]",   "[* 224 *]",   "[+ 224 +]",   "[- 224 -]"},
    {"232",      "[  232  ]",   "[* 232 *]",   "[+ 232 +]",   "[- 232 -]"},
    {"233",      "[  233  ]",   "[* 233 *]",   "[+ 233 +]",   "[- 233 -]"},
    {"212",      "[  212  ]",   "[* 212 *]",   "[+ 212 +]",   "[- 212 -]"},
    {"213",      "[  213  ]",   "[* 213 *]",   "[+ 213 +]",   "[- 213 -]"},
    {"142",      "[  142  ]",   "[* 142 *]",   "[+ 142 +]",   "[- 142 -]"},
    {"141",      "[  141  ]",   "[* 141 *]",   "[+ 141 +]",   "[- 141 -]"},
    {"122",      "[  122  ]",   "[* 122 *]",   "[+ 122 +]",   "[- 122 -]"},
    {"123",      "[  123  ]",   "[* 123 *]",   "[+ 123 +]",   "[- 123 -]"},
    {"113",      "[  113  ]",   "[* 113 *]",   "[+ 113 +]",   "[- 113 -]"},
    {NULL, NULL, NULL, NULL, NULL}
};

const char* get_canonical_node_code(const char* name, int room_num) {
    if (!name) return "";
    char lower[128];
    strncpy(lower, name, sizeof(lower) - 1);
    lower[sizeof(lower) - 1] = '\0';
    for (int i = 0; lower[i]; i++) lower[i] = (char)tolower((unsigned char)lower[i]);

    if (strstr(lower, "4stair")) return "4stair";
    if (strstr(lower, "3stair")) return "3stair";
    if (strstr(lower, "1stair")) return "1stair";
    if (strstr(lower, "lift")) return "lift";
    if (strstr(lower, "entry")) return "entry";
    if (strstr(lower, "atrium")) return "atrium";
    if (strstr(lower, "143") || strstr(lower, "art")) return "143art";
    if (strstr(lower, "112") || strstr(lower, "dream")) return "112dream";
    if (strstr(lower, "103") || strstr(lower, "dance")) return "103dance";
    if (strstr(lower, "122") || strstr(lower, "robot")) return "122";
    if (strstr(lower, "212") || strstr(lower, "temp1")) return "212";
    if (strstr(lower, "312") || strstr(lower, "temp2")) return "312";
    if (strstr(lower, "313") || strstr(lower, "tintin")) return "313";
    if (strstr(lower, "bath")) return "bathrm";

    static const char* specific_codes[] = {
        "343","342","3wing4","arc","3wing2","322","323","324","331","332",
        "243","242","2wing4","2wing2","222","223","224","232","233","audi2","213",
        "142","141","123","taobin","canteen","pingpong","ground","audi1","113", NULL
    };
    for (int i = 0; specific_codes[i]; i++) {
        if (strstr(lower, specific_codes[i])) return specific_codes[i];
    }

    if (room_num > 0) {
        char rstr[16];
        snprintf(rstr, sizeof(rstr), "%d", room_num);
        for (int i = 0; specific_codes[i]; i++) {
            if (strcmp(rstr, specific_codes[i]) == 0) return specific_codes[i];
        }
    }

    return name;
}

int get_node_floor_level(const char* name, int room_num) {
    if (!name) return 1;
    char lower[128];
    strncpy(lower, name, sizeof(lower) - 1);
    lower[sizeof(lower) - 1] = '\0';
    for (int i = 0; lower[i]; i++) lower[i] = (char)tolower((unsigned char)lower[i]);

    if (strstr(lower, "(l1)")) return 1;
    if (strstr(lower, "(l2)")) return 2;
    if (strstr(lower, "(l3)")) return 3;

    if (room_num > 0) {
        int f = room_num / 100;
        if (f >= 1 && f <= 3) return f;
    }

    const char* code = get_canonical_node_code(name, room_num);

    if (code[0] == '1' && strlen(code) >= 3 && isdigit((unsigned char)code[1])) return 1;
    if (code[0] == '2' && strlen(code) >= 3 && isdigit((unsigned char)code[1])) return 2;
    if (code[0] == '3' && strlen(code) >= 3 && isdigit((unsigned char)code[1])) return 3;

    if (strcmp(code, "arc") == 0 || strcmp(code, "3wing2") == 0 || strcmp(code, "3wing4") == 0) return 3;
    if (strcmp(code, "entry") == 0 || strcmp(code, "audi2") == 0 || strcmp(code, "2wing2") == 0 || strcmp(code, "2wing4") == 0) return 2;
    if (strcmp(code, "ground") == 0 || strcmp(code, "pingpong") == 0 || strcmp(code, "audi1") == 0 ||
        strcmp(code, "taobin") == 0 || strcmp(code, "canteen") == 0 || strcmp(code, "bathrm") == 0 ||
        strcmp(code, "atrium") == 0) return 1;

    return 1;
}

static void replace_exact(char* line, const char* target, const char* rep) {
    if (!line || !target || !rep) return;
    char* pos = strstr(line, target);
    if (pos) {
        size_t len_rep = strlen(rep);
        memcpy(pos, rep, len_rep);
    }
}

void display_floor_map(int floor_num) {
    if (floor_num < 1 || floor_num > 3) floor_num = 1;
    const char** base = (floor_num == 3) ? BASE_MAP_L3 : (floor_num == 2 ? BASE_MAP_L2 : BASE_MAP_L1);

    printf("\n==============================================================================================================================================\n");
    printf("                                                     LEVEL %d CAMPUS FLOOR MAP OVERVIEW                                                      \n", floor_num);
    printf("==============================================================================================================================================\n");
    for (int r = 0; r < NUM_ROWS; r++) {
        printf("%s\n", base[r]);
    }
    printf("==============================================================================================================================================\n\n");
}

void display_all_floor_maps(void) {
    display_floor_map(3);
    display_floor_map(2);
    display_floor_map(1);
}

static void update_corridors_for_edge(char grid[NUM_ROWS][ROW_WIDTH], int floor_num, const char* code_a, const char* code_b) {
    char pair1[64], pair2[64];
    snprintf(pair1, sizeof(pair1), "%s-%s", code_a, code_b);
    snprintf(pair2, sizeof(pair2), "%s-%s", code_b, code_a);

    #define IS_EDGE(x, y) (strcmp(pair1, (x "-" y)) == 0 || strcmp(pair2, (x "-" y)) == 0)

    if (floor_num == 3) {
        if (IS_EDGE("4stair", "343")) {
            replace_exact(grid[2], " --- [  343  ]", " *** [  343  ]");
        } else if (IS_EDGE("343", "342")) {
            replace_exact(grid[2], "[  343  ] --- [  342  ]", "[  343  ] *** [  342  ]");
        } else if (IS_EDGE("342", "3wing4")) {
            replace_exact(grid[2], "[  342  ] --- [ 3wing4 ]", "[  342  ] *** [ 3wing4 ]");
        } else if (IS_EDGE("3wing4", "arc")) {
            replace_exact(grid[3], "\\     |", "*     |");
            replace_exact(grid[4], "\\                            /", "*                            /");
            replace_exact(grid[5], "\\   [", "*   [");
        } else if (IS_EDGE("arc", "lift")) {
            replace_exact(grid[5], "-- [  lift3  ]", "** [  lift3  ]");
        } else if (IS_EDGE("arc", "3wing2")) {
            replace_exact(grid[3], "|     /", "|     *");
            replace_exact(grid[4], "     /", "     *");
            replace_exact(grid[5], "] /", "] *");
        } else if (IS_EDGE("3wing2", "322")) {
            replace_exact(grid[2], "[ 3wing2 ] --- [  322  ]", "[ 3wing2 ] *** [  322  ]");
        } else if (IS_EDGE("322", "323")) {
            replace_exact(grid[2], "[  322  ] --- [  323  ]", "[  322  ] *** [  323  ]");
        } else if (IS_EDGE("323", "324")) {
            replace_exact(grid[2], "[  323  ] --- [  324  ]", "[  323  ] *** [  324  ]");
        } else if (IS_EDGE("arc", "331")) {
            for (int r = 6; r <= 14; r++) replace_exact(grid[r], "||", "**");
            for (int r = 15; r <= 17; r++) replace_exact(grid[r], "/  \\", "*  \\");
            replace_exact(grid[18], "|__/________\\", "|__*________\\");
            replace_exact(grid[19], "/          \\", "*          \\");
            replace_exact(grid[20], "/            \\", "*            \\");
            replace_exact(grid[21], "-------------------/", "*******************/");
        } else if (IS_EDGE("331", "332")) {
            replace_exact(grid[21], "[  332  ] --- [  331  ]", "[  332  ] *** [  331  ]");
        } else if (IS_EDGE("332", "3stair")) {
            replace_exact(grid[21], " --- [  332  ]", " *** [  332  ]");
        } else if (IS_EDGE("arc", "312")) {
            for (int r = 6; r <= 14; r++) replace_exact(grid[r], "||", "**");
            for (int r = 15; r <= 17; r++) replace_exact(grid[r], "/  \\", "/  *");
            replace_exact(grid[18], "|__/________\\", "|__/________*");
            replace_exact(grid[19], "/          \\", "/          *");
            replace_exact(grid[20], "/            \\", "/            *");
            replace_exact(grid[21], "\\--------------------", "\\********************");
        } else if (IS_EDGE("312", "313")) {
            replace_exact(grid[21], "[  312  ] --- [  313  ]", "[  312  ] *** [  313  ]");
        } else if (IS_EDGE("313", "1stair")) {
            replace_exact(grid[21], "[  313  ] --- ", "[  313  ] *** ");
        }
    } else if (floor_num == 2) {
        if (IS_EDGE("4stair", "243")) {
            replace_exact(grid[2], " --- [  243  ]", " *** [  243  ]");
        } else if (IS_EDGE("243", "242")) {
            replace_exact(grid[2], "[  243  ] --- [  242  ]", "[  243  ] *** [  242  ]");
        } else if (IS_EDGE("242", "2wing4")) {
            replace_exact(grid[2], "[  242  ] --- [ 2wing4 ]", "[  242  ] *** [ 2wing4 ]");
        } else if (IS_EDGE("2wing4", "entry")) {
            replace_exact(grid[3], "\\     |", "*     |");
            replace_exact(grid[4], "\\   [", "*   [");
            replace_exact(grid[5], "\\         \\", "*         \\");
            replace_exact(grid[6], "\\        [", "*        [");
        } else if (IS_EDGE("entry", "lift")) {
            replace_exact(grid[5], "/        /", "*        /");
            replace_exact(grid[4], "-- [  lift2  ]", "** [  lift2  ]");
        } else if (IS_EDGE("entry", "atrium")) {
            replace_exact(grid[5], "\\        /", "*        /");
        } else if (IS_EDGE("entry", "2wing2")) {
            replace_exact(grid[5], "/        /                               WING 2", "/        *                               WING 2");
            replace_exact(grid[4], "    /", "    *");
            replace_exact(grid[3], "|     /", "|     *");
        } else if (IS_EDGE("2wing2", "222")) {
            replace_exact(grid[2], "[ 2wing2 ] --- [  222  ]", "[ 2wing2 ] *** [  222  ]");
        } else if (IS_EDGE("222", "223")) {
            replace_exact(grid[2], "[  222  ] --- [  223  ]", "[  222  ] *** [  223  ]");
        } else if (IS_EDGE("223", "224")) {
            replace_exact(grid[2], "[  223  ] --- [  224  ]", "[  223  ] *** [  224  ]");
        } else if (IS_EDGE("entry", "232")) {
            for (int r = 7; r <= 14; r++) replace_exact(grid[r], "||", "**");
            for (int r = 15; r <= 17; r++) replace_exact(grid[r], "/  \\", "*  \\");
            replace_exact(grid[18], "|______/__", "|______*__");
            replace_exact(grid[19], "    /     ", "    *     ");
            replace_exact(grid[20], "   /      ", "   *      ");
            replace_exact(grid[21], "<----------------------/", "<**********************/");
        } else if (IS_EDGE("232", "233")) {
            replace_exact(grid[21], "[  233  ] --- [  232  ]", "[  233  ] *** [  232  ]");
        } else if (IS_EDGE("233", "3stair")) {
            replace_exact(grid[21], " --- [  233  ]", " *** [  233  ]");
        } else if (IS_EDGE("entry", "audi2")) {
            for (int r = 7; r <= 14; r++) replace_exact(grid[r], "||", "**");
            for (int r = 15; r <= 17; r++) replace_exact(grid[r], "/  \\", "/  *");
            replace_exact(grid[18], "\\_|", "*_|");
            replace_exact(grid[19], "     \\", "     *");
            replace_exact(grid[20], "      \\", "      *");
            replace_exact(grid[21], "\\---> [  audi2  ]", "\\***> [  audi2  ]");
        } else if (IS_EDGE("audi2", "212")) {
            replace_exact(grid[21], "[  audi2  ] --- [  212  ]", "[  audi2  ] *** [  212  ]");
        } else if (IS_EDGE("212", "213")) {
            replace_exact(grid[21], "[  212  ] --- [  213  ]", "[  212  ] *** [  213  ]");
        } else if (IS_EDGE("213", "1stair")) {
            replace_exact(grid[21], "[  213  ] --- ", "[  213  ] *** ");
        }
    } else if (floor_num == 1) {
        if (IS_EDGE("4stair", "143art")) {
            replace_exact(grid[2], " --- [ 143art ]", " *** [ 143art ]");
        } else if (IS_EDGE("143art", "142")) {
            replace_exact(grid[2], "[ 143art ] --- [  142  ]", "[ 143art ] *** [  142  ]");
        } else if (IS_EDGE("142", "141")) {
            replace_exact(grid[2], "[  142  ] --- [  141  ]", "[  142  ] *** [  141  ]");
        } else if (IS_EDGE("141", "atrium")) {
            replace_exact(grid[2], "--+ |", "**+ |");
            replace_exact(grid[3], "|     |", "*     |");
            replace_exact(grid[4], "+--------- [  atrium ]", "+********* [  atrium ]");
        } else if (IS_EDGE("142", "taobin")) {
            for (int r = 3; r <= 11; r++) {
                replace_exact(grid[r], "                                    |", "                                    *");
            }
        } else if (IS_EDGE("taobin", "canteen")) {
            replace_exact(grid[12], "[ taobin ] --- [canteen]", "[ taobin ] *** [canteen]");
        } else if (IS_EDGE("taobin", "bathrm")) {
            replace_exact(grid[13], "                                    |", "                                    *");
        } else if (IS_EDGE("bathrm", "3stair")) {
            for (int r = 15; r <= 20; r++) {
                replace_exact(grid[r], "                                    |", "                                    *");
            }
            replace_exact(grid[21], "<--------------------+", "<********************+");
        } else if (IS_EDGE("atrium", "lift")) {
            replace_exact(grid[4], "-- [  lift1  ]", "** [  lift1  ]");
        } else if (IS_EDGE("atrium", "103dance")) {
            replace_exact(grid[5], "                                 ||", "                                 **");
        } else if (IS_EDGE("103dance", "pingpong")) {
            replace_exact(grid[6], "[ 103dance] -- [pingpong]", "[ 103dance] ** [pingpong]");
        } else if (IS_EDGE("pingpong", "122")) {
            replace_exact(grid[6], "<------+", "<******+");
            replace_exact(grid[5], "/                   WING 2", "*                   WING 2");
            replace_exact(grid[4], "      / ", "      * ");
            replace_exact(grid[3], "                  /", "                  *");
        } else if (IS_EDGE("122", "123")) {
            replace_exact(grid[2], "[  122  ] --- [  123  ]", "[  122  ] *** [  123  ]");
        } else if (IS_EDGE("pingpong", "ground")) {
            replace_exact(grid[7], "                                               ||", "                                               **");
        } else if (IS_EDGE("ground", "audi1")) {
            replace_exact(grid[8], "---+", "***+");
            for (int r = 9; r <= 20; r++) {
                replace_exact(grid[r], "| |", "* |");
            }
            replace_exact(grid[21], "+---> [  audi1  ]", "+***> [  audi1  ]");
        } else if (IS_EDGE("audi1", "112dream")) {
            replace_exact(grid[21], "[  audi1  ] - [112dream]", "[  audi1  ] * [112dream]");
        } else if (IS_EDGE("112dream", "113")) {
            replace_exact(grid[21], "[112dream] - [  113  ]", "[112dream] * [  113  ]");
        } else if (IS_EDGE("113", "1stair")) {
            replace_exact(grid[21], "[  113  ] - [ 1stair ]", "[  113  ] * [ 1stair ]");
        }
    }
}

void visualize_floor_path(Graph* g, const Path* path, int floor_num) {
    if (!g || !path || path->node_count <= 0) {
        display_floor_map(floor_num);
        return;
    }

    if (floor_num < 1 || floor_num > 3) floor_num = 1;
    const char** base = (floor_num == 3) ? BASE_MAP_L3 : (floor_num == 2 ? BASE_MAP_L2 : BASE_MAP_L1);

    char grid[NUM_ROWS][ROW_WIDTH];
    for (int r = 0; r < NUM_ROWS; r++) {
        strncpy(grid[r], base[r], ROW_WIDTH - 1);
        grid[r][ROW_WIDTH - 1] = '\0';
    }

    int nfloors[MAX_CHECKPOINTS];
    const char* ncodes[MAX_CHECKPOINTS];
    char nsymbols[MAX_CHECKPOINTS];

    for (int i = 0; i < path->node_count; i++) {
        int nid = path->nodes[i];
        const char* name = g->nodes[nid].name;
        int rnum = g->nodes[nid].room_num;
        nfloors[i] = get_node_floor_level(name, rnum);
        ncodes[i] = get_canonical_node_code(name, rnum);
    }

    for (int i = 0; i < path->node_count; i++) {
        int f_curr = nfloors[i];
        int f_prev = (i > 0) ? nfloors[i - 1] : f_curr;
        int f_next = (i < path->node_count - 1) ? nfloors[i + 1] : f_curr;

        if (f_curr < f_next || f_prev < f_curr) {
            nsymbols[i] = '+'; // UP
        } else if (f_curr > f_next || f_prev > f_curr) {
            nsymbols[i] = '-'; // DOWN
        } else {
            nsymbols[i] = '*'; // Walking on same floor
        }
    }

    // 1. Update corridors between adjacent nodes on this floor
    for (int i = 0; i < path->node_count - 1; i++) {
        if (nfloors[i] == floor_num && nfloors[i + 1] == floor_num) {
            update_corridors_for_edge(grid, floor_num, ncodes[i], ncodes[i + 1]);
        }
    }

    // 2. Update node box representations for nodes on this floor
    for (int i = 0; i < path->node_count; i++) {
        if (nfloors[i] == floor_num) {
            const char* code = ncodes[i];
            char sym = nsymbols[i];

            for (int b = 0; NODE_BOXES[b].code != NULL; b++) {
                if (strcmp(NODE_BOXES[b].code, code) == 0) {
                    const char* rep = (sym == '+') ? NODE_BOXES[b].plus_str :
                                      ((sym == '-') ? NODE_BOXES[b].minus_str : NODE_BOXES[b].star_str);

                    for (int r = 0; r < NUM_ROWS; r++) {
                        replace_exact(grid[r], NODE_BOXES[b].base_str, rep);
                    }
                    break;
                }
            }
        }
    }

    printf("\n==============================================================================================================================================\n");
    printf("                                                  LEVEL %d (FLOOR %d) - PATH NAVIGATION VISUALIZATION                                          \n", floor_num, floor_num);
    printf("==============================================================================================================================================\n");
    for (int r = 0; r < NUM_ROWS; r++) {
        printf("%s\n", grid[r]);
    }
    printf("==============================================================================================================================================\n");
    printf(" Map Legend: [*] Walking path on this floor   [+] Take Stairs/Lift UP   [-] Take Stairs/Lift DOWN\n");
    printf("----------------------------------------------------------------------------------------------------------------------------------------------\n");

    printf(" Navigation on Level %d: ", floor_num);
    int first_on_floor = 1;
    for (int i = 0; i < path->node_count; i++) {
        if (nfloors[i] == floor_num) {
            int nid = path->nodes[i];
            char sym = nsymbols[i];
            const char* trans = "";
            if (sym == '+') trans = " (UP [+])";
            else if (sym == '-') trans = " (DOWN [-])";

            printf("%s[%s]%s", (first_on_floor ? "" : " ──> "), g->nodes[nid].name, trans);
            first_on_floor = 0;
        }
    }
    printf("\n==============================================================================================================================================\n\n");
}

void visualize_path(Graph* g, const Path* path) {
    if (!g || !path || path->node_count <= 0) {
        printf("[Notice] No path route available to visualize.\n");
        return;
    }

    int visited_floors[MAX_CHECKPOINTS];
    int num_visited = 0;

    for (int i = 0; i < path->node_count; i++) {
        int nid = path->nodes[i];
        int f = get_node_floor_level(g->nodes[nid].name, g->nodes[nid].room_num);
        if (num_visited == 0 || visited_floors[num_visited - 1] != f) {
            visited_floors[num_visited++] = f;
        }
    }

    printf("\n========================================================================================\n");
    printf("                WALKING PATH VISUALIZATION: TOTAL DISTANCE %.1f m           \n", path->total_distance);
    printf("========================================================================================\n");
    printf(" Route Summary (%d waypoints across %d floor transition%s):\n",
           path->node_count, num_visited - 1, (num_visited - 1 == 1 ? "" : "s"));
    printf(" ");
    for (int i = 0; i < path->node_count; i++) {
        int nid = path->nodes[i];
        printf("[%s]%s", g->nodes[nid].name, (i < path->node_count - 1 ? " ──> " : "\n"));
    }

    for (int v = 0; v < num_visited; v++) {
        visualize_floor_path(g, path, visited_floors[v]);
    }
}
