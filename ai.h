#pragma once

#include "poker.h"

// AI difficulty levels
typedef enum {
    AI_EASY = 0,
    AI_MEDIUM,
    AI_HARD
} AIDifficulty;

// AI personality types
typedef enum {
    AI_CONSERVATIVE = 0,
    AI_AGGRESSIVE,
    AI_RANDOM
} AIPersonality;

// AI player structure
typedef struct {
    AIDifficulty difficulty;
    AIPersonality personality;
    float aggression; // 0.0 to 1.0
    float bluff_frequency; // 0.0 to 1.0
    
    // Enhanced AI features
    uint32_t hands_played;
    uint32_t hands_won;
    float opponent_aggression[MAX_PLAYERS]; // Track other players' aggression
    uint8_t position_awareness; // 0-3 for position consideration
    float risk_tolerance; // How willing to take risks
} AIPlayer;

// Function declarations
void ai_init_players(AIPlayer* ai_players);
PlayerAction ai_decide_action(GameState* game, uint8_t player_index, AIPlayer* ai_player);
uint32_t ai_decide_raise_amount(GameState* game, uint8_t player_index, AIPlayer* ai_player);
float ai_evaluate_hand_strength(Card* hand, Card* community, uint8_t community_count);
bool ai_should_bluff(AIPlayer* ai_player, GameState* game, uint8_t player_index);

// Enhanced AI functions
float ai_calculate_pot_odds(GameState* game, uint8_t player_index);
float ai_evaluate_position(GameState* game, uint8_t player_index);
void ai_update_opponent_model(AIPlayer* ai_player, uint8_t opponent_id, PlayerAction action);
bool ai_should_fold_to_aggression(AIPlayer* ai_player, GameState* game, uint8_t player_index);