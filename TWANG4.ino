// Required libs
#include "FastLED.h"
#include "I2Cdev.h"
//#include "MPU6050.h"
#include "Wire.h"
#include "toneAC.h"
//#include "iSin.h"
//#include "RunningMedian.h"

// Included libs

#include "Enemy.h"
#include "Particle.h"
#include "Spawner.h"
#include "Lava.h"
#include "Boss.h"
#include "Conveyor.h"

// gets rid of annoying "deprecated conversion from string constant blah blah" warning
#pragma GCC diagnostic ignored "-Wwrite-strings"

// LED setup
#define NUM_LEDS             300
#define DATA_PIN             3
//#define CLOCK_PIN            4
#define LED_COLOR_ORDER      GBR//GBR
int BRIGHTNESS     =         100;
#define DIRECTION            1     // 0 = right to left, 1 = left to right
#define MIN_REDRAW_INTERVAL  16    // Min redraw interval (ms) 33 = 30fps / 16 = 63fps
#define USE_GRAVITY          0     // 0/1 use gravity (LED strip going up wall)
#define BEND_POINT           1000   // 0/1000 point at which the LED strip goes up the wall

// GAME
long previousMillis = 0;           // Time of the last redraw
int levelNumber = 0;
long lastInputTime = 0;
#define TIMEOUT              30000
#define LEVEL_COUNT          16
#define MAX_VOLUME           5
long animationFrame = 0;
//iSin isin = iSin();

// JOYSTICK
#define JOYSTICK_ORIENTATION 1     // 0, 1 or 2 to set the angle of the joystick
#define JOYSTICK_DIRECTION   0     // 0/1 to flip joystick direction
#define ATTACK_THRESHOLD     10000 // The threshold that triggers an attack 30000
#define JOYSTICK_DEADZONE    5     // Angle to ignore
int joystickTilt = 0;              // Stores the angle of the joystick
int joystickWobble = 0;            // Stores the max amount of acceleration (wobble)

// WOBBLE ATTACK
#define ATTACK_WIDTH        60     // Width of the wobble attack, world is 1000 wide
#define ATTACK_DURATION     450    // Duration of a wobble attack (ms)
long attackMillis = 0;             // Time the attack started
bool attacking = 0;                // Is the attack in progress?
#define BOSS_WIDTH          40

// PLAYER
#define MAX_PLAYER_SPEED    30     // Max move speed of the player
char* stage;                       // what stage the game is at (PLAY/DEAD/WIN/GAMEOVER)
long stageStartTime;               // Stores the time the stage changed for stages that are time based
int playerPosition;                // Stores the player position
int playerPositionModifier;        // +/- adjustment to player position
bool playerAlive;
bool inWater = 0;
long killTime;
int lives = 3;

// Bluetooth
int maxSeconds = 10000; // send status message every maxSeconds
volatile boolean statusReport = false;
volatile int seconds = 0;
String inputString = "";
String command = "";
String value = "";
boolean stringComplete = false;

// POOLS
int lifeLEDs[3] = {52, 50, 40};
Enemy enemyPool[10] = {
    Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy()
};
int const enemyCount = 10;
Particle particlePool[40] = {
    Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle()
};
int const particleCount = 40;
Spawner spawnPool[2] = {
    Spawner(), Spawner()
};
int const spawnCount = 2;
Lava lavaPool[4] = {
    Lava(), Lava(), Lava(), Lava()
};
int const lavaCount = 4;
Conveyor conveyorPool[2] = {
    Conveyor(), Conveyor()
};
int const conveyorCount = 2;
Boss boss = Boss();

CRGB leds[NUM_LEDS];
//RunningMedian MPUAngleSamples = RunningMedian(5);
//RunningMedian MPUWobbleSamples = RunningMedian(5);

void setup() {
    Serial.begin(9600);
    while (!Serial);
    
    // Setup Bluetooth
    inputString.reserve(50);
    command.reserve(50);
    value.reserve(50);
    cli();          // disable global interrupts
    // initialize Timer1 for interrupt @ 1000 msec
    TCCR1A = 0;     // set entire TCCR1A register to 0
    TCCR1B = 0;     // same for TCCR1B
    // set compare match register to desired timer count:
    OCR1A = 15624;
    // turn on CTC mode:
    TCCR1B |= (1 << WGM12);
    // Set CS10 and CS12 bits for 1024 prescaler:
    TCCR1B |= (1 << CS10);
    TCCR1B |= (1 << CS12);
    // enable timer compare interrupt:
    TIMSK1 |= (1 << OCIE1A);
    sei();          // enable global interrupts
    
    // Setup LES's
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.setDither(1);

    loadLevel();
}

// Main Game Loop
void loop() {
 
  while (Serial.available() && !stringComplete) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    //Serial.write(inChar);
    // add it to the inputString:
    inputString += inChar;
    //delay(10);
    // if the incoming character is a newline or a carriage return, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n' || inChar == '\r') {
      stringComplete = true;
    } 
  }
  if(!stringComplete){
    //inputString="";
  }
  
    long mm = millis();
    int brightness = 0;
    
    ////////////////////////////
    ////     Sound FX       ////
    ////////////////////////////
    if(stage == "PLAY"){
        if(attacking)
            SFXattacking();
        else
            toneAC();
    }
    else if(stage == "DEAD")
        SFXdead();
        
    ////////////////////////////
    ////     ScreenSaver    ////
    ////////////////////////////
    if (mm - previousMillis >= MIN_REDRAW_INTERVAL) {
        //getInput();
        long frameTimer = mm;
        previousMillis = mm;
        
        if(abs(joystickTilt) > JOYSTICK_DEADZONE){
            lastInputTime = mm;
            if(stage == "SCREENSAVER"){
                levelNumber = -1;
                stageStartTime = mm;
                animationFrame = 0;
                stage = "WIN";
            }
        }else{
            if(lastInputTime+TIMEOUT < mm){
                stage = "SCREENSAVER";
            }
        }
        if(stage == "SCREENSAVER"){
            screenSaverTick();
        }
        
        ////////////////////////////
        ////     Gameplay       ////
        ////////////////////////////
        
        // Player is playing
        else if(stage == "PLAY"){
            if(attacking && attackMillis+ATTACK_DURATION < mm) attacking = 0;

            // If still not attacking, move!
            playerPosition += playerPositionModifier;
            if(!attacking){
                int moveAmount = (joystickTilt/6.0);
                if(DIRECTION) moveAmount = -moveAmount;
                moveAmount = constrain(moveAmount, -MAX_PLAYER_SPEED, MAX_PLAYER_SPEED);
                playerPosition -= moveAmount;
                if(playerPosition < 0) playerPosition = 0;
                if(playerPosition >= 1000 && !boss.Alive()) {
                    // Reached exit!
                    levelComplete();
                    return;
                }
            }
            
            if(inLava(playerPosition)){
                die();
            }
            
            // Draw everything
            FastLED.clear();
            tickConveyors();
            tickSpawners();
            tickBoss();
            tickLava();
            tickEnemies();
            drawPlayer();
            drawAttack();
            drawExit();
        }
        // Player Died
        else if(stage == "DEAD"){
            FastLED.clear();
            if(!tickParticles()){
                loadLevel();
            }
        }
        // A Level Was Completed
        else if(stage == "WIN"){
            FastLED.clear();
            // Show next level animation
            if(animationFrame < (NUM_LEDS+15)/5){
              for(int i = NUM_LEDS-animationFrame*5; i< NUM_LEDS-animationFrame*5+15; i++){
                // If LED is in the range of 0 - NUM_LEDS then make it green.
                if((i < NUM_LEDS) && (i > 0))
                  leds[i] = CRGB(0, BRIGHTNESS, 0);
              }
            } 
            else{
              nextLevel();
              animationFrame = 0;
            }
            animationFrame++;
        }
        // The Game Was Completed/Won
        else if(stage == "COMPLETE"){
            FastLED.clear();
            SFXcomplete();
            if(stageStartTime+500 > mm){
                int n = max(map(((mm-stageStartTime)), 0, 500, NUM_LEDS, 0), 0);
                for(int i = NUM_LEDS; i>= n; i--){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else if(stageStartTime+5000 > mm){
                for(int i = NUM_LEDS; i>= 0; i--){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else if(stageStartTime+5500 > mm){
                int n = max(map(((mm-stageStartTime)), 5000, 5500, NUM_LEDS, 0), 0);
                for(int i = 0; i< n; i++){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else{
                nextLevel();
            }
        }
        else if(stage == "GAMEOVER"){
            // GAME OVER!
            FastLED.clear();
            stageStartTime = 0;
        }

        // Send data to LEDs, the end of update portion of main loop.
        FastLED.show();
    }
    
    // BT
    if (stringComplete) {
      //Serial.println(inputString);
      boolean stringOK = false;
      
      ////////////////////////////////////////////////
      /////       Input from Bluetooth           /////
      ////////////////////////////////////////////////
      
      // Attack
      if (inputString.startsWith("A")){
        if(!attacking){
                attackMillis = mm;
                attacking = 1;
                Serial.write("AAA\n");
          }
      }
      // Movement Order:  L y l o  .C.  r e t R
      else if (inputString.startsWith("l"))
        joystickTilt = -MAX_PLAYER_SPEED/2;
      else if (inputString.startsWith("r"))
        joystickTilt = MAX_PLAYER_SPEED/2;
      else if (inputString.startsWith("o"))
        joystickTilt = -MAX_PLAYER_SPEED/4;
      else if (inputString.startsWith("e"))
        joystickTilt = MAX_PLAYER_SPEED/4;
      else if (inputString.startsWith("y"))
        joystickTilt = -MAX_PLAYER_SPEED*.75;
      else if (inputString.startsWith("t"))
        joystickTilt = MAX_PLAYER_SPEED*.75;
      else if (inputString.startsWith("L"))
        joystickTilt = -MAX_PLAYER_SPEED;
      else if (inputString.startsWith("R"))
        joystickTilt = MAX_PLAYER_SPEED;
      else if (inputString.startsWith("C"))
        joystickTilt = 0;
        
      // Next level; Phone needs to set lives to 3
      else if (inputString.startsWith("N")){
        nextLevel();
      }
      // Brightness Up
      else if (inputString.startsWith("U")){
        if(BRIGHTNESS < 245)
          BRIGHTNESS = BRIGHTNESS + 10;
      }
      // Brightness Down
      else if (inputString.startsWith("D")){
        if(BRIGHTNESS > 50)
          BRIGHTNESS = BRIGHTNESS - 10;
      }
      else { 
        inputString="";
      }
      inputString = "";
      stringComplete = false;
    }
    ///////////////////////////////////////////////////////
} // end of main loop


// ---------------------------------
// ------------ LEVELS -------------
// ---------------------------------
void loadLevel(){
    //updateLives();
    cleanupLevel();
    playerPosition = 0;
    playerAlive = 1;
    switch(levelNumber){
        case 0:
            // Left or right?
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
            //spawnPool[0].Spawn(1000, 5500, 4, 0, 3000);
            //spawnPool[1].Spawn(0, 5500, 5, 1, 10000);
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
            //spawnConveyor(601, 900, -1);
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
        //    spawnPool[2].Spawn(1000, 3800, 4, 0, 800);
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
    }
    stageStartTime = millis();
    stage = "PLAY";
}

void spawnBoss(){
    boss.Spawn();
    moveBoss();
}

void moveBoss(){
    int spawnSpeed = 2100;
    if(boss._lives == 2) spawnSpeed = 1600;
    if(boss._lives == 1) spawnSpeed = 1300;
    spawnPool[0].Spawn(boss._pos, spawnSpeed, 3, 0, 0);
    spawnPool[1].Spawn(boss._pos, spawnSpeed, 3, 1, 0);
}

void spawnEnemy(int pos, int dir, int sp, int wobble){
    for(int e = 0; e<enemyCount; e++){
        if(!enemyPool[e].Alive()){
            enemyPool[e].Spawn(pos, dir, sp, wobble);
            enemyPool[e].playerSide = pos > playerPosition?1:-1;
            return;
        }
    }
}

void spawnLava(int left, int right, int ontime, int offtime, int offset, char* state){
    for(int i = 0; i<lavaCount; i++){
        if(!lavaPool[i].Alive()){
            lavaPool[i].Spawn(left, right, ontime, offtime, offset, state);
            return;
        }
    }
}

void spawnConveyor(int startPoint, int endPoint, int dir){
    for(int i = 0; i<conveyorCount; i++){
        if(!conveyorPool[i]._alive){
            conveyorPool[i].Spawn(startPoint, endPoint, dir);
            return;
        }
    }
}

void cleanupLevel(){
    for(int i = 0; i<enemyCount; i++){
        enemyPool[i].Kill();
    }
    for(int i = 0; i<particleCount; i++){
        particlePool[i].Kill();
    }
    for(int i = 0; i<spawnCount; i++){
        spawnPool[i].Kill();
    }
    for(int i = 0; i<lavaCount; i++){
        lavaPool[i].Kill();
    }
    for(int i = 0; i<conveyorCount; i++){
        conveyorPool[i].Kill();
    }
    boss.Kill();
}

void levelComplete(){
    stageStartTime = millis();
    stage = "WIN";
    if(levelNumber == LEVEL_COUNT) stage = "COMPLETE";
    lives = 3;
    //updateLives();
}

void nextLevel(){
    levelNumber ++;
    Serial.write("NNNN\n");
    playerPositionModifier = 0;
    if(levelNumber > LEVEL_COUNT) levelNumber = 0;
    loadLevel();
}

void gameOver(){
    levelNumber = 0;
    loadLevel();
}

void die(){
    Serial.write("DDD\n");
    playerAlive = 0;
    playerPositionModifier = 0;
    if(levelNumber > 0) lives --;
   // updateLives();
    if(lives == 0){
        levelNumber = 0;
        lives = 3;
    }

    for(int p = 0; p < particleCount; p++){
        particlePool[p].Spawn(playerPosition);
    }
    stageStartTime = millis();
    stage = "DEAD";
    killTime = millis();
}

// ----------------------------------
// -------- TICKS & RENDERS ---------
// ----------------------------------
void tickEnemies(){
    for(int i = 0; i<enemyCount; i++){
        if(enemyPool[i].Alive()){
            enemyPool[i].Tick();
            // Hit attack?
            if(attacking){
                if(enemyPool[i]._pos > playerPosition-(ATTACK_WIDTH/2) && enemyPool[i]._pos < playerPosition+(ATTACK_WIDTH/2)){
                   enemyPool[i].Kill();
                   SFXkill();
                }
            }
            if(inLava(enemyPool[i]._pos)){
                enemyPool[i].Kill();
                SFXkill();
            }
            // Draw (if still alive)
            if(enemyPool[i].Alive()) {
                leds[getLED(enemyPool[i]._pos)] = CRGB(BRIGHTNESS, 0, 0);
            }
            // Hit player?
            if(
                (enemyPool[i].playerSide == 1 && enemyPool[i]._pos <= playerPosition) ||
                (enemyPool[i].playerSide == -1 && enemyPool[i]._pos >= playerPosition)
            ){
                die();
                return;
            }
        }
    }
}

void tickBoss(){
    // DRAW
    if(boss.Alive()){
        boss._ticks ++;
        for(int i = getLED(boss._pos-BOSS_WIDTH/2); i<=getLED(boss._pos+BOSS_WIDTH/2); i++){
            leds[i] = CRGB(BRIGHTNESS, 0, 0);
            leds[i] %= 100;
        }
        // CHECK COLLISION
        if(getLED(playerPosition) > getLED(boss._pos - BOSS_WIDTH/2) && getLED(playerPosition) < getLED(boss._pos + BOSS_WIDTH)){
            die();
            return; 
        }
        // CHECK FOR ATTACK
        if(attacking){
            if(
              (getLED(playerPosition+(ATTACK_WIDTH/2)) >= getLED(boss._pos - BOSS_WIDTH/2) && getLED(playerPosition+(ATTACK_WIDTH/2)) <= getLED(boss._pos + BOSS_WIDTH/2)) ||
              (getLED(playerPosition-(ATTACK_WIDTH/2)) <= getLED(boss._pos + BOSS_WIDTH/2) && getLED(playerPosition-(ATTACK_WIDTH/2)) >= getLED(boss._pos - BOSS_WIDTH/2))
            ){
               boss.Hit();
               if(boss.Alive()){
                   moveBoss();
               }else{
                   spawnPool[0].Kill();
                   spawnPool[1].Kill();
               }
            }
        }
    }
}

void drawPlayer(){
    leds[getLED(playerPosition)] = CRGB(0, BRIGHTNESS, 0);
}

void drawExit(){
    if(!boss.Alive()){
        leds[NUM_LEDS-1] = CRGB(0, 0, BRIGHTNESS);
    }
}

void tickSpawners(){
    long mm = millis();
    for(int s = 0; s<spawnCount; s++){
        if(spawnPool[s].Alive() && spawnPool[s]._activate < mm){
            if(spawnPool[s]._lastSpawned + spawnPool[s]._rate < mm || spawnPool[s]._lastSpawned == 0){
                spawnEnemy(spawnPool[s]._pos, spawnPool[s]._dir, spawnPool[s]._sp, 0);
                spawnPool[s]._lastSpawned = mm;
            }
        }
    }
}

void tickLava(){
    int A, B, p, i, brightness, flicker;
    long mm = millis();
    Lava LP;
    for(i = 0; i<lavaCount; i++){
        flicker = random8(5);
        LP = lavaPool[i];
        if(LP.Alive()){
            A = getLED(LP._left);
            B = getLED(LP._right);
            if(LP._state == "OFF"){
                if(LP._lastOn + LP._offtime < mm){
                    LP._state = "ON";
                    LP._lastOn = mm;
                }
                for(p = A; p<= B; p++){
                    leds[p] = CRGB(BRIGHTNESS/10+flicker+5,BRIGHTNESS/10+(3+flicker)/1.5, 0);
                }
            }else if(LP._state == "ON"){
                if(LP._lastOn + LP._ontime < mm){
                    LP._state = "OFF";
                    LP._lastOn = mm;
                }
                for(p = A; p<= B; p++){
                    leds[p] = CRGB(BRIGHTNESS+flicker, BRIGHTNESS*.7+flicker, 0);
                }
            }
        }
        lavaPool[i] = LP;
    }
}

bool tickParticles(){
    bool stillActive = false;
    for(int p = 0; p < particleCount; p++){
        if(particlePool[p].Alive()){
            particlePool[p].Tick(USE_GRAVITY);
            if(particlePool[p]._power+10 <= BRIGHTNESS)
              leds[getLED(particlePool[p]._pos)] += CRGB(particlePool[p]._power+10, 0, 0);
            else
              leds[getLED(particlePool[p]._pos)] += CRGB(BRIGHTNESS, 0, 0);
            stillActive = true;
        }
    }
    return stillActive;
}

void tickConveyors(){
    int b, dir, n, i, ss, ee, led;
    long m = 10000+millis();
    //playerPositionModifier = 0;
    
    for(i = 0; i<conveyorCount; i++){
        if(conveyorPool[i]._alive){
            dir = conveyorPool[i]._dir;
            ss = getLED(conveyorPool[i]._startPoint);
            ee = getLED(conveyorPool[i]._endPoint);
            for(led = ss; led<ee; led++){
                b = 5;
                n = (-led + (m/100)) % 5;
                if(dir == -1) n = (led + (m/100)) % 5;
                b = (5-n)/2.0;
                if(b > 0) leds[led] = CRGB(0, 0, BRIGHTNESS/6);
            }
            
            if(playerPosition > conveyorPool[i]._startPoint && playerPosition < conveyorPool[i]._endPoint){
                if(inWater == 0){
                  inWater = 1;
                  if(conveyorPool[i]._dir == 1)
                    playerPositionModifier = 3;
                  else
                    playerPositionModifier = -3;
                }
                //if(dir == -1){
                  //  playerPositionModifier = playerPositionModifier - 10; //= -(MAX_PLAYER_SPEED-4);
                 // joystickTilt = joystickTilt/2;
                //}else{
                   // playerPositionModifier = (MAX_PLAYER_SPEED-4);
                //}
            }
            else {
              inWater = 0;
              playerPositionModifier = 0;
            //  joystickTilt = joystickTilt*2;
            }
        }
    }
}

void drawAttack(){
    // Fade innner LED's with time
    if(!attacking) return;
    int n = map(millis() - attackMillis, 0, ATTACK_DURATION, BRIGHTNESS, 5);// bn was 100
    for(int i = getLED(playerPosition-(ATTACK_WIDTH/2))+1; i<=getLED(playerPosition+(ATTACK_WIDTH/2))-1; i++){
        leds[i] = CRGB(n*.4, 0, n*.8);
    }

    // Keep the player's pixel green.
    leds[getLED(playerPosition)] = CRGB(0, BRIGHTNESS, 0);

    // Draws the edge LEDs
    leds[getLED(playerPosition-(ATTACK_WIDTH/2))] = CRGB(BRIGHTNESS, n, BRIGHTNESS);// n,n,255
    leds[getLED(playerPosition+(ATTACK_WIDTH/2))] = CRGB(BRIGHTNESS, n, BRIGHTNESS);
}

int getLED(int pos){
    // The world is 1000 pixels wide, this converts world units into an LED number
    return constrain((int)map(pos, 0, 1000, 0, NUM_LEDS-1), 0, NUM_LEDS-1);
}

bool inLava(int pos){
    // Returns if the player is in active lava
    int i;
    Lava LP;
    for(i = 0; i<lavaCount; i++){
        LP = lavaPool[i];
        if(LP.Alive() && LP._state == "ON"){
            if(LP._left < pos && LP._right > pos) return true;
        }
    }
    return false;
}


// ---------------------------------
// --------- SCREENSAVER -----------
// ---------------------------------
void screenSaverTick(){
    int n, b, c, i;
    long mm = millis();
    int mode = (mm/20000)%2;
    
    for(i = 0; i<NUM_LEDS; i++){
        leds[i].nscale8(250);
    }/*
    if(mode == 0){
        // Marching green <> orange
        n = (mm/250)%10;
        b = 10+((sin(mm/500.00)+1)*20.00);
        c = 20+((sin(mm/5000.00)+1)*33);
        for(i = 0; i<NUM_LEDS; i++){
            if(i%10 == n){
                leds[i] = CHSV( c, 255, 150);
            }
        }
    }else if(mode == 1){*/
        // Random flashes
        randomSeed(mm);
        for(i = 0; i<NUM_LEDS; i++){
            if(random16(800) == 0){
              if(random8(2) == 0)
                leds[i] = CRGB(BRIGHTNESS, BRIGHTNESS, BRIGHTNESS);
              else
                leds[i] = CRGB(0, 0, BRIGHTNESS);
            }
        }
    //}
}

// ---------------------------------
// -------------- SFX --------------
// ---------------------------------
void SFXtilt(int amount){ 
    int f = map(abs(amount), 0, 90, 80, 900)+random8(100);
    if(playerPositionModifier < 0) f -= 500;
    if(playerPositionModifier > 0) f += 200;
    toneAC(f, min(min(abs(amount)/9, 5), MAX_VOLUME));
    
}
void SFXattacking(){
    int freq = map(sin(millis()/2.0)*1000.0, -1000, 1000, 500, 600);
    if(random8(5)== 0){
      freq *= 3;
    }
    toneAC(freq, MAX_VOLUME);
}
void SFXdead(){
    int freq = max(1000 - (millis()-killTime), 10);
    freq += random8(200);
    int vol = max(10 - (millis()-killTime)/200, 0);
    toneAC(freq, MAX_VOLUME);
}
void SFXkill(){
    toneAC(2000, MAX_VOLUME, 1000, true);
}
void SFXwin(){
    int freq = (millis()-stageStartTime)/3.0;
    freq += map(sin(millis()/20.0)*1000.0, -1000, 1000, 0, 20);
    int vol = 10;//max(10 - (millis()-stageStartTime)/200, 0);
    toneAC(freq, MAX_VOLUME);
}

void SFXcomplete(){
    noToneAC();
}

#pragma GCC diagnostic pop
