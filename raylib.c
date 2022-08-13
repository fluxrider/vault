// gcc -std=c2x --pedantic -Wall -Werror-implicit-function-declaration raylib.c $(pkg-config --cflags --libs raylib) -lm
#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main(int argc, char * argv[]) {
  int W = 800, H = 600;
  bool fullscreen = false; Vector2 stored_window_position, stored_window_size;
  InitWindow(W, H, "Valut"); SetWindowState(FLAG_WINDOW_RESIZABLE); SetWindowState(FLAG_VSYNC_HINT);

  GuiLoadStyleDefault();
  GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

  char line[12]; line[0] = '\0';
  char text[30]; text[0] = '\0';

  SetTargetFPS(60);
  while(!WindowShouldClose()) {
    if(IsKeyPressed(KEY_F)) { if((fullscreen = !fullscreen)) { stored_window_position = GetWindowPosition(); stored_window_size = (Vector2){GetScreenWidth(),GetScreenHeight()}; SetWindowState(FLAG_WINDOW_UNDECORATED); SetWindowSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor())); } else { ClearWindowState(FLAG_WINDOW_UNDECORATED); SetWindowPosition(stored_window_position.x, stored_window_position.y); SetWindowSize(stored_window_size.x, stored_window_size.y); } }

    BeginDrawing();
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
    
    /*
    GuiLabel((Rectangle){ 10, 10, 60, 24 }, "Style:");
    
    GuiSetIconScale(2);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
    GuiButton((Rectangle){ 25, 255, 300, 30 }, GuiIconText(ICON_FILE_SAVE, "Save File"));
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    */

    GuiTextBox((Rectangle){ 10, 10, 60, 24 }, line, 12, false);
    GuiTextBoxMulti((Rectangle){ 10, 100, 150, 150 }, text, 30, true); // test shows that raygui is not a viable options, it limits range, and gets confused real fast if multiple keys are pressed at the same time
        
    EndDrawing();
  }

  CloseWindow();
  return EXIT_SUCCESS;
}