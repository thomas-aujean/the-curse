#!/bin/sh
# PC tests of the game logic (no VDP). From the project root:  wsl sh tools/test.sh
set -e
mkdir -p out
CC="gcc -std=c99 -Wall -Wextra -Werror -DPC_TEST -Iinc"
LOGIC="src/game_level.c src/rooms.c src/game_floor.c src/game_player.c src/game_room.c src/game_enemy.c src/game_decor.c"
$CC tests/floor_test.c src/game_floor.c -o out/floor_test && out/floor_test
$CC tests/player_test.c src/game_player.c src/game_room.c -o out/player_test && out/player_test
$CC tests/enemy_test.c src/game_enemy.c src/game_player.c src/game_room.c -o out/enemy_test && out/enemy_test
$CC tests/level_test.c $LOGIC -o out/level_test && out/level_test
$CC tests/decor_test.c $LOGIC -o out/decor_test && out/decor_test
