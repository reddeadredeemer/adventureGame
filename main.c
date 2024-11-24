#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char printSymbol;
    char *flavorText;
    bool isWall;         // True if the tile is a wall
    bool isScenario;
    bool isDiscovered;
} MapTile;

typedef struct {
    int x;
    int y;
} PlayerLocation;

typedef struct {
    MapTile **map;      // Dynamic 2D array of MapTile
    int width;          // Map width
    int height;         // Map height
    PlayerLocation location;
} Map;

void parseMapFile(const char *filename, Map *map) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error opening map file.\n");
        exit(1);
    }

    char line[256];
    int maxWidth = 0;
    char **mapLines = NULL;
    int mapLineCount = 0;
    char **flavorTextLines = NULL;
    int flavorTextCount = 0;
    bool inMap = true;

    // Read the entire file into memory
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline character (handles both LF and CRLF)
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(line, "==========") == 0) {
            inMap = false;
            continue;
        }

        if (inMap) {
            // We are in the map section
            // Store map lines to process later
            mapLines = realloc(mapLines, sizeof(char *) * (mapLineCount + 1));
            mapLines[mapLineCount] = strdup(line);
            int lineWidth = strlen(line);
            if (lineWidth > maxWidth) {
                maxWidth = lineWidth;
            }
            mapLineCount++;
        } else {
            // We are in the flavor text section
            flavorTextLines = realloc(flavorTextLines, sizeof(char *) * (flavorTextCount + 1));
            flavorTextLines[flavorTextCount] = strdup(line);
            flavorTextCount++;
        }
    }
    fclose(fp);

    // Set map dimensions
    map->width = maxWidth;
    map->height = mapLineCount;

    // Allocate memory for the map
    map->map = malloc(sizeof(MapTile *) * map->height);
    for (int i = 0; i < map->height; i++) {
        map->map[i] = malloc(sizeof(MapTile) * map->width);
        memset(map->map[i], 0, sizeof(MapTile) * map->width);
    }

    // Process the stored map lines
    for (int y = 0; y < mapLineCount; y++) {
        char *line = mapLines[y];
        for (int x = 0; x < map->width; x++) {
            char ch = x < strlen(line) ? line[x] : ' ';
            MapTile *tile = &map->map[y][x];
            tile->printSymbol = ch;
            tile->isDiscovered = false;
            tile->flavorText = NULL;
            tile->isWall = false;
            tile->isScenario = false;

            // Determine tile properties based on the character
            if (ch == '|' || ch == '-') {
                tile->isWall = true;
            } else if (ch == '#') {
                tile->isScenario = true;
            } else if (ch == 'S') {
                map->location.x = x;
                map->location.y = y;
            }
            // Other tiles are passable by default
        }
        free(line);
    }
    free(mapLines);

    // Process the stored flavor text lines
    for (int i = 0; i < flavorTextCount; i++) {
        char *line = flavorTextLines[i];

        int x, y;
        char *text = strchr(line, ' ');
        if (text != NULL) {
            *text = '\0';
            text++;
            if (sscanf(line, "%d,%d", &x, &y) == 2) {
                if (x >= 0 && y >= 0 && x < map->width && y < map->height) {
                    map->map[y][x].flavorText = strdup(text);
                }
            }
        }
        free(line);
    }
    free(flavorTextLines);
}

bool isAdjacentToDiscovered(Map *map, int x, int y) {
    // Include diagonals in the adjacency check
    int dx[] = { -1,  0,  1,  0, -1, -1,  1,  1 };
    int dy[] = {  0,  1,  0, -1, -1,  1, -1,  1 };

    for (int dir = 0; dir < 8; dir++) { // Loop through all 8 directions
        int adjX = x + dx[dir];
        int adjY = y + dy[dir];

        if (adjX >= 0 && adjX < map->width && adjY >= 0 && adjY < map->height) {
            if (map->map[adjY][adjX].isDiscovered) {
                return true;
            }
        }
    }
    return false;
}

void printMap(Map *map, bool showAll) {
    bool hasPrintedAnything = false;

    for (int y = 0; y < map->height; y++) {
        bool rowHasVisibleTile = false; // Track if the row has any visible tiles
        for (int x = 0; x < map->width; x++) {
            MapTile *tile = &map->map[y][x];

            if (x == map->location.x && y == map->location.y) {
                printf("@");
                rowHasVisibleTile = true;
            } else if (showAll || tile->isDiscovered) {
                printf("%c", tile->printSymbol);
                rowHasVisibleTile = true;
            } else if (isAdjacentToDiscovered(map, x, y)) {
                if (tile->isWall) {
                    printf("%c", tile->printSymbol);
                } else {
                    printf(".");
                }
                rowHasVisibleTile = true;
            }
        }

        if (rowHasVisibleTile) {
            printf("\n");
            hasPrintedAnything = true;
        }
    }

    if (hasPrintedAnything) {
        printf("\n"); // Add an extra newline after the map output
    }
}

void repl(Map *map) {
    char command[10];

    // Mark the starting position as discovered
    map->map[map->location.y][map->location.x].isDiscovered = true;

    // Print the initial map
    printMap(map, false);

    while (true) {
        printf("\nYou are at (%d, %d).\n", map->location.x, map->location.y);
        printf("Enter a command (north, south, east, west, look, quit): ");
        scanf("%s", command);

        int newX = map->location.x;
        int newY = map->location.y;

        if (strcmp(command, "north") == 0) {
            newY--;
        } else if (strcmp(command, "south") == 0) {
            newY++;
        } else if (strcmp(command, "east") == 0) {
            newX++;
        } else if (strcmp(command, "west") == 0) {
            newX--;
        } else if (strcmp(command, "look") == 0) {
            // Print the map with fog of war
            printMap(map, false);
            continue;
        } else if (strcmp(command, "quit") == 0) {
            break;
        } else {
            printf("Invalid command.\n");
            continue;
        }

        // Check boundaries
        if (newX < 0 || newX >= map->width || newY < 0 || newY >= map->height) {
            printf("You can't move outside the map!\n");
            continue;
        }

        MapTile *tile = &map->map[newY][newX];

        // Check if the target tile is a wall
        if (tile->isWall) {
            printf("There's a wall there!\n");
            printMap(map, false);
            continue;
        }

        // Move player
        map->location.x = newX;
        map->location.y = newY;

        // Mark the tile as discovered
        tile->isDiscovered = true;

        // Print the map showing the fog of war
        printMap(map, false);

        // Print flavor text if available
        if (tile->flavorText) {
            printf("%s", tile->flavorText);
        } else {
            printf("You moved to a new location.");
        }
    }
}

int main() {
    Map map;
    memset(&map, 0, sizeof(Map));

    parseMapFile("map.txt", &map);

    printf("Welcome to the Maze Game!\n");
    repl(&map);

    // Free allocated flavor texts and map memory
    for (int y = 0; y < map.height; y++) {
        for (int x = 0; x < map.width; x++) {
            free(map.map[y][x].flavorText);
        }
        free(map.map[y]);
    }
    free(map.map);

    return 0;
}
