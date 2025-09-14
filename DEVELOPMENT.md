# Texas Hold'em Flipper Zero App - Development Notes

## Build Requirements
- Flipper Zero firmware development environment
- uFBT (micro Flipper Build Tool)
- Git for version control

## Building the App
```bash
# Clone the repository
git clone https://github.com/SaadSaid158/F0-Texas-Holdem-.git
cd F0-Texas-Holdem-

# Build with uFBT
ufbt

# Launch on connected Flipper Zero
ufbt launch
```

## File Overview

### Core Files
- **application.fam**: App manifest defining metadata and entry point
- **main.c**: Game loop, state management, input handling, timer callbacks
- **poker.c/h**: Card deck, shuffling, dealing, hand evaluation algorithms
- **ai.c/h**: AI opponent decision making with different personalities
- **ui.c/h**: Screen rendering, card display, menu system

### Key Data Structures
- `GameState`: Manages players, community cards, pot, current phase
- `Player`: Individual player data (cards, chips, betting status)
- `Deck`: 52-card deck with shuffling and dealing
- `HandResult`: Poker hand evaluation with ranking and tie-breaking

### Game Flow
1. **Initialization**: Set up 4 players (1 human + 3 AI) with $1000 chips each
2. **New Hand**: Shuffle deck, deal 2 hole cards to each player
3. **Betting Rounds**: Pre-flop → Flop → Turn → River with player actions
4. **Showdown**: Evaluate hands, determine winner, distribute pot
5. **Repeat**: Continue until human player runs out of chips

### AI Personalities
- **Conservative**: Tight play, low aggression, minimal bluffing
- **Aggressive**: Frequent betting/raising, higher bluff rate
- **Random**: Balanced strategy with unpredictable elements

## Development Notes

### Hand Evaluation
The poker hand evaluation algorithm handles all standard poker hands:
- High Card through Royal Flush
- Proper tie-breaking with kicker evaluation
- Supports both hole cards + community cards evaluation

### Betting Logic
- Supports fold, check, call, raise actions
- Handles all-in situations when chips are limited
- Pot management with proper side pot handling
- Minimum raise enforcement

### UI Considerations
- Optimized for 128x64 pixel monochrome display
- Clear card representation using 2-character abbreviations
- Menu navigation with D-pad and OK button
- Real-time game state display

## Testing
Core game logic has been validated with standalone compilation tests.
The poker engine correctly handles deck operations, card dealing, and basic game flow.

## Future Enhancements
- Add tournament mode with blinds escalation
- Implement side pots for complex all-in situations
- Add statistics tracking (hands won, biggest pot, etc.)
- Sound effects and animations
- Multiplayer support via RF communication