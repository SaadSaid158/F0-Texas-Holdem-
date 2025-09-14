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
} AIPlayer;

// Function declarations
void ai_init_players(AIPlayer* ai_players);
PlayerAction ai_decide_action(GameState* game, uint8_t player_index, AIPlayer* ai_player);
uint32_t ai_decide_raise_amount(GameState* game, uint8_t player_index, AIPlayer* ai_player);
float ai_evaluate_hand_strength(Card* hand, Card* community, uint8_t community_count);
bool ai_should_bluff(AIPlayer* ai_player, GameState* game, uint8_t player_index);