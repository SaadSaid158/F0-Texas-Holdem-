#include "ai.h"
#include <furi.h>
#include <stdlib.h>

void ai_init_players(AIPlayer* ai_players) {
    // AI Player 1 - Conservative
    ai_players[0].difficulty = AI_EASY;
    ai_players[0].personality = AI_CONSERVATIVE;
    ai_players[0].aggression = 0.3f;
    ai_players[0].bluff_frequency = 0.1f;
    
    // AI Player 2 - Aggressive
    ai_players[1].difficulty = AI_MEDIUM;
    ai_players[1].personality = AI_AGGRESSIVE;
    ai_players[1].aggression = 0.7f;
    ai_players[1].bluff_frequency = 0.3f;
    
    // AI Player 3 - Random/Balanced
    ai_players[2].difficulty = AI_MEDIUM;
    ai_players[2].personality = AI_RANDOM;
    ai_players[2].aggression = 0.5f;
    ai_players[2].bluff_frequency = 0.2f;
}

float ai_evaluate_hand_strength(Card* hand, Card* community, uint8_t community_count) {
    HandResult result = poker_evaluate_hand(hand, community, community_count);
    
    // Convert hand rank to strength value (0.0 to 1.0)
    float base_strength = 0.0f;
    
    switch(result.rank) {
        case HAND_HIGH_CARD:
            base_strength = 0.1f;
            break;
        case HAND_PAIR:
            base_strength = 0.2f;
            break;
        case HAND_TWO_PAIR:
            base_strength = 0.4f;
            break;
        case HAND_THREE_KIND:
            base_strength = 0.5f;
            break;
        case HAND_STRAIGHT:
            base_strength = 0.6f;
            break;
        case HAND_FLUSH:
            base_strength = 0.7f;
            break;
        case HAND_FULL_HOUSE:
            base_strength = 0.8f;
            break;
        case HAND_FOUR_KIND:
            base_strength = 0.9f;
            break;
        case HAND_STRAIGHT_FLUSH:
        case HAND_ROYAL_FLUSH:
            base_strength = 1.0f;
            break;
    }
    
    // Add some variance based on hand value within the rank
    if(result.rank <= HAND_PAIR) {
        // For high card and pair, consider the actual card values
        uint8_t high_card = (result.value >> 16) & 0xF;
        if(high_card == 0) high_card = (result.value >> 12) & 0xF;
        
        if(high_card >= RANK_JACK) {
            base_strength += 0.05f;
        }
        if(high_card >= RANK_ACE) {
            base_strength += 0.05f;
        }
    }
    
    return base_strength;
}

bool ai_should_bluff(AIPlayer* ai_player, GameState* game, uint8_t player_index) {
    // Simple bluffing logic based on personality and situation
    float bluff_chance = ai_player->bluff_frequency;
    
    // Increase bluff chance if pot is small
    if(game->pot < STARTING_CHIPS / 4) {
        bluff_chance *= 1.5f;
    }
    
    // Decrease bluff chance if many players are still active
    if(game->active_players > 2) {
        bluff_chance *= 0.5f;
    }
    
    // Aggressive players bluff more often
    if(ai_player->personality == AI_AGGRESSIVE) {
        bluff_chance *= 1.3f;
    }
    
    return (rand() % 100) < (bluff_chance * 100);
}

PlayerAction ai_decide_action(GameState* game, uint8_t player_index, AIPlayer* ai_player) {
    Player* player = &game->players[player_index];
    
    // If player is all-in or folded, they can't act
    if(player->all_in || player->folded) {
        return ACTION_CHECK;
    }
    
    // Evaluate hand strength
    float hand_strength = ai_evaluate_hand_strength(player->hand, game->community, game->community_count);
    
    // Calculate pot odds if there's a bet to call
    float pot_odds = 0.0f;
    uint32_t call_amount = game->current_bet - player->bet;
    if(call_amount > 0) {
        pot_odds = (float)call_amount / (game->pot + call_amount);
    }
    
    // Adjust decision based on personality and hand strength
    float action_threshold = hand_strength;
    
    // Personality adjustments
    switch(ai_player->personality) {
        case AI_CONSERVATIVE:
            action_threshold -= 0.1f;
            break;
        case AI_AGGRESSIVE:
            action_threshold += 0.1f;
            break;
        case AI_RANDOM:
            action_threshold += ((rand() % 21) - 10) * 0.01f; // +/- 0.1 random
            break;
    }
    
    // Phase-based adjustments
    switch(game->phase) {
        case PHASE_PREFLOP:
            // Be more conservative pre-flop
            action_threshold -= 0.05f;
            break;
        case PHASE_RIVER:
            // Be more aggressive on river with good hands
            if(hand_strength > 0.6f) {
                action_threshold += 0.1f;
            }
            break;
        default:
            break;
    }
    
    // Decide action
    if(call_amount == 0) {
        // No bet to call - check or bet
        if(hand_strength > 0.6f || ai_should_bluff(ai_player, game, player_index)) {
            return ACTION_RAISE;
        } else {
            return ACTION_CHECK;
        }
    } else {
        // There's a bet to call
        if(call_amount >= player->chips) {
            // All-in situation
            if(hand_strength > 0.7f) {
                return ACTION_CALL; // This will be all-in
            } else {
                return ACTION_FOLD;
            }
        }
        
        // Normal betting situation
        if(hand_strength > 0.7f || (hand_strength > 0.4f && ai_should_bluff(ai_player, game, player_index))) {
            return ACTION_RAISE;
        } else if(hand_strength > 0.3f || pot_odds < 0.3f) {
            return ACTION_CALL;
        } else {
            return ACTION_FOLD;
        }
    }
}

uint32_t ai_decide_raise_amount(GameState* game, uint8_t player_index, AIPlayer* ai_player) {
    Player* player = &game->players[player_index];
    float hand_strength = ai_evaluate_hand_strength(player->hand, game->community, game->community_count);
    
    // Base raise amount (fraction of pot)
    float raise_factor = 0.5f;
    
    // Adjust based on hand strength
    if(hand_strength > 0.8f) {
        raise_factor = 1.0f; // Pot-sized bet
    } else if(hand_strength > 0.6f) {
        raise_factor = 0.75f;
    } else if(hand_strength < 0.4f) {
        // Bluffing - smaller bet
        raise_factor = 0.3f;
    }
    
    // Personality adjustments
    switch(ai_player->personality) {
        case AI_CONSERVATIVE:
            raise_factor *= 0.8f;
            break;
        case AI_AGGRESSIVE:
            raise_factor *= 1.3f;
            break;
        case AI_RANDOM:
            raise_factor *= (0.7f + (rand() % 6) * 0.1f); // 0.7x to 1.2x
            break;
    }
    
    uint32_t raise_amount = (uint32_t)(game->pot * raise_factor);
    
    // Ensure minimum raise
    uint32_t min_raise = game->current_bet * 2 - player->bet;
    if(raise_amount < min_raise) {
        raise_amount = min_raise;
    }
    
    // Don't bet more than we have
    if(raise_amount > player->chips) {
        raise_amount = player->chips;
    }
    
    // Minimum bet of 1 chip
    if(raise_amount == 0) {
        raise_amount = 1;
    }
    
    return raise_amount;
}