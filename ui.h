#pragma once

#include <gui/gui.h>
#include "poker.h"

// UI constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CARD_WIDTH 16
#define CARD_HEIGHT 12

// UI states
typedef enum {
    UI_STATE_GAME = 0,
    UI_STATE_MENU,
    UI_STATE_BETTING,
    UI_STATE_SHOWDOWN
} UIState;

// Menu options
typedef enum {
    MENU_FOLD = 0,
    MENU_CHECK_CALL,
    MENU_RAISE,
    MENU_COUNT
} MenuOption;

// Function declarations
void ui_draw_game_screen(Canvas* canvas, GameState* game);
void ui_draw_menu(Canvas* canvas, MenuOption selected);
void ui_draw_cards(Canvas* canvas, Card* cards, uint8_t count, uint8_t x, uint8_t y);
void ui_draw_player_info(Canvas* canvas, Player* player, uint8_t x, uint8_t y, bool is_current);
void ui_draw_community_cards(Canvas* canvas, Card* community, uint8_t count);
void ui_draw_pot_info(Canvas* canvas, uint32_t pot, uint32_t current_bet);
void ui_draw_notification(Canvas* canvas, const char* message);
void ui_get_card_display(Card card, char* buffer, size_t buffer_size);