#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define SCALE 50
#define WINDOW_WIDTH 16 * SCALE
#define WINDOW_HEIGHT 9 * SCALE
#define PLAYER_ID 1
#define MAX_ENTITIES 1000
#define GRAVITY 2500.0f
#define TERMINAL_FALL_VELOCITY 3000.0f
#define MIN_JUMP_VELOCITY 300.0f
#define JUMP_VELOCITY_INCREMENT 1500.0f
#define MAX_JUMP_VELOCITY 1000.0f
#define JUMP_BUFFER_TIME 0.15f
#define COYOTE_TIME 0.12f
#define AFTER_IMAGES_TIME 0.1f
#define AFTER_IMAGES_COUNT 4
#define AFTER_IMAGES_TOTAL_TIME 0.2f
#define DASH_TIMER 0.2f
#define HORIZONTAL_DASH_VELOCITY 1000.0f
#define VERTICAL_DASH_VELOCITY 750.0f
#define SQRT_TWO 1.141f

typedef enum EntityType EntityType;
enum EntityType {
  PLAYER,
  PLATFORM,
};

typedef enum EntityDirection EntityDirection;
enum EntityDirection {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  LEFTUP,
  LEFTDOWN,
  RIGHTUP,
  RIGHTDOWN,
};

typedef enum EntityState EntityState;
enum EntityState {
  IDLE,
  RUN,
  DASH,
  JUMP,
  FALL,
  WALL_RIDE,
};

typedef struct AfterImage AfterImage;
struct AfterImage {
  Vector2 pos;
  Vector2 hitbox;
  EntityType type;
  EntityState state;
};

typedef struct Entity Entity;
struct Entity {
  Vector2 pos;
  Vector2 prev_pos;
  Vector2 velocity;
  Vector2 hitbox;
  float movement_speed;
  EntityType type;
  EntityDirection direction;
  EntityState state;
  EntityState prev_state;
  bool grounded;
  AfterImage after_images[AFTER_IMAGES_COUNT];
};

static int LAST_ENTITY_ID = PLAYER_ID;
static bool DEBUG = true;

void addEntity(Entity *Entities, Entity entity) {
  assert(LAST_ENTITY_ID < MAX_ENTITIES);
  Entities[LAST_ENTITY_ID] = entity;
  LAST_ENTITY_ID++;
}

void input(Entity *entities, float dt) {
  const float left_stick_dead_zone_x = 0.1f;
  const float left_stick_dead_zone_y = 0.1f;
  const float left_trigger_dead_zone = -0.9f;

  int gamepad = 0;
  float left_stick_x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
  float left_stick_y = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
  float left_trigger =
      GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER);

  if (left_stick_x > -left_stick_dead_zone_x &&
      left_stick_x < left_stick_dead_zone_x) {
    left_stick_x = 0.0f;
  }
  if (left_stick_y > -left_stick_dead_zone_y &&
      left_stick_y < left_stick_dead_zone_y) {
    left_stick_y = 0.0f;
  }
  if (left_trigger < left_trigger_dead_zone) {
    left_trigger = -1.0f;
  }

  Entity *player = &entities[PLAYER_ID];

  static float dash = DASH_TIMER;
  if ((IsKeyPressed(KEY_LEFT_SHIFT) ||
       IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) &&
      player->state != DASH) {
    player->prev_state = player->state;
    player->state = DASH;

    Vector2 dir = {0};
    switch (player->direction) {
    case LEFT: {
      dir.x -= 1;
    } break;
    case RIGHT: {
      dir.x += 1;
    } break;
    case UP: {
      dir.y -= 1;
    } break;
    case DOWN: {
      dir.y += 1;
    } break;
    case LEFTUP: {
      dir.x -= 1;
      dir.y -= 1;
    } break;
    case LEFTDOWN: {
      dir.x -= 1;
      dir.y += 1;
    } break;
    case RIGHTUP: {
      dir.x += 1;
      dir.y -= 1;
    } break;
    case RIGHTDOWN: {
      dir.x += 1;
      dir.y += 1;
    } break;
    }

    if (dir.x != 0 || dir.y != 0) {
      dir = Vector2Normalize(dir);
      player->velocity.x = dir.x * HORIZONTAL_DASH_VELOCITY;
      player->velocity.y = dir.y * VERTICAL_DASH_VELOCITY;
    }
  }

  if (player->state == DASH) {
    dash -= dt;
  }
  if (dash <= 0) {
    player->state = player->prev_state;
    player->prev_state = DASH;
  }

  if (player->state != DASH) {
    dash = DASH_TIMER;
    player->velocity.x = 0;
    if (IsKeyDown(KEY_A) || left_stick_x < -0.25f) {
      player->velocity.x = -player->movement_speed;
      player->direction = LEFT;
    }
    if (IsKeyDown(KEY_D) || left_stick_x > 0.25f) {
      player->velocity.x = player->movement_speed;
      player->direction = RIGHT;
    }
    if (IsKeyDown(KEY_W)) {
      player->direction = UP;
    }
    if (IsKeyDown(KEY_S)) {
      player->direction = DOWN;
    }
    if (IsKeyDown(KEY_A) && IsKeyDown(KEY_W)) {
      player->direction = LEFTUP;
    }
    if (IsKeyDown(KEY_A) && IsKeyDown(KEY_S)) {
      player->direction = LEFTDOWN;
    }
    if (IsKeyDown(KEY_D) && IsKeyDown(KEY_W)) {
      player->direction = RIGHTUP;
    }
    if (IsKeyDown(KEY_D) && IsKeyDown(KEY_S)) {
      player->direction = RIGHTDOWN;
    }

    static float jump_buffer = 0.0f;
    static float coyote = 0.0f;

    bool jump_pressed =
        IsKeyPressed(KEY_SPACE) ||
        IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    bool jump_down =
        IsKeyDown(KEY_SPACE) ||
        IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    bool jump_released =
        IsKeyReleased(KEY_SPACE) ||
        IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    if (jump_down && player->grounded) {
      jump_pressed = true;
    }

    if (jump_pressed) {
      jump_buffer = JUMP_BUFFER_TIME;
    } else {
      jump_buffer -= dt;
    }

    if (player->grounded) {
      coyote = COYOTE_TIME;
    } else {
      coyote -= dt;
    }

    if (jump_buffer > 0.0f && coyote > 0.0f) {
      if (jump_down) {
        player->velocity.y = -MAX_JUMP_VELOCITY;
      } else {
        player->velocity.y = -MIN_JUMP_VELOCITY;
      }
      player->grounded = false;
      jump_buffer = 0.0f;
      coyote = 0.0f;
    }

    if (jump_released && player->velocity.y < -MIN_JUMP_VELOCITY) {
      player->velocity.y = -MIN_JUMP_VELOCITY;
    }
  }

  if (IsKeyPressed(KEY_F1)) {
    DEBUG = !DEBUG;
  }
}

void gravity(Entity *entities, float dt) {
  for (int i = 1; i < LAST_ENTITY_ID; i++) {
    if (entities[i].type == PLAYER) {
      if (entities[i].velocity.y <= TERMINAL_FALL_VELOCITY) {
        entities[i].velocity.y += GRAVITY * dt;
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

      if (entities[i].state != DASH) {

        if (entities[i].prev_pos.x != entities[i].pos.x &&
            entities[i].grounded) {
          entities[i].prev_state = entities[i].state;
          entities[i].state = RUN;
        }
        if (entities[i].prev_pos.y > entities[i].pos.y) {
          entities[i].grounded = false;
          entities[i].prev_state = entities[i].state;
          entities[i].state = JUMP;
        }
        if (entities[i].prev_pos.y < entities[i].pos.y) {
          entities[i].prev_state = entities[i].state;
          entities[i].grounded = false;
          entities[i].state = FALL;
        }
        if (entities[i].prev_pos.x == entities[i].pos.x &&
            entities[i].prev_pos.y == entities[i].pos.y) {
          entities[i].prev_state = entities[i].state;
          entities[i].state = IDLE;
        }
      }

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
              entities[i].grounded = true;
            } else if (entities[i].velocity.y < 0) {
              new_pos.y = entities[j].pos.y + entities[j].hitbox.y / 2.0f +
                          entities[i].hitbox.y / 2.0f;
              entities[i].prev_state = entities[i].state;
              entities[i].state = FALL;
            }
            entities[i].velocity.y = 0;

            break;
          }
        }
      }

      entities[i].prev_pos.x = entities[i].pos.x;
      entities[i].prev_pos.y = entities[i].pos.y;
      entities[i].pos.x = new_pos.x;
      entities[i].pos.y = new_pos.y;
    }
  }
}

void updateAfterImages(Entity *entities, float dt) {
  Entity *player = &entities[PLAYER_ID];
  static float after_images_timer = 0.0f;
  after_images_timer += dt;
  if (after_images_timer >= AFTER_IMAGES_TIME) {
    after_images_timer = after_images_timer - AFTER_IMAGES_TIME;
    AfterImage after_image = {0};
    after_image.pos = player->pos;
    after_image.hitbox = player->hitbox;
    after_image.type = player->type;
    after_image.state = player->state;
    for (int i = 0; i < AFTER_IMAGES_COUNT - 1; i++) {
      player->after_images[i] = player->after_images[i + 1];
    }
    player->after_images[AFTER_IMAGES_COUNT - 1] = after_image;
  }
}

void physics(Entity *entities, float dt) {
  if (entities[PLAYER_ID].state != DASH) {
    gravity(entities, dt);
  }
  updatePosition(entities, dt);
  updateAfterImages(entities, dt);
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

  switch (player.state) {
  case IDLE: {
    snprintf(buffer, sizeof(buffer), "PLAYER STATE: IDLE");
  } break;
  case RUN: {
    snprintf(buffer, sizeof(buffer), "PLAYER STATE: RUN");
  } break;
  case JUMP: {
    snprintf(buffer, sizeof(buffer), "PLAYER STATE: JUMP");
  } break;
  case FALL: {
    snprintf(buffer, sizeof(buffer), "PLAYER STATE: FALL");
  } break;
  case DASH: {
    snprintf(buffer, sizeof(buffer), "PLAYER STATE: DASH");
  } break;
  case WALL_RIDE: {
    snprintf(buffer, sizeof(buffer), "PLAYER STATE: WALL_RIDE");
  } break;
  }
  // printf("%s\n", buffer);
  DrawText(buffer, 2, 2 + 3 * font_size, font_size, WHITE);

  switch (player.direction) {
  case LEFT: {
    snprintf(buffer, sizeof(buffer), "PLAYER DIRECTION: LEFT");
  } break;
  case RIGHT: {
    snprintf(buffer, sizeof(buffer), "PLAYER DIRECTION: RIGHT");
  } break;
  case UP: {
    snprintf(buffer, sizeof(buffer), "PLAYER DIRECTION: UP");
  } break;
  case DOWN: {
    snprintf(buffer, sizeof(buffer), "PLAYER DIRECTION: DOWN");
  } break;
  case LEFTUP: {
    snprintf(buffer, sizeof(buffer), "PLAYER DIRECTION: LEFTUP");
  } break;
  case LEFTDOWN: {
    snprintf(buffer, sizeof(buffer), "PLAYER DIRECTION: LEFTDOWN");
  } break;
  case RIGHTUP: {
    snprintf(buffer, sizeof(buffer), "PLAYER DIRECTION: RIGHTUP");
  } break;
  case RIGHTDOWN: {
    snprintf(buffer, sizeof(buffer), "PLAYER DIRECTION: RIGHTDOWN");
  } break;
  }

  DrawText(buffer, 2, 2 + 4 * font_size, font_size, WHITE);
}

void drawPlayer(Entity player, float dt) {
  static float after_image_timer = 0.0f;
  if (player.state == DASH) {
    after_image_timer = AFTER_IMAGES_TOTAL_TIME;
  }
  after_image_timer -= dt;
  if (after_image_timer > 0.0f) {
    for (int i = 0; i < AFTER_IMAGES_COUNT; i++) {
      Rectangle after_image_rect = {
          .x = player.after_images[i].pos.x -
               player.after_images[i].hitbox.x / 2.0f,
          .y = player.after_images[i].pos.y -
               player.after_images[i].hitbox.y / 2.0f,
          .width = player.after_images[i].hitbox.x,
          .height = player.after_images[i].hitbox.y,
      };
      float t = (float)i / AFTER_IMAGES_COUNT; // 1.0, 0.75, 0.5, 0.25
      Color after_image_color = {135, 60, 190, (unsigned char)(255 * t)};
      DrawRectangleLinesEx(after_image_rect, 2.0f, after_image_color);
    }
  }

  Rectangle player_rect = {
      .x = player.pos.x - player.hitbox.x / 2.0f,
      .y = player.pos.y - player.hitbox.y / 2.0f,
      .width = player.hitbox.x,
      .height = player.hitbox.y,
  };
  DrawRectangleLinesEx(player_rect, 2.0f, BLUE);
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

void drawEntities(Entity *entities, float dt) {
  if (DEBUG) {
    drawDebug(entities);
  }
  for (int i = 1; i < LAST_ENTITY_ID; i++) {
    switch (entities[i].type) {
    case PLAYER: {
      drawPlayer(entities[i], dt);
    } break;
    case PLATFORM: {
      drawPlatform(entities[i]);
    } break;
    }
  }
}

void render(Entity *entities, float dt) {
  BeginDrawing();
  ClearBackground(BLACK);
  drawEntities(entities, dt);
  EndDrawing();
}

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "le game");
  SetTargetFPS(60);
  Entity entities[MAX_ENTITIES] = {0};

  Entity player = {0};
  player.pos.x = WINDOW_WIDTH / 2.0f;
  player.pos.y = 200.0f;
  player.hitbox.x = 32.0f;
  player.hitbox.y = 32.0f;
  player.movement_speed = 300.0f;
  player.type = PLAYER;
  player.direction = RIGHT;
  player.state = IDLE;
  addEntity(entities, player);

  Entity ground = {0};
  ground.pos.x = WINDOW_WIDTH / 2.0f;
  ground.pos.y = WINDOW_HEIGHT - 10.0f;
  ground.hitbox.x = WINDOW_WIDTH;
  ground.hitbox.y = 20.0f;
  ground.type = PLATFORM;
  addEntity(entities, ground);

  Entity ceiling = {0};
  ceiling.pos.x = WINDOW_WIDTH / 2.0f;
  ceiling.pos.y = 10.0f;
  ceiling.hitbox.x = WINDOW_WIDTH;
  ceiling.hitbox.y = 20.0f;
  ceiling.type = PLATFORM;
  addEntity(entities, ceiling);

  Entity left_wall = {0};
  left_wall.pos.x = 0 + 10.0f;
  left_wall.pos.y = (WINDOW_HEIGHT) / 2.0f;
  left_wall.hitbox.x = 20.0f;
  left_wall.hitbox.y = WINDOW_HEIGHT - 40.0f;
  left_wall.type = PLATFORM;
  addEntity(entities, left_wall);

  Entity right_wall = {0};
  right_wall.pos.x = WINDOW_WIDTH - 10.0f;
  right_wall.pos.y = (WINDOW_HEIGHT) / 2.0f;
  right_wall.hitbox.x = 20.0f;
  right_wall.hitbox.y = WINDOW_HEIGHT - 40.0f;
  right_wall.type = PLATFORM;
  addEntity(entities, right_wall);

  Entity floating_platform = {0};
  floating_platform.pos.x = WINDOW_WIDTH / 2.0f;
  floating_platform.pos.y = WINDOW_HEIGHT * 2.0f / 3.0f;
  floating_platform.hitbox.x = 80.0f;
  floating_platform.hitbox.y = 80.0f;
  floating_platform.type = PLATFORM;
  addEntity(entities, floating_platform);

  float dt = 0.0f;
  while (!WindowShouldClose()) {
    dt = GetFrameTime();
    input(entities, dt);
    physics(entities, dt);
    render(entities, dt);
  }
  CloseWindow();
  return 0;
}
