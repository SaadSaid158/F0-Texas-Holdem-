#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

#define MAX_PLAYERS 4
#define STARTING_CHIPS 1000
#define DECK_SIZE 52
#define HAND_SIZE 2
#define COMMUNITY_SIZE 5

// Card suits
typedef enum {
    SUIT_HEARTS = 0,
    SUIT_DIAMONDS,
    SUIT_CLUBS,
    SUIT_SPADES
} CardSuit;

// Card ranks (2-14, where 14 = Ace)
typedef enum {
    RANK_2 = 2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8,
    RANK_9,
    RANK_10,
    RANK_JACK,
    RANK_QUEEN,
    RANK_KING,
    RANK_ACE
} CardRank;

// Card structure
typedef struct {
    CardRank rank;
    CardSuit suit;
} Card;

// Hand rankings
typedef enum {
    HAND_HIGH_CARD = 0,
    HAND_PAIR,
    HAND_TWO_PAIR,
    HAND_THREE_KIND,
    HAND_STRAIGHT,
    HAND_FLUSH,
    HAND_FULL_HOUSE,
    HAND_FOUR_KIND,
    HAND_STRAIGHT_FLUSH,
    HAND_ROYAL_FLUSH
} HandRank;

// Hand evaluation result
typedef struct {
    HandRank rank;
    uint32_t value; // For comparing hands of same rank
    Card best_hand[5]; // Best 5-card hand
} HandResult;

// Deck structure
typedef struct {
    Card cards[DECK_SIZE];
    uint8_t top; // Index of next card to deal
} Deck;

// Player actions
typedef enum {
    ACTION_FOLD = 0,
    ACTION_CHECK,
    ACTION_CALL,
    ACTION_RAISE
} PlayerAction;

// Player structure
typedef struct {
    Card hand[HAND_SIZE];
    uint32_t chips;
    uint32_t bet;
    bool folded;
    bool all_in;
    PlayerAction last_action;
    char name[16];
} Player;

// Game phases
typedef enum {
    PHASE_PREFLOP = 0,
    PHASE_FLOP,
    PHASE_TURN,
    PHASE_RIVER,
    PHASE_SHOWDOWN
} GamePhase;

// Game state
typedef struct {
    Player players[MAX_PLAYERS];
    Card community[COMMUNITY_SIZE];
    uint8_t community_count;
    Deck deck;
    GamePhase phase;
    uint8_t current_player;
    uint8_t dealer;
    uint32_t pot;
    uint32_t current_bet;
    uint8_t active_players;
    bool game_over;
} GameState;

// Function declarations
void poker_init_deck(Deck* deck);
void poker_shuffle_deck(Deck* deck);
Card poker_deal_card(Deck* deck);
HandResult poker_evaluate_hand(Card* hand, Card* community, uint8_t community_count);
int poker_compare_hands(HandResult* hand1, HandResult* hand2);
void poker_get_card_string(Card card, char* buffer, size_t buffer_size);