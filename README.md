# F0-Texas-Holdem-
Poker Texas Holdem variation Flipper Zero app

## Description
A single-player Texas Hold'em poker game for Flipper Zero where you play against 3 AI opponents. Features complete poker rules, betting rounds, hand evaluation, and an intuitive interface designed for the Flipper Zero's screen and controls.

## Features
- **Complete Texas Hold'em gameplay**: Pre-flop, flop, turn, river, and showdown
- **3 AI opponents** with different personalities (Conservative, Aggressive, Random)
- **Full betting system**: Fold, check/call, raise with pot management
- **Hand evaluation**: All standard poker hands from high card to royal flush
- **Chip tracking**: Starting with $1000 chips per player
- **Visual interface**: Card display, player info, pot size, and betting status

## Game Controls
- **Left/Right**: Navigate menu options (Fold, Check/Call, Raise)
- **OK**: Confirm selected action
- **Back**: Exit game

## Game Flow
1. Each player starts with $1000 in chips
2. Two hole cards are dealt to each player
3. Pre-flop betting round
4. Flop (3 community cards) + betting round
5. Turn (1 community card) + betting round  
6. River (1 community card) + betting round
7. Showdown - best hand wins the pot
8. New hand begins with dealer button rotation

## Building
This app is designed to be built with the Flipper Zero firmware build system (uFBT).

```bash
# Build the app
ufbt

# Install to connected Flipper Zero
ufbt launch
```

## File Structure
- `main.c` - Game loop, input handling, state management
- `poker.c/h` - Deck handling, card dealing, hand evaluation
- `ai.c/h` - AI opponent logic and decision making
- `ui.c/h` - Display rendering and user interface
- `application.fam` - Flipper app manifest
- `assets/` - App icon and resources

## AI Behavior
- **AI Player 1 (Conservative)**: Plays tight, folds weak hands, bets cautiously
- **AI Player 2 (Aggressive)**: Bets and raises frequently, bluffs more often
- **AI Player 3 (Random/Balanced)**: Balanced play style with some unpredictability

The AI evaluates hand strength based on current cards and community cards, adjusts betting based on pot odds, and incorporates bluffing behavior.
