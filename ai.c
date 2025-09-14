#include "ai.h"
#include <furi.h>
#include <stdlib.h>

void ai_init_players(AIPlayer* ai_players) {
    // AI Player 1 - Conservative
    ai_players[0].difficulty = AI_EASY;
    ai_players[0].personality = AI_CONSERVATIVE;
    ai_players[0].aggression = 0.3f;
    ai_players[0].bluff_frequency = 0.1f;
    ai_players[0].hands_played = 0;
    ai_players[0].hands_won = 0;
    ai_players[0].position_awareness = 2;
    ai_players[0].risk_tolerance = 0.3f;
    
    // AI Player 2 - Aggressive
    ai_players[1].difficulty = AI_MEDIUM;
    ai_players[1].personality = AI_AGGRESSIVE;
    ai_players[1].aggression = 0.7f;
    ai_players[1].bluff_frequency = 0.3f;
    ai_players[1].hands_played = 0;
    ai_players[1].hands_won = 0;
    ai_players[1].position_awareness = 3;
    ai_players[1].risk_tolerance = 0.7f;
    
    // AI Player 3 - Random/Balanced
    ai_players[2].difficulty = AI_MEDIUM;
    ai_players[2].personality = AI_RANDOM;
    ai_players[2].aggression = 0.5f;
    ai_players[2].bluff_frequency = 0.2f;
    ai_players[2].hands_played = 0;
    ai_players[2].hands_won = 0;
    ai_players[2].position_awareness = 2;
    ai_players[2].risk_tolerance = 0.5f;
    
    // Initialize opponent modeling
    for(uint8_t i = 0; i < 3; i++) {
        for(uint8_t j = 0; j < MAX_PLAYERS; j++) {
            ai_players[i].opponent_aggression[j] = 0.5f; // Start with neutral assumption
        }
    }
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
    float pot_odds = ai_calculate_pot_odds(game, player_index);
    uint32_t call_amount = game->current_bet - player->bet;
    
    // Evaluate position advantage
    float position_value = ai_evaluate_position(game, player_index);
    
    // Adjust decision based on personality and hand strength
    float action_threshold = hand_strength;
    
    // Position adjustments - better position allows more aggressive play
    if(ai_player->position_awareness > 0) {
        action_threshold += (position_value - 0.5f) * 0.1f * ai_player->position_awareness;
    }
    
    // Personality adjustments
    switch(ai_player->personality) {
        case AI_CONSERVATIVE:
            action_threshold -= 0.1f;
            // More cautious against aggressive opponents
            if(ai_should_fold_to_aggression(ai_player, game, player_index)) {
                action_threshold -= 0.15f;
            }
            break;
        case AI_AGGRESSIVE:
            action_threshold += 0.1f;
            // Less affected by opponent aggression
            break;
        case AI_RANDOM:
            action_threshold += ((rand() % 21) - 10) * 0.01f; // +/- 0.1 random
            break;
    }
    
    // Phase-based adjustments
    switch(game->phase) {
        case PHASE_PREFLOP:
            // Be more conservative pre-flop, except in good position
            action_threshold -= 0.05f + (0.05f * (1.0f - position_value));
            break;
        case PHASE_FLOP:
            // Standard adjustments
            break;
        case PHASE_TURN:
            // Slightly more conservative as fewer cards remain
            action_threshold -= 0.02f;
            break;
        case PHASE_RIVER:
            // Be more aggressive on river with good hands, more cautious with marginal ones
            if(hand_strength > 0.6f) {
                action_threshold += 0.1f;
            } else {
                action_threshold -= 0.05f;
            }
            break;
        default:
            break;
    }
    
    // Stack size considerations
    float stack_ratio = (float)player->chips / STARTING_CHIPS;
    if(stack_ratio < 0.3f) {
        // Short stack - be more aggressive with decent hands
        if(hand_strength > 0.4f) {
            action_threshold += 0.1f;
        }
    } else if(stack_ratio > 2.0f) {
        // Big stack - can afford to be more aggressive
        action_threshold += 0.05f;
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
            if(hand_strength > (0.7f - ai_player->risk_tolerance * 0.2f)) {
                return ACTION_CALL; // This will be all-in
            } else {
                return ACTION_FOLD;
            }
        }
        
        // Normal betting situation with improved logic
        if(hand_strength > 0.7f || 
           (hand_strength > 0.4f && ai_should_bluff(ai_player, game, player_index))) {
            return ACTION_RAISE;
        } else if(hand_strength > 0.3f || 
                  (pot_odds < 0.3f && hand_strength > 0.2f)) {
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

float ai_calculate_pot_odds(GameState* game, uint8_t player_index) {
    Player* player = &game->players[player_index];
    uint32_t call_amount = game->current_bet - player->bet;
    
    if(call_amount == 0) return 0.0f;
    
    return (float)call_amount / (game->pot + call_amount);
}

float ai_evaluate_position(GameState* game, uint8_t player_index) {
    // Calculate position relative to dealer
    uint8_t position = (player_index - game->dealer + MAX_PLAYERS) % MAX_PLAYERS;
    
    // Later position is better (closer to 1.0)
    switch(position) {
        case 0: return 0.1f; // Small blind (worst position)
        case 1: return 0.2f; // Big blind
        case 2: return 0.6f; // Early position
        case 3: return 1.0f; // Button (best position)
        default: return 0.5f;
    }
}

void ai_update_opponent_model(AIPlayer* ai_player, uint8_t opponent_id, PlayerAction action) {
    if(opponent_id >= MAX_PLAYERS) return;
    
    // Update aggression estimation based on action
    switch(action) {
        case ACTION_FOLD:
            ai_player->opponent_aggression[opponent_id] *= 0.95f; // Slightly less aggressive
            break;
        case ACTION_CHECK:
            // No change in aggression estimate
            break;
        case ACTION_CALL:
            ai_player->opponent_aggression[opponent_id] = 
                (ai_player->opponent_aggression[opponent_id] * 0.9f) + (0.4f * 0.1f);
            break;
        case ACTION_RAISE:
            ai_player->opponent_aggression[opponent_id] = 
                (ai_player->opponent_aggression[opponent_id] * 0.8f) + (0.8f * 0.2f);
            break;
    }
    
    // Keep values in range [0.1, 0.9]
    if(ai_player->opponent_aggression[opponent_id] < 0.1f) {
        ai_player->opponent_aggression[opponent_id] = 0.1f;
    }
    if(ai_player->opponent_aggression[opponent_id] > 0.9f) {
        ai_player->opponent_aggression[opponent_id] = 0.9f;
    }
}

bool ai_should_fold_to_aggression(AIPlayer* ai_player, GameState* game, uint8_t player_index) {
    // Check if facing a very aggressive opponent
    uint8_t aggressor = game->current_player;
    for(uint8_t i = 0; i < MAX_PLAYERS; i++) {
        if(game->players[i].last_action == ACTION_RAISE && !game->players[i].folded) {
            aggressor = i;
            break;
        }
    }
    
    if(aggressor < MAX_PLAYERS && ai_player->opponent_aggression[aggressor] > 0.7f) {
        // Facing a very aggressive player - be more cautious
        return ai_player->risk_tolerance < 0.4f;
    }
    
    return false;
}