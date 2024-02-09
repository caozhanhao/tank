<h2 align="center">
Tank
</h2> 

<p align="center">
<strong>A Multiplayer Cross-platform Game</strong>
</p>

### Intro:

In Tank, you will take control of a powerful tank in a maze, showcasing your strategic skills on the infinite map and
overcome unpredictable obstacles. You can play solo or team up with friends.

### Example

![Game](examples/game-example.png)
![Status](examples/status-example.png)

### Tutorial

#### Control

- Move: WASD or direction keys
- Attack: space
- Tank Status: 'o' or 'O'
- Command: '/'

#### Tank

User's Tank:

- HP: 10000, Lethality: 100
- Auto Tank:
- HP: (11 - level) * 150, Lethality: (11 - level) * 15
- The higher level the tank is, the faster it moves.

#### Command

help [page]

- Get this help.
- Use 'Enter' to return game.

quit

- Quit Tank.

fill [Status] [A x,y] [B x,y optional]

- Status: [0] Empty [1] Wall
- Fill the area from A to B as the given Status.
- B defaults to the same as A
- e.g. fill 1 0 0 10 10 | fill 1 0 0

tp [A id] ([B id] or [B x,y])

- Teleport A to B
- A should be alive, and there should be space around B.
- e.g. tp 0 1 | tp 0 1 1

revive [A id optional]

- Revive A.
- Default to revive all tanks.

summon [n] [level]

- Summon n tanks with the given level.
- e.g. summon 50 10

kill [A id optional]

- Kill A.
- Default to kill all tanks.

clear [A id optional]

- Clear A.(only Auto Tank)
- Default to clear all auto tanks.

clear death

- Clear all the died Auto Tanks  
  Note:
  Clear is to delete rather than to kill, so the cleared tank can't revive. And the bullets of the cleared tank will
  also be cleared.

set [A id] [key] [value]

- Set A's attribute below:
- max_hp (int): Max hp of A. This will take effect when A is revived.
- hp (int): hp of A. This takes effect immediately but won't last when A is revived.
- target (id, int): Auto Tank's target. Target should be alive.
- name (string): Name of A. set [A id] bullet [key] [value]
- hp (int): hp of A's bullet.
- lethality (int): lethality of A's bullet. (negative to increase hp)
- range (int): range of A's bullet.(default)
- e.g. set 0 max_hp 1000 | set 0 bullet lethality 10  
  Note:
  When a bullet hits the wall, its hp decreases by one. That means it can bounce "hp - 1" times.

set tick [tick]

- tick (int, milliseconds): minimum time of the game's(or server's) mainloop.

set msg_ttl [ttl]

- ttl (int, milliseconds): a message's time to live. set seed [seed]
- seed (int): the game map's seed.

tell [A id optional] [msg]

- Send a message to A.
- id defaults to be -1, in which case all the players will receive the message.
- msg (string): the message's content.

server start [port]

- Start Tank Server.
- port (int): the server's port. server stop
- Stop Tank Server.

connect [ip] [port]

- Connect to Tank Server.
- ip (string): the server's IP.
- port (int): the server's port.

disconnect

- Disconnect from the Server.

### Dependencies

- Requires C++ 20