#include <SDL2/SDL.h>

const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *screen = NULL;

struct Ship {
  float x, y, a;
  float vx, vy, va;
};
Ship ship;

void UpdateShip(float dt, bool bLeft, bool bRight) {
  float da = 3.0f;
  float dv = 150.f;
  float fv = 0.02f;
  if (bLeft && !bRight) {
    ship.va = -da;
  } else if (!bLeft && bRight) {
    ship.va = da;
  } else {
    ship.va = 0.0;
  }
  ship.x += ship.vx * dt;
  ship.y += ship.vy * dt;
  ship.vx += cos(ship.a) * dt * dv;
  ship.vy += sin(ship.a) * dt * dv;
  ship.vx *= 1.0f - fv;
  ship.vy *= 1.0f - fv;
  ship.a += ship.va * dt;
  if (ship.x < 0 && ship.vx < 0) {
    ship.vx = -ship.vx;
  }
  if (ship.x > SCREEN_WIDTH && ship.vx > 0) {
    ship.vx = -ship.vx;
  }
  if (ship.y < 0 && ship.vy < 0) {
    ship.vy = -ship.vy;
  }
  if (ship.y > SCREEN_HEIGHT && ship.vy > 0) {
    ship.vy = -ship.vy;
  }
}

void RenderShip() {
  float SHIP_D = 10.f;
  SDL_Rect rect;
  rect.x = ship.x - SHIP_D / 2;
  rect.y = ship.y - SHIP_D / 2;
  rect.w = SHIP_D;
  rect.h = SHIP_D;
  SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderFillRect(renderer, &rect);
  SDL_RenderDrawRect(renderer, &rect);
  SDL_RenderDrawLine(renderer, ship.x, ship.y, ship.x + cos(ship.a) * 20.f,
                     ship.y + sin(ship.a) * 20.f);
}

bool init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return false;
  }
  int r = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT,
                                      SDL_WINDOW_SHOWN, &window, &renderer);
  if (r != 0) {
    return false;
  }
  screen = SDL_GetWindowSurface(window);
  if (screen == NULL) {
    return false;
  }
  return true;
}

bool quit() {
  if (window) {
    SDL_DestroyWindow(window);
  }
  SDL_Quit();
  return true;
}

int main(int argc, char *argv[]) {
  if (!init()) {
    return 1;
  }

  bool q;
  SDL_Event e;
  while (!q) {
    bool bRight = false;
    bool bLeft = false;
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        q = true;
      }
    }

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    UpdateShip(0.016f, state[SDL_SCANCODE_LEFT], state[SDL_SCANCODE_RIGHT]);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    RenderShip();
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  if (!quit()) {
    return 2;
  }

  return 0;
}
