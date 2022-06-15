#ifndef GUI_H
#define GUI_H

#define SDL_MAIN_HANDLED
#include "./../../lib/SDL/SDL/include/SDL2/SDL.h"

SDL_Window* GUI_initialiseWindow();
SDL_Surface* GUI_getSurface(SDL_Window *window);

#endif