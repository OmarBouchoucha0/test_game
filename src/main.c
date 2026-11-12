#include <assert.h>
#include <raylib.h>
#include <stdio.h>

#define SCALE 50
#define WINDOW_WIDTH 16 * SCALE
#define WINDOW_HEIGHT 9 * SCALE
#define PLAYER_ID 1
#define MAX_ENTITIES 1000
#define GRAVITY 80.0f
#define TERMINAL_FALL_VELOCITY 300.0f

typedef enum EntityType EntityType;
enum EntityType {
  PLAYER,
  PLATFORM,
};

typedef struct Entity Entity;
struct Entity {
  Vector2 pos;
  Vector2 velocity;
  Vector2 hitbox;
  float movement_speed;
  EntityType type;
};
static int LAST_ENTITY_ID = PLAYER_ID;
static bool DEBUG = true;

void addEntity(Entity *Entities, Entity entity) {
  assert(LAST_ENTITY_ID < MAX_ENTITIES);
  Entities[LAST_ENTITY_ID] = entity;
  LAST_ENTITY_ID++;
}

void input(Entity *entities) {
  Entity *player = &entities[PLAYER_ID];
  player->velocity.x = 0;
  if (IsKeyDown(KEY_A)) {
    player->velocity.x = -player->movement_speed;
  }
  if (IsKeyDown(KEY_D)) {
    player->velocity.x = player->movement_speed;
  }
  if (IsKeyPressed(KEY_F1)) {
    DEBUG = !DEBUG;
  }
}

void gravity(Entity *entities) {
  for (int i = 1; i < LAST_ENTITY_ID; i++) {
    if (entities[i].type == PLAYER) {
      if (entities[i].velocity.y <= TERMINAL_FALL_VELOCITY) {
        entities[i].velocity.y += GRAVITY;
      } else {
        entities[i].velocity.y = TERMINAL_FALL_VELOCITY;
      }
    }
  }
}

void updatePosition(Entity *entities, float dt) {
  for (int i = 1; i < LAST_ENTITY_ID; i++) {
    if (entities[i].type == PLAYER) {
      Vector2 new_pos = {
          .x = entities[i].pos.x + entities[i].velocity.x * dt,
          .y = entities[i].pos.y + entities[i].velocity.y * dt,
      };

      Rectangle entity_rect1 = {
          entities[i].pos.x - entities[i].hitbox.x / 2.0f,
          entities[i].pos.y - entities[i].hitbox.y / 2.0f,
          entities[i].hitbox.x,
          entities[i].hitbox.y,
      };

      entity_rect1.x = new_pos.x - entities[i].hitbox.x / 2.0f;
      for (int j = 1; j < LAST_ENTITY_ID; j++) {
        if (i != j) {
          Rectangle entity_rect2 = {
              entities[j].pos.x - entities[j].hitbox.x / 2.0f,
              entities[j].pos.y - entities[j].hitbox.y / 2.0f,
              entities[j].hitbox.x,
              entities[j].hitbox.y,
          };
          if (CheckCollisionRecs(entity_rect1, entity_rect2)) {
            if (entities[i].velocity.x > 0) {
              new_pos.x = entities[j].pos.x - entities[j].hitbox.x / 2.0f -
                          entities[i].hitbox.x / 2.0f;
            } else if (entities[i].velocity.x < 0) {
              new_pos.x = entities[j].pos.x + entities[j].hitbox.x / 2.0f +
                          entities[i].hitbox.x / 2.0f;
            }
            break;
          }
        }
      }

      entity_rect1.x = entities[i].pos.x - entities[i].hitbox.x / 2.0f,
      entity_rect1.y = new_pos.y - entities[i].hitbox.y / 2.0f;
      for (int j = 1; j < LAST_ENTITY_ID; j++) {
        if (i != j) {
          Rectangle entity_rect2 = {
              entities[j].pos.x - entities[j].hitbox.x / 2.0f,
              entities[j].pos.y - entities[j].hitbox.y / 2.0f,
              entities[j].hitbox.x,
              entities[j].hitbox.y,
          };
          if (CheckCollisionRecs(entity_rect1, entity_rect2)) {

            if (entities[i].velocity.y > 0) {
              new_pos.y = entities[j].pos.y - entities[j].hitbox.y / 2.0f -
                          entities[i].hitbox.y / 2.0f;
            } else if (entities[i].velocity.y < 0) {
              new_pos.y = entities[j].pos.y + entities[j].hitbox.y / 2.0f +
                          entities[i].hitbox.y / 2.0f;
            }
            if (entities[i].velocity.y > 0) {
              entities[i].velocity.y = 0;
            }
            break;
          }
        }
      }

      entities[i].pos.x = new_pos.x;
      entities[i].pos.y = new_pos.y;
    }
  }
}

void physics(Entity *entities, float dt) {
  gravity(entities);
  updatePosition(entities, dt);
}

void drawDebug(Entity *entities) {
  Entity player = entities[PLAYER_ID];
  char buffer[100];
  int font_size = 16;
  snprintf(buffer, sizeof(buffer), "FPS : %d", GetFPS());
  DrawText(buffer, 2, 2, font_size, WHITE);

  DrawText("player pos", 2, 2 + font_size, font_size, WHITE);
  snprintf(buffer, sizeof(buffer), "%.2f", player.pos.x);
  DrawText(buffer, 90, 2 + font_size, font_size, WHITE);
  snprintf(buffer, sizeof(buffer), "%.2f", player.pos.y);
  DrawText(buffer, 150, 2 + font_size, font_size, WHITE);

  DrawText("player vel", 2, 2 + 2 * font_size, font_size, WHITE);
  snprintf(buffer, sizeof(buffer), "%.2f", player.velocity.x);
  DrawText(buffer, 90, 2 + 2 * font_size, font_size, WHITE);
  snprintf(buffer, sizeof(buffer), "%.2f", player.velocity.y);
  DrawText(buffer, 150, 2 + 2 * font_size, font_size, WHITE);
}

void drawPlayer(Entity player) {
  Rectangle rect = {
      .x = player.pos.x - player.hitbox.x / 2.0f,
      .y = player.pos.y - player.hitbox.y / 2.0f,
      .width = player.hitbox.x,
      .height = player.hitbox.y,
  };
  DrawRectangleLinesEx(rect, 2.0f, BLUE);
}

void drawPlatform(Entity platform) {
  Rectangle rect = {
      .x = platform.pos.x - platform.hitbox.x / 2.0f,
      .y = platform.pos.y - platform.hitbox.y / 2.0f,
      .width = platform.hitbox.x,
      .height = platform.hitbox.y,
  };
  DrawRectangleLinesEx(rect, 2.0f, WHITE);
}

void drawEntities(Entity *entities) {
  if (DEBUG) {
    drawDebug(entities);
  }
  for (int i = 1; i < LAST_ENTITY_ID; i++) {
    switch (entities[i].type) {
    case PLAYER: {
      drawPlayer(entities[i]);
    } break;
    case PLATFORM: {
      drawPlatform(entities[i]);
    } break;
    }
  }
}

void render(Entity *entities) {
  BeginDrawing();
  ClearBackground(BLACK);
  drawEntities(entities);
  EndDrawing();
}

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "le game");
  SetTargetFPS(60);
  Entity entities[MAX_ENTITIES] = {0};

  Entity player = {0};
  player.pos.x = WINDOW_WIDTH / 2.0f;
  player.pos.y = 50.0f;
  player.hitbox.x = 32.0f;
  player.hitbox.y = 32.0f;
  player.movement_speed = 300.0f;
  player.type = PLAYER;
  addEntity(entities, player);

  Entity ground = {0};
  ground.pos.x = WINDOW_WIDTH / 2.0f;
  ground.pos.y = WINDOW_HEIGHT - 40.0f;
  ground.hitbox.x = WINDOW_WIDTH;
  ground.hitbox.y = 60.0f;
  ground.type = PLATFORM;
  addEntity(entities, ground);

  Entity left_wall = {0};
  left_wall.pos.x = 0 + 20.0f;
  left_wall.pos.y = (WINDOW_HEIGHT - 70.0f) / 2.0f;
  left_wall.hitbox.x = 40.0f;
  left_wall.hitbox.y = WINDOW_HEIGHT - 70.0f;
  left_wall.type = PLATFORM;
  addEntity(entities, left_wall);

  Entity right_wall = {0};
  right_wall.pos.x = WINDOW_WIDTH - 20.0f;
  right_wall.pos.y = (WINDOW_HEIGHT - 70.0f) / 2.0f;
  right_wall.hitbox.x = 40.0f;
  right_wall.hitbox.y = WINDOW_HEIGHT - 70.0f;
  right_wall.type = PLATFORM;
  addEntity(entities, right_wall);

  Entity floating_platform = {0};
  floating_platform.pos.x = WINDOW_WIDTH / 2.0f;
  floating_platform.pos.y = WINDOW_HEIGHT / 2.0f;
  floating_platform.hitbox.x = 80.0f;
  floating_platform.hitbox.y = 80.0f;
  floating_platform.type = PLATFORM;
  addEntity(entities, floating_platform);

  float dt = 0.0f;
  while (!WindowShouldClose()) {
    dt = GetFrameTime();
    input(entities);
    physics(entities, dt);
    render(entities);
  }
  CloseWindow();
  return 0;
}
