#if USE_SDL
#include <SDL2/SDL.h>
#endif

#include <math.h>
#include <vector>
#include <errno.h>

const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;

#if USE_SDL
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *screen = NULL;
#endif

enum Mode { Server, Client };
Mode mode = Client;

struct ClientData {
  int sock;
};

struct Ship {
  float x, y, a;
  float vx, vy, va;
  bool bLeft, bRight, bShot, bAccel;
  int sock;
};
struct Bullet {
  float x, y;
  float vx, vy;
  float lifetime;
};
std::vector<Ship *> ships;
std::vector<Bullet *> bullets;

#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
extern "C" {
#include <ae.h>
#include <anet.h>
}
#include <cstdio>
#include <unistd.h>
#include <cstring>

static const int TIMER_INTERVAL_MS = 16;

char err[ANET_ERR_LEN];

bool init() {
#if USE_SDL
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
#else
  return true;
#endif
}

bool quit() {
#if USE_SDL
  if (window) {
    SDL_DestroyWindow(window);
  }
  SDL_Quit();
  return true;
#else
  return true;
#endif
}

void MakeBullet(Ship *pShip) {
  Bullet *pBullet = new Bullet();
  float BULLET_VD = 200.0f;
  float BULLET_LIFETIME = 3.0f;
  pBullet->x = pShip->x;
  pBullet->y = pShip->y;
  pBullet->vx = cos(pShip->a) * BULLET_VD;
  pBullet->vy = sin(pShip->a) * BULLET_VD;
  pBullet->lifetime = BULLET_LIFETIME;
  bullets.push_back(pBullet);
}

void UpdateShip(Ship *pShip, float dt) {
  float da = 3.0f;
  float dv = pShip->bAccel ? 150.f : 0.0f;
  float fv = 0.02f;
  if (pShip->bLeft && !pShip->bRight) {
    pShip->va = -da;
  } else if (!pShip->bLeft && pShip->bRight) {
    pShip->va = da;
  } else {
    pShip->va = 0.0;
  }
  if (pShip->bShot) {
    MakeBullet(pShip);
  }
  pShip->x += pShip->vx * dt;
  pShip->y += pShip->vy * dt;
  pShip->vx += cos(pShip->a) * dt * dv;
  pShip->vy += sin(pShip->a) * dt * dv;
  pShip->vx *= 1.0f - fv;
  pShip->vy *= 1.0f - fv;
  pShip->a += pShip->va * dt;
  if (pShip->x < 0 && pShip->vx < 0) {
    pShip->vx = -pShip->vx;
  }
  if (pShip->x > SCREEN_WIDTH && pShip->vx > 0) {
    pShip->vx = -pShip->vx;
  }
  if (pShip->y < 0 && pShip->vy < 0) {
    pShip->vy = -pShip->vy;
  }
  if (pShip->y > SCREEN_HEIGHT && pShip->vy > 0) {
    pShip->vy = -pShip->vy;
  }
}

void RenderShip(Ship *pShip) {
#if USE_SDL
  float SHIP_D = 10.f;
  SDL_Rect rect;
  rect.x = pShip->x - SHIP_D / 2;
  rect.y = pShip->y - SHIP_D / 2;
  rect.w = SHIP_D;
  rect.h = SHIP_D;
  SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderFillRect(renderer, &rect);
  SDL_RenderDrawRect(renderer, &rect);
  SDL_RenderDrawLine(renderer, pShip->x, pShip->y,
                     pShip->x + cos(pShip->a) * 20.f,
                     pShip->y + sin(pShip->a) * 20.f);
#endif
}

void UpdateBullet(Bullet *pBullet, float dt) {
  pBullet->x += pBullet->vx * dt;
  pBullet->y += pBullet->vy * dt;

  if (pBullet->x < 0 && pBullet->vx < 0) {
    pBullet->vx = -pBullet->vx;
  }
  if (pBullet->x > SCREEN_WIDTH && pBullet->vx > 0) {
    pBullet->vx = -pBullet->vx;
  }
  if (pBullet->y < 0 && pBullet->vy < 0) {
    pBullet->vy = -pBullet->vy;
  }
  if (pBullet->y > SCREEN_HEIGHT && pBullet->vy > 0) {
    pBullet->vy = -pBullet->vy;
  }
  pBullet->lifetime -= dt;
}

void RenderBullet(Bullet *pBullet) {
#if USE_SDL
  float BULLET_D = 5.f;
  SDL_Rect rect;
  rect.x = pBullet->x - BULLET_D / 2;
  rect.y = pBullet->y - BULLET_D / 2;
  rect.w = BULLET_D;
  rect.h = BULLET_D;
  SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderFillRect(renderer, &rect);
  SDL_RenderDrawRect(renderer, &rect);
#endif
}

void serverSocketProc(aeEventLoop *eventLoop, int fd, void *clientData,
                      int mask) {
  Ship *pShip = (Ship *)clientData;
  char buf[128];
  int readed = anetRead(fd, buf, 1);
  if (readed <= 0) {
    printf("closed\n");
    close(fd);
    aeDeleteFileEvent(eventLoop, fd, mask);
    for (int i = 0; i < ships.size();) {
      if (ships[i] == pShip) {
        ships.erase(ships.begin() + i);
      } else {
        i++;
      }
    }
    return;
  }
  pShip->bLeft = buf[0] & 0x1;
  pShip->bRight = buf[0] & 0x2;
  pShip->bAccel = buf[0] & 0x4;
  pShip->bShot = buf[0] & 0x8;
}

void clientSocketProc(aeEventLoop *eventLoop, int fd, void *clientData,
                      int mask) {
  char buf[12800];
  int readed = 0;

  readed = anetRead(fd, buf, sizeof(int));
  if (readed <= 0) {
    printf("closed\n");
    close(fd);
    aeDeleteFileEvent(eventLoop, fd, mask);
    return;
  }
  int nShips = 0;
  memcpy(&nShips, buf, sizeof(int));

  readed = anetRead(fd, buf, sizeof(int));
  if (readed <= 0) {
    printf("closed\n");
    close(fd);
    aeDeleteFileEvent(eventLoop, fd, mask);
    return;
  }
  int nBullets = 0;
  memcpy(&nBullets, buf, sizeof(int));

  int stateSize = nShips * sizeof(Ship) + nBullets * sizeof(Bullet);
  readed = anetRead(fd, buf, stateSize);
  if (readed <= 0) {
    printf("closed\n");
    close(fd);
    aeDeleteFileEvent(eventLoop, fd, mask);
    return;
  }

  for (int i = 0; i < ships.size(); i++) {
    delete (ships[i]);
  }
  ships.clear();
  for (int i = 0; i < bullets.size(); i++) {
    delete (bullets[i]);
  }
  bullets.clear();

  int offset = 0;
  for (int i = 0; i < nShips; i++) {
    Ship *pShip = new Ship();
    *pShip = *(Ship *)(&buf[offset]);
    ships.push_back(pShip);
    offset += sizeof(Ship);
  }
  for (int i = 0; i < nBullets; i++) {
    Bullet *pBullet = new Bullet();
    *pBullet = *(Bullet *)(&buf[offset]);
    bullets.push_back(pBullet);
    offset += sizeof(Bullet);
  }
}

void listenProc(aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
  char ip[128];
  int port = 0;
  int sock = anetTcpAccept(err, fd, ip, sizeof(ip), &port);
  if (sock == ANET_ERR) {
    printf("failed to accept : %s\n", err);
    return;
  }
  printf("connected from %s %d\n", ip, port);
  Ship *pShip = new Ship();
  pShip->sock = sock;
  ships.push_back(pShip);
  if (aeCreateFileEvent(eventLoop, sock, AE_READABLE, serverSocketProc,
                        pShip) == AE_ERR) {
    printf("failed to create file event %d\n", errno);
    ships.erase(ships.end() - 1);
    return;
  }
}

int timeProc(aeEventLoop *eventLoop, long long id, void *clientData) {
  {
#if USE_SDL
    SDL_Event e;
    bool bRight = false;
    bool bLeft = false;
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        aeStop(eventLoop);
      }
    }

    if (mode == Client) {
      const Uint8 *state = SDL_GetKeyboardState(NULL);
      char bStat = 0;
      bStat |= state[SDL_SCANCODE_LEFT] ? 0x1 : 0x0;
      bStat |= state[SDL_SCANCODE_RIGHT] ? 0x2 : 0x0;
      bStat |= state[SDL_SCANCODE_UP] ? 0x4 : 0x0;
      bStat |= state[SDL_SCANCODE_DOWN] ? 0x8 : 0x0;

      ClientData *pClient = (ClientData *)clientData;
      int wrote = anetWrite(pClient->sock, &bStat, 1);
      if (wrote != 1) {
        printf("failed to send key state : %d\n", wrote);
        aeStop(eventLoop);
      }
    }
#endif
    if (mode == Server) {
      // create state buffer
      char buf[10240];
      int offset = 0;
      int nShips = ships.size();
      int nBullets = bullets.size();
      memcpy(buf + offset, &nShips, sizeof(nShips));
      offset += sizeof(nShips);
      memcpy(buf + offset, &nBullets, sizeof(nBullets));
      offset += sizeof(nBullets);
      for (int i = 0; i < nShips; i++) {
        memcpy(buf + offset, (const void *)ships[i], sizeof(Ship));
        offset += sizeof(Ship);
      }
      for (int i = 0; i < nBullets; i++) {
        memcpy(buf + offset, (const void *)bullets[i], sizeof(Bullet));
        offset += sizeof(Bullet);
      }

      // send to all clients
      for (int i = 0; i < nShips; i++) {
        int sock = ships[i]->sock;
        int wrote = anetWrite(sock, buf, offset);
        if (wrote != offset) {
          printf("failed to write state buffer\n");
          close(sock);
        }
      }
    }

    for (int i = 0; i < ships.size(); i++) {
      UpdateShip(ships[i], TIMER_INTERVAL_MS * 0.001f);
    }
    for (int i = 0; i < bullets.size(); i++) {
      UpdateBullet(bullets[i], TIMER_INTERVAL_MS * 0.001f);
    }
    for (int i = 0; i < bullets.size();) {
      if (bullets[i]->lifetime <= 0.0f) {
        delete bullets[i];
        bullets.erase(bullets.begin() + i);
      } else {
        i++;
      }
    }
#if USE_SDL
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    for (int i = 0; i < ships.size(); i++) {
      RenderShip(ships[i]);
    }
    for (int i = 0; i < bullets.size(); i++) {
      RenderBullet(bullets[i]);
    }
    SDL_RenderPresent(renderer);
#endif
  }

  return TIMER_INTERVAL_MS;
}

int main(int argc, char *argv[]) {

  if (!init()) {
    return 1;
  }
  if (argc >= 2 && strcmp(argv[1], "--server") == 0) {
    mode = Server;
  }

  int setsize = 1024;
  aeEventLoop *eventLoop = aeCreateEventLoop(setsize);

  ClientData client;

  if (mode == Client) {
    char *connectaddr = "127.0.0.1";
    int connectSock = anetTcpConnect(err, connectaddr, 3000);
    if (connectSock == ANET_ERR) {
      printf("failed to connect server : %s", err);
      return -1;
    }
    client.sock = connectSock;
    aeCreateFileEvent(eventLoop, connectSock, AE_READABLE, clientSocketProc,
                      (void *)&client);
  }
  if (mode == Server) {
    char *bindaddr = "127.0.0.1";
    int listenSock = anetTcpServer(err, 3000, bindaddr, 128);
    if (listenSock == ANET_ERR) {
      printf("failed to create server : %s", err);
      return -1;
    }
    client.sock = listenSock;
    aeCreateFileEvent(eventLoop, listenSock, AE_READABLE, listenProc, NULL);
  }

  void *clientdata = (void *)&client;
  aeCreateTimeEvent(eventLoop, TIMER_INTERVAL_MS, timeProc, clientdata, NULL);

  aeMain(eventLoop);
  if (!quit()) {
    return 2;
  }
  return 0;
}
