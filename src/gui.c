#include <stdio.h>

#include "./../lib/SDL/SDL/include/SDL2/SDL.h"

#define SDL_MAIN_HANDLED
#define Width 256
#define Height 240
#define Scale 3


/*
    SDL Helper Functions
*/
SDL_Window* GUI_initialiseWindow() {

    SDL_Window *window = NULL;
    if (SDL_Init(SDL_INIT_VIDEO) > 0) {
        printf("%s\n", SDL_GetError());
        exit(1);
    }
    else {
        window = SDL_CreateWindow("Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width * Scale, Height * Scale, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("%s\n", SDL_GetError());
            exit(1);
        }
        return window;
    }
}

SDL_Surface* GUI_getSurface(SDL_Window *window) {
    return SDL_GetWindowSurface(window);
}

void GUI_closeWindow(SDL_Window* window) {
    SDL_DestroyWindow(window);
}

void GUI_stopSDL() {
    SDL_Quit(); 
}


/*

*/