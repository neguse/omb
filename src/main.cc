#include <SDL2/SDL.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* screen = NULL;

bool init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }
    int r = SDL_CreateWindowAndRenderer(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN,
        &window,
        &renderer);
    if (r != 0) {
        return false;
    }
    screen = SDL_GetWindowSurface(window);
    if (screen == NULL) {
        return false;
    }
    return true;
}

bool quit()
{
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    return true;
}

int main(int argc, char* argv[])
{
    if (!init()) {
        return 1;
    }

    bool q;
    SDL_Event e;
    int posx = 0;
    int posy = 0;
    int POS_D = 20;
    while (!q) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                q = true;
            }
            if (e.type == SDL_KEYDOWN) {
                int ksym = e.key.keysym.sym;
                if (ksym == SDLK_LEFT) {
                    posx -= POS_D;
                }
                if (ksym == SDLK_RIGHT) {
                    posx += POS_D;
                }
                if (ksym == SDLK_UP) {
                    posy -= POS_D;
                }
                if (ksym == SDLK_DOWN) {
                    posy += POS_D;
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        SDL_Rect rect;
        rect.x = posx;
        rect.y = posy;
        rect.w = 100;
        rect.h = 100;
        SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
        SDL_RenderFillRect(renderer, &rect);
        SDL_RenderDrawRect(renderer, &rect);
        SDL_RenderPresent(renderer);
    }

    if (!quit()) {
        return 2;
    }

    return 0;
}
