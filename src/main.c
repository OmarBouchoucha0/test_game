#include <raylib.h>

#define SCALE 30
#define WINDOW_WIDTH 16 * SCALE
#define WINDOW_HEIGHT 9 * SCALE

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "le game");
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("Helo", 200, 200, 32, WHITE);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
