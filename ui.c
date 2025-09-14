#include "ui.h"
#include <gui/elements.h>
#include <furi.h>
#include <string.h>

void ui_get_card_display(Card card, char* buffer, size_t buffer_size) {
    const char* rank_chars = "23456789TJQKA";
    const char* suit_symbols[] = {"♥", "♦", "♣", "♠"};
    
    if(buffer_size >= 4) {
        // Use simple letters for now since Unicode might not be supported
        const char* suit_chars = "HDCS";
        buffer[0] = rank_chars[card.rank - 2];
        buffer[1] = suit_chars[card.suit];
        buffer[2] = '\0';
    }
}

void ui_draw_cards(Canvas* canvas, Card* cards, uint8_t count, uint8_t x, uint8_t y) {
    for(uint8_t i = 0; i < count; i++) {
        uint8_t card_x = x + (i * (CARD_WIDTH + 2));
        
        // Draw card background
        canvas_draw_rframe(canvas, card_x, y, CARD_WIDTH, CARD_HEIGHT, 2);
        canvas_draw_frame(canvas, card_x, y, CARD_WIDTH, CARD_HEIGHT);
        
        // Draw card text
        char card_str[4];
        ui_get_card_display(cards[i], card_str, sizeof(card_str));
        
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, card_x + 2, y + 8, card_str);
    }
}

void ui_draw_community_cards(Canvas* canvas, Card* community, uint8_t count) {
    if(count == 0) return;
    
    uint8_t start_x = (SCREEN_WIDTH - (count * (CARD_WIDTH + 2) - 2)) / 2;
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, start_x, 8, "Community:");
    
    ui_draw_cards(canvas, community, count, start_x, 10);
}

void ui_draw_player_info(Canvas* canvas, Player* player, uint8_t x, uint8_t y, bool is_current) {
    char info_str[32];
    
    // Highlight current player
    if(is_current) {
        canvas_draw_rframe(canvas, x - 1, y - 1, 30, 12, 1);
    }
    
    // Draw player name
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, x, y, player->name);
    
    // Draw chips
    snprintf(info_str, sizeof(info_str), "$%lu", player->chips);
    canvas_draw_str(canvas, x, y + 8, info_str);
    
    // Draw current bet if any
    if(player->bet > 0) {
        snprintf(info_str, sizeof(info_str), "Bet:%lu", player->bet);
        canvas_draw_str(canvas, x, y + 16, info_str);
    }
    
    // Draw status
    if(player->folded) {
        canvas_draw_str(canvas, x, y + 24, "FOLD");
    } else if(player->all_in) {
        canvas_draw_str(canvas, x, y + 24, "ALL-IN");
    } else {
        // Show last action
        switch(player->last_action) {
            case ACTION_CHECK:
                canvas_draw_str(canvas, x, y + 24, "CHECK");
                break;
            case ACTION_CALL:
                canvas_draw_str(canvas, x, y + 24, "CALL");
                break;
            case ACTION_RAISE:
                canvas_draw_str(canvas, x, y + 24, "RAISE");
                break;
            default:
                break;
        }
    }
}

void ui_draw_pot_info(Canvas* canvas, uint32_t pot, uint32_t current_bet) {
    char pot_str[32];
    
    canvas_set_font(canvas, FontSecondary);
    snprintf(pot_str, sizeof(pot_str), "Pot: $%lu", pot);
    canvas_draw_str(canvas, 2, SCREEN_HEIGHT - 24, pot_str);
    
    if(current_bet > 0) {
        snprintf(pot_str, sizeof(pot_str), "Bet: $%lu", current_bet);
        canvas_draw_str(canvas, 2, SCREEN_HEIGHT - 16, pot_str);
    }
}

void ui_draw_menu(Canvas* canvas, MenuOption selected) {
    const char* menu_items[] = {"Fold", "Check/Call", "Raise"};
    uint8_t menu_y = SCREEN_HEIGHT - 32;
    
    canvas_set_font(canvas, FontSecondary);
    
    for(uint8_t i = 0; i < MENU_COUNT; i++) {
        uint8_t x = 70 + (i * 20);
        
        if(i == selected) {
            canvas_draw_rframe(canvas, x - 2, menu_y - 2, 18, 12, 1);
            canvas_set_color(canvas, ColorBlack);
        } else {
            canvas_set_color(canvas, ColorBlack);
        }
        
        canvas_draw_str(canvas, x, menu_y + 6, menu_items[i]);
    }
}

void ui_draw_notification(Canvas* canvas, const char* message) {
    if(!message) return;
    
    uint8_t msg_len = strlen(message);
    uint8_t msg_width = msg_len * 6; // Approximate character width
    uint8_t x = (SCREEN_WIDTH - msg_width) / 2;
    uint8_t y = SCREEN_HEIGHT / 2;
    
    // Draw background
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_rbox(canvas, x - 4, y - 8, msg_width + 8, 16, 2);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_rframe(canvas, x - 4, y - 8, msg_width + 8, 16, 2);
    
    // Draw message
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, x, y, message);
}

void ui_draw_game_screen(Canvas* canvas, GameState* game) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    
    // Draw community cards
    ui_draw_community_cards(canvas, game->community, game->community_count);
    
    // Draw AI players info (players 1, 2, 3)
    ui_draw_player_info(canvas, &game->players[1], 2, 25, game->current_player == 1);
    ui_draw_player_info(canvas, &game->players[2], 50, 25, game->current_player == 2);
    ui_draw_player_info(canvas, &game->players[3], 98, 25, game->current_player == 3);
    
    // Draw human player (player 0) at bottom
    Player* human = &game->players[0];
    ui_draw_player_info(canvas, human, 2, SCREEN_HEIGHT - 48, game->current_player == 0);
    
    // Draw human player's cards
    if(!human->folded) {
        ui_draw_cards(canvas, human->hand, HAND_SIZE, 40, SCREEN_HEIGHT - 48);
    }
    
    // Draw pot info
    ui_draw_pot_info(canvas, game->pot, game->current_bet);
    
    // Draw game phase
    const char* phase_names[] = {"Pre-flop", "Flop", "Turn", "River", "Showdown"};
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, SCREEN_WIDTH - 40, 8, phase_names[game->phase]);
    
    // Draw hand number
    char hand_str[16];
    snprintf(hand_str, sizeof(hand_str), "Hand #%lu", game->hand_number);
    canvas_draw_str(canvas, 2, 8, hand_str);
    
    // Draw blind indicators
    if(game->phase == PHASE_PREFLOP && game->blinds_posted) {
        canvas_set_font(canvas, FontSecondary);
        
        // Small blind indicator
        if(game->small_blind_pos == 0) {
            canvas_draw_str(canvas, 2, SCREEN_HEIGHT - 56, "SB");
        } else if(game->small_blind_pos == 1) {
            canvas_draw_str(canvas, 2, 17, "SB");
        } else if(game->small_blind_pos == 2) {
            canvas_draw_str(canvas, 50, 17, "SB");
        } else if(game->small_blind_pos == 3) {
            canvas_draw_str(canvas, 98, 17, "SB");
        }
        
        // Big blind indicator
        if(game->big_blind_pos == 0) {
            canvas_draw_str(canvas, 12, SCREEN_HEIGHT - 56, "BB");
        } else if(game->big_blind_pos == 1) {
            canvas_draw_str(canvas, 12, 17, "BB");
        } else if(game->big_blind_pos == 2) {
            canvas_draw_str(canvas, 60, 17, "BB");
        } else if(game->big_blind_pos == 3) {
            canvas_draw_str(canvas, 108, 17, "BB");
        }
    }
}