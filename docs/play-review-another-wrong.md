postman->0: ident 0 3 4
0->postman: Hello World
postman->1: ident 1 3 4
1->postman: Hello World
postman->2: ident 2 3 4
2->postman: Hello World
postman->3: ident 3 3 4
3->postman: Hello World
postman->all: begin
postman->all: player 3
postman->3: draw Soldier
postman->all: player 0
postman->0: draw General
postman->all: player 1
postman->1: draw Wizard
postman->all: player 2
postman->2: draw Soldier
postman->all: begin
postman->all: player 3
postman->3: draw Clown
3->postman: play Clown 1
postman->all: played Clown 1
postman->3: reveal Wizard
postman->all: player 0
postman->all: out 0 General Minister
postman->0: draw Minister
postman->all: player 1
postman->1: draw Clown
1->postman: play Clown 2
postman->all: played Clown 2
postman->1: reveal Soldier
postman->all: player 2
postman->2: draw Soldier
2->postman: play Soldier 2 Priestess
postman->all: played Soldier 2 Priestess
postman->all: player 3
postman->3: draw Priestess
3->postman: play Soldier 2 Knight
postman->all: played Soldier 2 Knight
postman->all: player 1
postman->1: draw Princess
1->postman: play Princess
postman->all: played Princess
postman->all: out 1 Wizard
postman->all: player 2
postman->2: draw Priestess
2->postman: play Soldier 3 General
postman->all: played Soldier 3 General
postman->all: player 3
postman->3: draw Knight
3->postman: play Priestess
postman->all: played Priestess
postman->all: player 2
postman->2: draw Soldier
2->postman: play Soldier 1 Clown
postman->all: played Soldier 1 Clown
postman->all: protected 1
postman->all: player 3
postman->3: draw Wizard
3->postman: play Wizard 2
postman->all: played Wizard 2
postman->all: discard 2 Priestess
postman->2: draw Knight
postman->all: player 2
postman->2: draw Soldier
2->postman: play Knight 3
postman->all: played Knight 3				==> I THINK SOMETHING HERE IS WRONG
postman->2: reveal 3 Knight
postman->3: reveal 2 Priestess
postman->all: out 3 Knight
Player 2 Hello World won with a Priestess card.
