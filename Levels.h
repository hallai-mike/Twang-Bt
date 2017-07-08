// ---------------------------------
// ------------ LEVELS -------------
// ---------------------------------

#ifndef LEVELS_H
#define LEVELS_H

#include "Enemy.h"
#include "Particle.h"
#include "Spawner.h"
#include "Lava.h"
#include "Boss.h"
#include "Conveyor.h"

void loadLevel(int levelNumber)
{
	switch(levelNumber)
	{
		case 0:
			// Intro, get user used to moving left and right
			playerPosition = 200;
			spawnEnemy(1, 0, 0, 0);
			break;

		case 1:
			// Slow moving enemy
			spawnEnemy(900, 0, 1, 0);
			break;

		case 2:
			// Spawning enemies at exit every 3 seconds
			spawnPool[0].Spawn(1000, 3000, 2, 0, 0);
			break;

		case 3:
			// Lava intro
			spawnLava(400, 490, 2000, 2000, 0, "OFF");
			spawnPool[0].Spawn(1000, 5500, 3, 0, 0);
			break;

		case 4:
			// 2 Sin enemies
			spawnEnemy(700, 1, 7, 275);
			spawnEnemy(500, 1, 5, 250);
			break;

		case 5:
			// Conveyor and stationary enemy
			spawnConveyor(100, 600, -1);
			spawnEnemy(800, 0, 0, 0);
			break;

		case 6:
			// water towards lots of enemies
			spawnConveyor(50, 1000, 1);
			spawnEnemy(300, 0, 0, 0);
			spawnEnemy(400, 0, 0, 0);
			spawnEnemy(500, 0, 0, 0);
			spawnEnemy(600, 0, 0, 0);
			spawnEnemy(700, 0, 0, 0);
			spawnEnemy(800, 0, 0, 0);
			spawnEnemy(900, 0, 0, 0);
			break;

		case 7:
			// Conveyor and lava pit
			spawnConveyor(200, 600, -1);
			spawnLava(601, 710, 1500, 2000, 0, "OFF");
			spawnEnemy(800, 0, 0, 0);
			break;

		case 8:
			spawnLava(195, 800, 1000, 2800, 0, "OFF");
			break;

		case 9:
			// Sin enemy #2
			spawnEnemy(700, 1, 7, 275);
			spawnEnemy(500, 1, 5, 250);
			spawnConveyor(100, 900, -1);
			break;

		case 10:
			// 4 lava pits
			spawnLava(195, 300, 2000, 2000, 0, "OFF");
			spawnLava(350, 455, 2000, 2000, 0, "OFF");
			spawnLava(510, 610, 2000, 2000, 0, "OFF");
			spawnLava(660, 760, 2000, 2000, 0, "OFF");
			spawnPool[0].Spawn(1000, 3800, 4, 0, 0);
			break;

		case 11:
			// Sin enemy #2
			spawnEnemy(700, 1, 7, 275);
			spawnEnemy(500, 1, 5, 250);
			spawnPool[0].Spawn(1000, 5500, 4, 0, 3000);
			spawnPool[1].Spawn(0, 5500, 5, 1, 10000);
			spawnConveyor(100, 300, -1);
			spawnConveyor(301, 600, 1);
			break;

		case 12:
			spawnLava(110, 400, 1000, 2800, 0, "ON");
			spawnLava(405, 845, 1000, 2800, 0, "OFF");
			break;

		case 13:
			spawnLava(110, 200, 1000, 2800, 0, "OFF");
			spawnLava(510, 600, 1000, 2800, 0, "ON");
			spawnPool[0].Spawn(1000, 3200, 5, 0, 0);
			spawnPool[1].Spawn(1000, 3200, 4, 0, 300);
			break;

		case 14:
			// 4 Lava pits with 2 sin enemies
			spawnEnemy(700, 1, 7, 275);
			spawnEnemy(500, 1, 5, 250);
			spawnLava(195, 300, 2000, 2000, 0, "OFF");
			spawnConveyor(304, 336, -1);
			spawnLava(340, 455, 2000, 2000, 0, "OFF");
			spawnLava(500, 610, 2000, 2000, 0, "OFF");
			spawnLava(650, 760, 2000, 2000, 0, "OFF");
			spawnPool[0].Spawn(1000, 3800, 4, 0, 0);
			break;

		case 15:
			spawnEnemy(700, 1, 7, 275);
			spawnEnemy(500, 1, 5, 250);
			spawnEnemy(400, 1, 7, 175);
			spawnEnemy(300, 1, 5, 100);
			spawnEnemy(800, 1, 9, 100);
			break;

		case 16:
			// Boss
			spawnBoss();
			break;

		default:
			playerPosition = 200;
			spawnEnemy(1, 0, 0, 0);
			break;
	}
}


#endif