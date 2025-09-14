#include "poker.h"
#include <furi.h>
#include <stdlib.h>

void poker_init_deck(Deck* deck) {
    uint8_t index = 0;
    
    // Initialize deck with all 52 cards
    for(uint8_t suit = SUIT_HEARTS; suit <= SUIT_SPADES; suit++) {
        for(uint8_t rank = RANK_2; rank <= RANK_ACE; rank++) {
            deck->cards[index].suit = (CardSuit)suit;
            deck->cards[index].rank = (CardRank)rank;
            index++;
        }
    }
    
    deck->top = 0;
}

void poker_shuffle_deck(Deck* deck) {
    // Fisher-Yates shuffle algorithm
    for(uint8_t i = DECK_SIZE - 1; i > 0; i--) {
        uint8_t j = rand() % (i + 1);
        
        // Swap cards[i] and cards[j]
        Card temp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = temp;
    }
    
    deck->top = 0;
}

Card poker_deal_card(Deck* deck) {
    if(deck->top >= DECK_SIZE) {
        // Reset deck if we've run out of cards (shouldn't happen in normal play)
        deck->top = 0;
    }
    
    return deck->cards[deck->top++];
}

void poker_get_card_string(Card card, char* buffer, size_t buffer_size) {
    const char* rank_chars = "23456789TJQKA";
    const char* suit_chars = "HDCS";
    
    if(buffer_size >= 3) {
        buffer[0] = rank_chars[card.rank - 2];
        buffer[1] = suit_chars[card.suit];
        buffer[2] = '\0';
    }
}

// Helper function to count occurrences of each rank
static void count_ranks(Card* all_cards, uint8_t count, uint8_t* rank_counts) {
    for(uint8_t i = 0; i < 15; i++) {
        rank_counts[i] = 0;
    }
    
    for(uint8_t i = 0; i < count; i++) {
        rank_counts[all_cards[i].rank]++;
    }
}

// Helper function to count occurrences of each suit
static void count_suits(Card* all_cards, uint8_t count, uint8_t* suit_counts) {
    for(uint8_t i = 0; i < 4; i++) {
        suit_counts[i] = 0;
    }
    
    for(uint8_t i = 0; i < count; i++) {
        suit_counts[all_cards[i].suit]++;
    }
}

// Check for flush
static bool is_flush(uint8_t* suit_counts, CardSuit* flush_suit) {
    for(uint8_t i = 0; i < 4; i++) {
        if(suit_counts[i] >= 5) {
            *flush_suit = (CardSuit)i;
            return true;
        }
    }
    return false;
}

// Check for straight
static bool is_straight(uint8_t* rank_counts, uint8_t* straight_high) {
    // Check for regular straight
    for(uint8_t i = RANK_ACE; i >= RANK_6; i--) {
        bool found = true;
        for(uint8_t j = 0; j < 5; j++) {
            if(rank_counts[i - j] == 0) {
                found = false;
                break;
            }
        }
        if(found) {
            *straight_high = i;
            return true;
        }
    }
    
    // Check for A-2-3-4-5 straight (wheel)
    if(rank_counts[RANK_ACE] && rank_counts[RANK_2] && rank_counts[RANK_3] && 
       rank_counts[RANK_4] && rank_counts[RANK_5]) {
        *straight_high = RANK_5;
        return true;
    }
    
    return false;
}

HandResult poker_evaluate_hand(Card* hand, Card* community, uint8_t community_count) {
    HandResult result = {0};
    Card all_cards[7]; // 2 hole cards + 5 community cards max
    uint8_t total_cards = HAND_SIZE + community_count;
    
    // Combine hole cards and community cards
    for(uint8_t i = 0; i < HAND_SIZE; i++) {
        all_cards[i] = hand[i];
    }
    for(uint8_t i = 0; i < community_count; i++) {
        all_cards[HAND_SIZE + i] = community[i];
    }
    
    // Count ranks and suits
    uint8_t rank_counts[15] = {0};
    uint8_t suit_counts[4] = {0};
    count_ranks(all_cards, total_cards, rank_counts);
    count_suits(all_cards, total_cards, suit_counts);
    
    // Check for flush and straight
    CardSuit flush_suit;
    bool has_flush = is_flush(suit_counts, &flush_suit);
    uint8_t straight_high;
    bool has_straight = is_straight(rank_counts, &straight_high);
    
    // Determine hand rank
    if(has_straight && has_flush) {
        // Check if it's specifically a straight flush in the flush suit
        bool is_straight_flush = true;
        for(uint8_t i = 0; i < 5; i++) {
            bool found = false;
            uint8_t check_rank = (straight_high == RANK_5 && i == 0) ? RANK_ACE : straight_high - i;
            
            for(uint8_t j = 0; j < total_cards; j++) {
                if(all_cards[j].rank == check_rank && all_cards[j].suit == flush_suit) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                is_straight_flush = false;
                break;
            }
        }
        
        if(is_straight_flush) {
            if(straight_high == RANK_ACE) {
                result.rank = HAND_ROYAL_FLUSH;
            } else {
                result.rank = HAND_STRAIGHT_FLUSH;
            }
            result.value = straight_high;
            return result;
        }
    }
    
    // Check for four of a kind
    for(uint8_t i = RANK_ACE; i >= RANK_2; i--) {
        if(rank_counts[i] == 4) {
            result.rank = HAND_FOUR_KIND;
            result.value = i << 4;
            // Find kicker
            for(uint8_t j = RANK_ACE; j >= RANK_2; j--) {
                if(rank_counts[j] > 0 && j != i) {
                    result.value |= j;
                    break;
                }
            }
            return result;
        }
    }
    
    // Check for full house
    uint8_t trips = 0, pair = 0;
    for(uint8_t i = RANK_ACE; i >= RANK_2; i--) {
        if(rank_counts[i] == 3 && trips == 0) {
            trips = i;
        } else if(rank_counts[i] >= 2 && pair == 0) {
            pair = i;
        }
    }
    if(trips && pair) {
        result.rank = HAND_FULL_HOUSE;
        result.value = (trips << 4) | pair;
        return result;
    }
    
    // Check for flush
    if(has_flush) {
        result.rank = HAND_FLUSH;
        // Find highest 5 cards of flush suit
        uint8_t flush_cards[5];
        uint8_t flush_count = 0;
        for(uint8_t i = RANK_ACE; i >= RANK_2 && flush_count < 5; i--) {
            for(uint8_t j = 0; j < total_cards; j++) {
                if(all_cards[j].rank == i && all_cards[j].suit == flush_suit) {
                    flush_cards[flush_count++] = i;
                    break;
                }
            }
        }
        result.value = (flush_cards[0] << 16) | (flush_cards[1] << 12) | 
                      (flush_cards[2] << 8) | (flush_cards[3] << 4) | flush_cards[4];
        return result;
    }
    
    // Check for straight
    if(has_straight) {
        result.rank = HAND_STRAIGHT;
        result.value = straight_high;
        return result;
    }
    
    // Check for three of a kind
    if(trips) {
        result.rank = HAND_THREE_KIND;
        result.value = trips << 8;
        // Find two highest kickers
        uint8_t kickers = 0;
        for(uint8_t i = RANK_ACE; i >= RANK_2 && kickers < 2; i--) {
            if(rank_counts[i] > 0 && i != trips) {
                result.value |= i << (4 * (1 - kickers));
                kickers++;
            }
        }
        return result;
    }
    
    // Check for pairs
    uint8_t pairs[2] = {0, 0};
    uint8_t pair_count = 0;
    for(uint8_t i = RANK_ACE; i >= RANK_2 && pair_count < 2; i--) {
        if(rank_counts[i] == 2) {
            pairs[pair_count++] = i;
        }
    }
    
    if(pair_count == 2) {
        result.rank = HAND_TWO_PAIR;
        result.value = (pairs[0] << 8) | (pairs[1] << 4);
        // Find kicker
        for(uint8_t i = RANK_ACE; i >= RANK_2; i--) {
            if(rank_counts[i] > 0 && i != pairs[0] && i != pairs[1]) {
                result.value |= i;
                break;
            }
        }
        return result;
    }
    
    if(pair_count == 1) {
        result.rank = HAND_PAIR;
        result.value = pairs[0] << 12;
        // Find three highest kickers
        uint8_t kickers = 0;
        for(uint8_t i = RANK_ACE; i >= RANK_2 && kickers < 3; i--) {
            if(rank_counts[i] > 0 && i != pairs[0]) {
                result.value |= i << (4 * (2 - kickers));
                kickers++;
            }
        }
        return result;
    }
    
    // High card
    result.rank = HAND_HIGH_CARD;
    uint8_t high_cards = 0;
    for(uint8_t i = RANK_ACE; i >= RANK_2 && high_cards < 5; i--) {
        if(rank_counts[i] > 0) {
            result.value |= i << (4 * (4 - high_cards));
            high_cards++;
        }
    }
    
    return result;
}

int poker_compare_hands(HandResult* hand1, HandResult* hand2) {
    if(hand1->rank > hand2->rank) return 1;
    if(hand1->rank < hand2->rank) return -1;
    
    if(hand1->value > hand2->value) return 1;
    if(hand1->value < hand2->value) return -1;
    
    return 0; // Tie
}