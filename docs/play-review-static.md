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
postman->3: draw Minister 			==> 3 Minister
postman->all: player 0
postman->0: draw Clown 			==> 0 Clown
postman->all: player 1
postman->1: draw Clown 			==> 1 Clown
postman->all: player 2
postman->2: draw General 			==> 2 General
postman->all: begin
postman->all: player 3
postman->3: draw Knight 			==> 3 Minister Knight
3->postman: play Knight 1 			==> 3 Minister
postman->all: played Knight 1
postman->3: reveal 1 Clown
postman->1: reveal 3 Minister
postman->all: out 1 Clown 			==> Clown worth less than Minister, fine
postman->all: player 0
postman->0: draw Soldier 			==> 0 Clown Soldier
0->postman: play Soldier 1 Minister 			==> 0 Clown
postman->all: played Soldier 1 Minister
postman->all: protected 1 			==> 1 is indeed out
postman->all: player 2
postman->2: draw Soldier 			==> 2 General Soldier
2->postman: play Soldier 1 Minister 			==> 2 General
postman->all: played Soldier 1 Minister
postman->all: protected 1 			==> 1 is indeed out
postman->all: player 3
postman->3: draw Priestess 			==> 3 Minister Priestess
3->postman: play Priestess 			==> 3 Minister
Player Hello World specified an invalid player. 			==> DISAGREE
postman->all: out 3 Minister 			==> DISAGREE ^
postman->all: player 0
postman->0: draw Soldier 			==> 0 Clown Soldier
0->postman: play Clown 2 			==> 0 Soldier
postman->all: played Clown 2
postman->0: reveal General 			==> AGREE
postman->all: player 2
postman->2: draw Knight 			==> 2 General Knight
2->postman: play General 2 			==> 2 Knight
postman->all: played General 2
postman->2: swap Knight 			==> AGREE
postman->2: swap Knight 			==> AGREE (played on self)
postman->all: player 0
postman->0: draw Soldier 			==> 0 Soldier Soldier
0->postman: play Soldier 0 Clown 			==> SOLDIER ON SELF
postman->all: played Soldier 0 Clown 			==> 0 Soldier
postman->all: player 2
postman->2: draw Priestess 			==> 2 Knight Priestess
2->postman: play Knight 3 			==> 2 Priestess
postman->all: played Knight 3
postman->all: protected 3 			==> AGREE, BUT ALSO DISAGREED OUT
postman->all: player 0
postman->0: draw Wizard 			==> 0 Soldier Wizard
0->postman: play Wizard 1 			==> 0 Soldier
postman->all: played Wizard 1
postman->all: protected 1 			==> AGREE
postman->all: player 2
postman->2: draw Soldier 			==> 2 Priestess Soldier
2->postman: play Soldier 0 Knight 			==> 2 Priestess
postman->all: played Soldier 0 Knight 			==> AGREE
postman->all: player 0
postman->0: draw Wizard 			==> 0 Soldier Wizard
0->postman: play Soldier 1 Priestess 			==> 0 Wizard
postman->all: played Soldier 1 Priestess
postman->all: protected 1 			==> AGREE
postman->all: player 2
postman->2: draw Princess 			==> 2 Priestess Princess
2->postman: play Priestess
Player Hello World specified an invalid player. 			==> DISAGREE
postman->all: out 2 Princess 			==> DISAGREE ^
Player 0 Hello World won with a Wizard card. 			==> AGREE WITH CAVEATS ^
