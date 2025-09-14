# F0-Texas-Holdem-
Enhanced Poker Texas Holdem variation Flipper Zero app with advanced AI and realistic gameplay

## Description
A single-player Texas Hold'em poker game for Flipper Zero where you play against 3 AI opponents. Features complete poker rules, betting rounds, hand evaluation, and an intuitive interface designed for the Flipper Zero's screen and controls.

**🆕 New in Enhanced Version:**
- **Advanced AI System**: Position-aware AI with opponent modeling and adaptive strategies
- **Realistic Blinds System**: Small/big blind rotation for authentic poker experience  
- **Enhanced UI**: Hand tracking, blind indicators, and improved status display
- **Smart Betting Logic**: Pot odds calculation and stack size considerations
- **Professional Icon**: Custom 10x10 PNG icon for Flipper Zero

## Features
- **Complete Texas Hold'em gameplay**: Pre-flop, flop, turn, river, and showdown
- **3 Enhanced AI opponents** with distinct personalities and adaptive learning:
  - **Conservative AI**: Tight play with position awareness
  - **Aggressive AI**: High aggression with advanced bluffing
  - **Balanced AI**: Unpredictable mixed strategy
- **Realistic blinds system**: $10 small blind, $20 big blind with proper rotation
- **Advanced betting system**: Fold, check/call, raise with pot odds and position strategy
- **Complete hand evaluation**: All standard poker hands from high card to royal flush
- **Chip tracking**: Starting with $1000 chips per player
- **Enhanced visual interface**: Hand numbers, blind positions, last actions, and game statistics

## Game Controls
- **Left/Right**: Navigate menu options (Fold, Check/Call, Raise)
- **OK**: Confirm selected action
- **Back**: Exit game

## Game Flow
1. Each player starts with $1000 in chips
2. Blinds are posted ($10 SB, $20 BB) and rotate each hand
3. Two hole cards are dealt to each player
4. Pre-flop betting round (starting with player after big blind)
5. Flop (3 community cards) + betting round
6. Turn (1 community card) + betting round  
7. River (1 community card) + betting round
8. Showdown - best hand wins the pot
9. New hand begins with dealer button rotation

## AI Intelligence
The enhanced AI system includes:
- **Position Strategy**: Early vs late position play adaptation
- **Opponent Modeling**: Tracking and adapting to other players' tendencies
- **Stack Management**: Short stack vs big stack strategic adjustments  
- **Phase Awareness**: Pre-flop through river strategy modifications
- **Bluffing Logic**: Situational bluffing based on position and opponents
- **Risk Assessment**: Individual risk tolerance per AI personality

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
- **AI Player 1 (Conservative)**: Plays tight with position awareness, folds to aggression, calculates pot odds carefully
- **AI Player 2 (Aggressive)**: Bets and raises frequently, advanced bluffing, less affected by opponent pressure  
- **AI Player 3 (Balanced/Random)**: Mixed strategy with unpredictable elements, adapts to table dynamics

Each AI uses advanced decision-making that considers:
- Hand strength evaluation with community cards
- Position relative to dealer button
- Opponent aggression patterns and adaptation
- Stack size and pot odds calculations  
- Phase-specific strategy (pre-flop vs river play)
- Risk tolerance and bluffing frequency

The AI continuously learns and adapts to opponents throughout the game, making each session unique and challenging.
