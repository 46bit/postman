Love Letter Manager
===================

This is the game manager for the HackSoc Love Letter AI competition in
Summer term. This is provided, along with some sample AIs, as a
reference implementation of the rules which people should make use of
when implementing their own AIs.

How to Play
-----------

Love Letter is a game for 2 to 4 players. At the start of the game,
the deck is shuffled and every player is dealt one card. The top card
of the deck is then set aside, face down. This prevents players from
having perfect information.

On a player's turn they draw a card, and choose one of their two cards
to play. All cards are played face up in front of the player who
played them.

A round ends when a player's turn ends and there are no more cards
left in the deck, or when all of the players but one are out. At this
point, all of the players still in compare hands, and the player with
the highest-valued card wins.

The player who won becomes the first player in the next round of the
game.

A player must win four rounds to win a game, and so the maximum number
of rounds in a game is 13.

*If you want to try the game out a bit before writing your AI, come
along to CoffeeScript, to which barrucadu often brings the game.*

### Cards

There are 8 types of cards in the game, worth varying amounts of
points.

If a card mentions a "target player", that cannot be the person
playing the card, with the exception of the **Wizard**.

 - **Princess** (8 points, 1 in deck) - if you play or discard it, you
   are out.

 - **Minister** (7 points, 1 in deck) - if your hand is worth 12 or
   more, you are out.

 - **General** (6 points, 1 in deck) - change hands with the target
   player.

 - **Wizard** (5 points, 2 in deck) - target player discards their
     hand and draws a new card.

 - **Priestess** (4 points, 2 in deck) - ignore all cards targeting
   you until your next turn.

 - **Knight** (3 points, 2 in deck) - compare hands with target
     player, lowest hand is out, if hands are equal nobody is out.

 - **Clown** (2 points, 2 in deck) - look at target player's hand.

 - **Soldier** (1 point, 5 in deck) - name a non-soldier card, if the
   target player has that card, they are out.

AIs
---

AIs can be implemented in whatever language you want, but we must be
able to compile and run them. If in doubt, produce something which
works on the ITS Linux machines.

### Communication

The manager and AIs will communicate over stdin and stdout. After
launching an AI, the manager will first send it its player number, and
then which card it draws for its initial hand.

If an AI is knocked out (or forfeits), it will be sent SIGTERM.

All AIs will be executed anew at the start of a round. SIGTERM will be
sent to all AIs when a round ends.

### Messages

Messages that the manager may send an AI:

 - `player <n>` - it is the turn of player `n`.

 - `draw <card>` - you draw the named card into your hand.

 - `swap <card>` - you replace the card in your hand with the named
   card.

 - `played <a> <card> [<b>] [<query>]` - player `a` played the named
   card, on player `b` if it has a target, with the named query if it
   has one.

 - `reveal <n> <card>` - player `n` reveals the named card from their
   hand, without discarding it.

 - `discard <n> <card>` - player `n` reveals the named card from their
   hand, discarding it.

 - `out <n> <cards>` - player `n` is out, followed by a
   space-delimited list of the cards in their hand. This will be 0 (if
   they played a princess, for example), 1, or 2 (if they had
   previously played a minister, for example) values.

Messages that you may send the manager on your turn:

 - `play <card> [<b>] [<query>]` - same as `played` above, but you
   don't need to specify your player number.

 - `forfeit` - you forfeit the game.

In addition, a malformed `play` message will be interpreted as a
forfeit. An illegal play also counts as a forfeit.

Card names are taken to be case-insensitive in messages, however the
manager will only use lower-case names in messages it sends, to
remove needless work from the AI authors.

### Card Messages

All players receive the `played` message, but some cards cause
messages to then follow.

 - **Princess** - `out`

 - **Minister** - may cause an `out` after `draw`

 - **General** - both players receive a `swap`

 - **Wizard** - `discard` from target player, `draw` to target player
   if there are cards left, otherwise `out`.

 - **Priestess** - manager will discard messages as appropriate

 - **Knight** - both players receive a `reveal`, and then one `out`
   unless players have equal cards.

 - **Clown** - `reveal`

 - **Soldier** - `out` if guess was correct

### Example

Within brackets are the sender and receiver of the message, and within
braces are comments indicating what's going on.

    { set up }
    [manager -> 1]   1
    [manager -> 1]   draw soldier
    [manager -> 2]   2
    [manager -> 2]   draw wizard
    [manager -> 3]   3
    [manager -> 3]   draw princess
    [manager -> 4]   4
    [manager -> 4]   draw knight

    { player 1's turn }
    [manager -> all] player 1
    [manager -> 1]   draw minister
    [1 -> manager]   play soldier 2 princess
    [manager -> all] played 1 soldier 2 princess

    { player 2's turn }
    [manager -> all] player 2
    [manager -> 2]   draw clown
    [2 -> manager]   play wizard 3
    [manager -> all] played 2 wizard 3
    [manager -> all] discard 3 princess
    [manager -> all] out 3
    { ai 3 receives sigterm }

    { player 4's turn }
    [manager -> all] player 4
    [manager -> 4]   draw priestess
    [4 -> manager]   play knight 1
    [manager -> all] played 4 knight 1
    [manager -> 1]   reveal 4 priestess
    [manager -> 4]   reveal 1 minister
    [manager -> all] out 4 priestess
    { ai 4 receives sigterm }
