#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "./headers/gui.h"
#include "./headers/common.h"


/*
    Lists files present in the Roms folder, and then returns the selected file
*/
FILE* loadROM() {

    DIR* roms = opendir("./../Roms");
    if (roms == NULL) {
        printf("Please Ensure Roms Folder Exists!\n");
        exit(1);
    }

    struct dirent* item;
    item = readdir(roms);
    uint16_t counter;
    counter = 0;

    struct dirent *files;
    files = (struct dirent*)malloc(sizeof(struct dirent) * (counter + 1));

    while (item != NULL) {
        if (item->d_type == DT_REG) {
            printf("%d %s \n", counter, item->d_name);
            counter++;
            files = realloc(files, sizeof(struct dirent) * (counter + 1));
            files[(counter - 1)] = *item;
        }
        item = readdir(roms);
    }

    printf("Enter the index of the file you would like to load: ");
    int chosen = -1;
    while (chosen < 0 || chosen > counter) {
            scanf("%d", &chosen);
    }
    printf("You have chosen %s\n", files[chosen].d_name);

    FILE *rom;
    char path[280] = "./../Roms/";
    strncat(path, files[chosen].d_name, strlen(files[chosen].d_name));
    rom = fopen(path, "rb");
    free(files);
    
    int blah = 0;
    fread(&blah, sizeof(int), 1, rom);
    return rom;
}


/*
    Entry point into the program
*/
int main() {
    FILE *rom = loadROM();
    void* surface = GUI_getSurface(GUI_initialiseWindow()); 
    struct NES consoleState = {0};
    
    return 0;
}