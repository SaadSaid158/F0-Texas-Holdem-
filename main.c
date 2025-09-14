#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <furi_hal.h>
#include <string.h>

#include "poker.h"
#include "ai.h"
#include "ui.h"

typedef struct {
    GameState game;
    AIPlayer ai_players[MAX_PLAYERS - 1];
    MenuOption selected_menu;
    UIState ui_state;
    bool show_notification;
    char notification_text[64];
    uint32_t notification_timer;
    FuriTimer* game_timer;
    bool waiting_for_input;
} TexasHoldemApp;

// Forward declarations
static void game_init(TexasHoldemApp* app);
static void game_new_hand(TexasHoldemApp* app);
static void game_deal_cards(TexasHoldemApp* app);
static void game_next_phase(TexasHoldemApp* app);
static void game_betting_round(TexasHoldemApp* app);
static void game_showdown(TexasHoldemApp* app);
static void game_update_pot(TexasHoldemApp* app);
static bool game_is_betting_complete(TexasHoldemApp* app);
static void game_process_player_action(TexasHoldemApp* app, PlayerAction action);
static void game_process_ai_turn(TexasHoldemApp* app);
static void game_show_notification(TexasHoldemApp* app, const char* message);

static void render_callback(Canvas* canvas, void* ctx) {
    TexasHoldemApp* app = (TexasHoldemApp*)ctx;
    
    ui_draw_game_screen(canvas, &app->game);
    
    if(app->game.current_player == 0 && !app->game.players[0].folded && app->waiting_for_input) {
        ui_draw_menu(canvas, app->selected_menu);
    }
    
    if(app->show_notification) {
        ui_draw_notification(canvas, app->notification_text);
    }
}

static void input_callback(InputEvent* input_event, void* ctx) {
    TexasHoldemApp* app = (TexasHoldemApp*)ctx;
    
    if(input_event->type != InputTypePress) return;
    
    if(app->show_notification) {
        if(input_event->key == InputKeyOk) {
            app->show_notification = false;
        }
        return;
    }
    
    if(app->game.current_player == 0 && app->waiting_for_input && !app->game.players[0].folded) {
        switch(input_event->key) {
            case InputKeyLeft:
                if(app->selected_menu > 0) {
                    app->selected_menu--;
                }
                break;
            case InputKeyRight:
                if(app->selected_menu < MENU_COUNT - 1) {
                    app->selected_menu++;
                }
                break;
            case InputKeyOk:
                switch(app->selected_menu) {
                    case MENU_FOLD:
                        game_process_player_action(app, ACTION_FOLD);
                        break;
                    case MENU_CHECK_CALL:
                        if(app->game.current_bet > app->game.players[0].bet) {
                            game_process_player_action(app, ACTION_CALL);
                        } else {
                            game_process_player_action(app, ACTION_CHECK);
                        }
                        break;
                    case MENU_RAISE:
                        game_process_player_action(app, ACTION_RAISE);
                        break;
                }
                break;
            case InputKeyBack:
                // Exit game
                app->game.game_over = true;
                break;
            default:
                break;
        }
    }
}

static void timer_callback(void* ctx) {
    TexasHoldemApp* app = (TexasHoldemApp*)ctx;
    
    if(app->show_notification) {
        app->notification_timer++;
        if(app->notification_timer > 60) { // 3 seconds at 20 FPS
            app->show_notification = false;
            app->notification_timer = 0;
        }
    }
    
    // Process AI turns
    if(app->game.current_player != 0 && !app->waiting_for_input) {
        game_process_ai_turn(app);
    }
}

static void game_init(TexasHoldemApp* app) {
    // Initialize random seed
    srand(furi_hal_random_get());
    
    // Initialize players
    for(uint8_t i = 0; i < MAX_PLAYERS; i++) {
        app->game.players[i].chips = STARTING_CHIPS;
        app->game.players[i].bet = 0;
        app->game.players[i].folded = false;
        app->game.players[i].all_in = false;
        app->game.players[i].last_action = ACTION_CHECK;
        
        if(i == 0) {
            strcpy(app->game.players[i].name, "You");
        } else {
            snprintf(app->game.players[i].name, sizeof(app->game.players[i].name), "AI%d", i);
        }
    }
    
    // Initialize AI players
    ai_init_players(app->ai_players);
    
    // Initialize game state
    app->game.community_count = 0;
    app->game.phase = PHASE_PREFLOP;
    app->game.current_player = 0;
    app->game.dealer = 0;
    app->game.pot = 0;
    app->game.current_bet = 0;
    app->game.active_players = MAX_PLAYERS;
    app->game.game_over = false;
    
    // Initialize UI state
    app->selected_menu = MENU_CHECK_CALL;
    app->ui_state = UI_STATE_GAME;
    app->show_notification = false;
    app->waiting_for_input = false;
    
    game_new_hand(app);
}

static void game_new_hand(TexasHoldemApp* app) {
    // Reset for new hand
    for(uint8_t i = 0; i < MAX_PLAYERS; i++) {
        app->game.players[i].bet = 0;
        app->game.players[i].folded = false;
        app->game.players[i].all_in = false;
        app->game.players[i].last_action = ACTION_CHECK;
    }
    
    app->game.community_count = 0;
    app->game.phase = PHASE_PREFLOP;
    app->game.pot = 0;
    app->game.current_bet = 0;
    app->game.active_players = MAX_PLAYERS;
    
    // Move dealer button
    app->game.dealer = (app->game.dealer + 1) % MAX_PLAYERS;
    app->game.current_player = (app->game.dealer + 1) % MAX_PLAYERS;
    
    // Initialize and shuffle deck
    poker_init_deck(&app->game.deck);
    poker_shuffle_deck(&app->game.deck);
    
    // Deal hole cards
    game_deal_cards(app);
    
    // Start betting round
    game_betting_round(app);
}

static void game_deal_cards(TexasHoldemApp* app) {
    // Deal two hole cards to each player
    for(uint8_t i = 0; i < HAND_SIZE; i++) {
        for(uint8_t j = 0; j < MAX_PLAYERS; j++) {
            app->game.players[j].hand[i] = poker_deal_card(&app->game.deck);
        }
    }
}

static void game_betting_round(TexasHoldemApp* app) {
    if(app->game.current_player == 0) {
        app->waiting_for_input = true;
    } else {
        app->waiting_for_input = false;
    }
}

static void game_process_player_action(TexasHoldemApp* app, PlayerAction action) {
    Player* player = &app->game.players[app->game.current_player];
    
    switch(action) {
        case ACTION_FOLD:
            player->folded = true;
            player->last_action = ACTION_FOLD;
            app->game.active_players--;
            game_show_notification(app, "You folded");
            break;
            
        case ACTION_CHECK:
            player->last_action = ACTION_CHECK;
            game_show_notification(app, "You checked");
            break;
            
        case ACTION_CALL:
            {
                uint32_t call_amount = app->game.current_bet - player->bet;
                if(call_amount >= player->chips) {
                    // All-in
                    call_amount = player->chips;
                    player->all_in = true;
                }
                player->bet += call_amount;
                player->chips -= call_amount;
                player->last_action = ACTION_CALL;
                
                char msg[32];
                snprintf(msg, sizeof(msg), "You called $%lu", call_amount);
                game_show_notification(app, msg);
            }
            break;
            
        case ACTION_RAISE:
            {
                uint32_t raise_amount = app->game.current_bet * 2; // Simple 2x raise
                if(raise_amount > player->chips) {
                    raise_amount = player->chips;
                    player->all_in = true;
                }
                
                uint32_t total_bet = raise_amount;
                player->chips -= (total_bet - player->bet);
                player->bet = total_bet;
                app->game.current_bet = total_bet;
                player->last_action = ACTION_RAISE;
                
                char msg[32];
                snprintf(msg, sizeof(msg), "You raised to $%lu", total_bet);
                game_show_notification(app, msg);
            }
            break;
    }
    
    // Move to next player
    do {
        app->game.current_player = (app->game.current_player + 1) % MAX_PLAYERS;
    } while(app->game.players[app->game.current_player].folded || 
            app->game.players[app->game.current_player].all_in);
    
    // Check if betting round is complete
    if(game_is_betting_complete(app)) {
        game_update_pot(app);
        
        if(app->game.active_players <= 1) {
            // Only one player left, they win
            game_showdown(app);
        } else {
            game_next_phase(app);
        }
    } else {
        game_betting_round(app);
    }
    
    app->waiting_for_input = false;
}

static void game_process_ai_turn(TexasHoldemApp* app) {
    uint8_t ai_index = app->game.current_player - 1;
    Player* player = &app->game.players[app->game.current_player];
    
    PlayerAction action = ai_decide_action(&app->game, app->game.current_player, &app->ai_players[ai_index]);
    
    switch(action) {
        case ACTION_FOLD:
            player->folded = true;
            player->last_action = ACTION_FOLD;
            app->game.active_players--;
            break;
            
        case ACTION_CHECK:
            player->last_action = ACTION_CHECK;
            break;
            
        case ACTION_CALL:
            {
                uint32_t call_amount = app->game.current_bet - player->bet;
                if(call_amount >= player->chips) {
                    call_amount = player->chips;
                    player->all_in = true;
                }
                player->bet += call_amount;
                player->chips -= call_amount;
                player->last_action = ACTION_CALL;
            }
            break;
            
        case ACTION_RAISE:
            {
                uint32_t raise_amount = ai_decide_raise_amount(&app->game, app->game.current_player, &app->ai_players[ai_index]);
                if(raise_amount > player->chips) {
                    raise_amount = player->chips;
                    player->all_in = true;
                }
                
                player->chips -= (raise_amount - player->bet);
                player->bet = raise_amount;
                app->game.current_bet = raise_amount;
                player->last_action = ACTION_RAISE;
            }
            break;
    }
    
    // Move to next player
    do {
        app->game.current_player = (app->game.current_player + 1) % MAX_PLAYERS;
    } while(app->game.players[app->game.current_player].folded || 
            app->game.players[app->game.current_player].all_in);
    
    // Check if betting round is complete
    if(game_is_betting_complete(app)) {
        game_update_pot(app);
        
        if(app->game.active_players <= 1) {
            game_showdown(app);
        } else {
            game_next_phase(app);
        }
    } else {
        game_betting_round(app);
    }
}

static bool game_is_betting_complete(TexasHoldemApp* app) {
    uint32_t max_bet = 0;
    uint8_t players_to_act = 0;
    
    // Find maximum bet
    for(uint8_t i = 0; i < MAX_PLAYERS; i++) {
        if(!app->game.players[i].folded && app->game.players[i].bet > max_bet) {
            max_bet = app->game.players[i].bet;
        }
    }
    
    // Count players who haven't matched the max bet and can still act
    for(uint8_t i = 0; i < MAX_PLAYERS; i++) {
        Player* player = &app->game.players[i];
        if(!player->folded && !player->all_in && player->bet < max_bet) {
            players_to_act++;
        }
    }
    
    return players_to_act == 0;
}

static void game_update_pot(TexasHoldemApp* app) {
    for(uint8_t i = 0; i < MAX_PLAYERS; i++) {
        app->game.pot += app->game.players[i].bet;
        app->game.players[i].bet = 0;
    }
    app->game.current_bet = 0;
}

static void game_next_phase(TexasHoldemApp* app) {
    switch(app->game.phase) {
        case PHASE_PREFLOP:
            // Deal flop (3 cards)
            for(uint8_t i = 0; i < 3; i++) {
                app->game.community[i] = poker_deal_card(&app->game.deck);
            }
            app->game.community_count = 3;
            app->game.phase = PHASE_FLOP;
            break;
            
        case PHASE_FLOP:
            // Deal turn (1 card)
            app->game.community[3] = poker_deal_card(&app->game.deck);
            app->game.community_count = 4;
            app->game.phase = PHASE_TURN;
            break;
            
        case PHASE_TURN:
            // Deal river (1 card)
            app->game.community[4] = poker_deal_card(&app->game.deck);
            app->game.community_count = 5;
            app->game.phase = PHASE_RIVER;
            break;
            
        case PHASE_RIVER:
            // Go to showdown
            game_showdown(app);
            return;
            
        case PHASE_SHOWDOWN:
            // Start new hand
            game_new_hand(app);
            return;
    }
    
    // Reset for new betting round
    app->game.current_player = (app->game.dealer + 1) % MAX_PLAYERS;
    while(app->game.players[app->game.current_player].folded) {
        app->game.current_player = (app->game.current_player + 1) % MAX_PLAYERS;
    }
    
    game_betting_round(app);
}

static void game_showdown(TexasHoldemApp* app) {
    app->game.phase = PHASE_SHOWDOWN;
    
    HandResult best_hands[MAX_PLAYERS];
    uint8_t winners[MAX_PLAYERS];
    uint8_t winner_count = 0;
    
    // Evaluate all non-folded hands
    for(uint8_t i = 0; i < MAX_PLAYERS; i++) {
        if(!app->game.players[i].folded) {
            best_hands[i] = poker_evaluate_hand(app->game.players[i].hand, app->game.community, app->game.community_count);
        }
    }
    
    // Find winner(s)
    for(uint8_t i = 0; i < MAX_PLAYERS; i++) {
        if(app->game.players[i].folded) continue;
        
        bool is_winner = true;
        for(uint8_t j = 0; j < MAX_PLAYERS; j++) {
            if(i == j || app->game.players[j].folded) continue;
            
            if(poker_compare_hands(&best_hands[i], &best_hands[j]) < 0) {
                is_winner = false;
                break;
            }
        }
        
        if(is_winner) {
            winners[winner_count++] = i;
        }
    }
    
    // Distribute pot
    uint32_t share = app->game.pot / winner_count;
    for(uint8_t i = 0; i < winner_count; i++) {
        app->game.players[winners[i]].chips += share;
    }
    
    // Show result
    if(winner_count == 1) {
        char msg[64];
        if(winners[0] == 0) {
            snprintf(msg, sizeof(msg), "You won $%lu!", share);
        } else {
            snprintf(msg, sizeof(msg), "%s won $%lu", app->game.players[winners[0]].name, share);
        }
        game_show_notification(app, msg);
    } else {
        game_show_notification(app, "Split pot!");
    }
    
    app->game.pot = 0;
    
    // Check for game over
    if(app->game.players[0].chips == 0) {
        game_show_notification(app, "Game Over!");
        app->game.game_over = true;
    } else {
        // Start new hand after delay
        furi_delay_ms(3000);
        game_new_hand(app);
    }
}

static void game_show_notification(TexasHoldemApp* app, const char* message) {
    strcpy(app->notification_text, message);
    app->show_notification = true;
    app->notification_timer = 0;
}

int32_t texas_holdem_app(void* p) {
    UNUSED(p);
    
    TexasHoldemApp* app = malloc(sizeof(TexasHoldemApp));
    
    // Initialize app
    game_init(app);
    
    // Set up GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, render_callback, app);
    view_port_input_callback_set(view_port, input_callback, app);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    
    // Set up timer
    app->game_timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
    furi_timer_start(app->game_timer, furi_ms_to_ticks(50)); // 20 FPS
    
    // Main loop
    while(!app->game.game_over) {
        furi_delay_ms(100);
    }
    
    // Cleanup
    furi_timer_stop(app->game_timer);
    furi_timer_free(app->game_timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    free(app);
    
    return 0;
}