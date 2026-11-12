/*
 * File:   main.c
 * sudent : basm mwassi
 * ID: 323930479
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "System/system.h"
#include "System/delay.h"
#include "oledDriver/oledC.h"
#include "oledDriver/oledC_colors.h"
#include "oledDriver/oledC_shapes.h"

#include "Accel_i2c.h"


#define BALL_RADIUS     1.8
#define DEAD_ZONE       6
#define MAX_SPEED       5
#define SPEED_DIVIDER   16

#define X_CENTER        14
#define Y_CENTER        0




#define S1_IS_PRESSED()   ((PORTA & (1u << 11)) == 0)
#define S2_IS_PRESSED()   ((PORTA & (1u << 12)) == 0)


#define TIMER1_PERIOD_100MS 6249


volatile unsigned long systemTenths = 0;

volatile int gameTimeRemaining = 0;
volatile unsigned char gameTimerRunning = 0;


volatile unsigned char blinkRequest = 0;



void User_Initialize(void)
{
    TRISA |= (1u << 11);
    TRISA |= (1u << 12);

    TRISB |= (1u << 12);
    ANSB  |= (1u << 12);

    AD1CON1bits.ADON = 0;

    AD1CON1 = 0x0000;
    AD1CON2 = 0x0000;
    AD1CON3 = 0x10FF;


    AD1CON1bits.MODE12 = 0;


    AD1CON1bits.FORM = 0;


    AD1CON1bits.SSRC = 0b0111;

 
    AD1CON1bits.ASAM = 0;


    AD1CHSbits.CH0SA = 8;
    AD1CHSbits.CH0NA = 0;


    AD1CON1bits.ADON = 1;

    DELAY_milliseconds(2);
}


unsigned int readPotentiometer(void)
{

    AD1CON1bits.DONE = 0;


    AD1CON1bits.SAMP = 1;


    while (AD1CON1bits.DONE == 0)
    {
        ;
    }

    return ADC1BUF0 & 0x03FF;
}


void waitButtonsReleased(void)
{
    while (S1_IS_PRESSED() || S2_IS_PRESSED())
    {
        DELAY_milliseconds(10);
    }

    DELAY_milliseconds(30);
}



void waitForAnyButton(void)
{
    while (!S1_IS_PRESSED() &&
           !S2_IS_PRESSED())
    {
        ;
    }

    DELAY_milliseconds(30);
    waitButtonsReleased();
}



void Timer1_Initialize(void)
{

    T1CON = 0x0000;

    TMR1 = 0;
    PR1 = TIMER1_PERIOD_100MS;


    T1CONbits.TCS = 0;


    T1CONbits.TGATE = 0;


    T1CONbits.TCKPS = 2;



    IPC0bits.T1IP = 4;

    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;


    T1CONbits.TON = 1;
}



void __attribute__((interrupt, no_auto_psv))
_T1Interrupt(void)
{

    IFS0bits.T1IF = 0;



    systemTenths++;



    if (gameTimerRunning)
    {
        if (gameTimeRemaining > 0)
        {
            gameTimeRemaining--;


            if (gameTimeRemaining <= 50 &&
                gameTimeRemaining > 0)
            {
                if ((gameTimeRemaining % 5) == 0)
                {
                    blinkRequest = 1;
                }
            }
        }
    }
}


typedef struct
{
    int x1;
    int y1;
    int x2;
    int y2;
} Wall;


static const Wall mazeWalls[] =
{
    /* Horizontal walls */
    {5, 7, 84, 7},
    {11, 12, 36, 12},
    {41, 12, 84, 12},
    {41, 17, 54, 17},
    {17, 18, 30, 18},
    {65, 18, 84, 18},
    {35, 22, 42, 22},
    {48, 22, 60, 22},
    {72, 22, 84, 22},
    {29, 27, 35, 27},
    {54, 27, 66, 27},
    {23, 33, 30, 33},
    {35, 33, 54, 33},
    {60, 33, 78, 33},
    {11, 38, 23, 38},
    {41, 38, 84, 38},
    {17, 43, 30, 43},
    {41, 43, 73, 43},
    {35, 48, 66, 48},
    {23, 48, 30, 48},
    {78, 48, 90, 48},
    {18, 54, 42, 54},
    {48, 54, 60, 54},
    {72, 54, 84, 54},
    {18, 58, 30, 58},
    {42, 59, 54, 59},
    {66, 58, 78, 58},
    {23, 64, 36, 64},
    {48, 64, 79, 64},
    {48, 69, 60, 69},
    {23, 69, 42, 69},
    {66, 69, 84, 69},
    {11, 74, 18, 74},
    {36, 74, 48, 74},
    {72, 75, 90, 75},
    {5, 79, 24, 79},
    {36, 79, 48, 79},
    {66, 79, 84, 79},
    {12, 84, 48, 84},
    {60, 84, 84, 84},
    {12, 90, 90, 90},

    /* Vertical walls */
    {5, 7, 5, 89},
    {90, 7, 90, 90},
    {11, 12, 11, 74},
    {35, 12, 35, 22},
    {41, 12, 41, 18},
    {60, 12, 60, 23},
    {84, 12, 84, 18},
    {17, 18, 17, 33},
    {29, 18, 29, 27},
    {66, 18, 66, 27},
    {23, 22, 23, 38},
    {42, 22, 42, 27},
    {48, 22, 48, 28},
    {72, 22, 72, 33},
    {84, 22, 84, 43},
    {35, 27, 35, 48},
    {54, 27, 54, 33},
    {78, 27, 78, 33},
    {29, 33, 29, 48},
    {41, 38, 41, 43},
    {72, 38, 72, 54},
    {17, 43, 17, 54},
    {23, 43, 23, 48},
    {78, 43, 78, 48},
    {66, 48, 66, 58},
    {35, 54, 35, 64},
    {60, 54, 60, 64},
    {84, 54, 84, 70},
    {17, 58, 17, 74},
    {42, 58, 42, 69},
    {78, 58, 78, 64},
    {23, 64, 23, 79},
    {48, 64, 48, 74},
    {60, 68, 60, 85},
    {66, 69, 66, 79},
    {29, 74, 29, 84},
    {36, 74, 36, 79},
    {54, 74, 54, 90},
    {48, 79, 48, 84},
    {84, 79, 84, 85},
    {12, 84, 12, 90}
};
#define WALL_COUNT \
    (sizeof(mazeWalls) / sizeof(mazeWalls[0]))



static const Wall hiddenStartWalls[] =
{
    {80, 0, 80, 5},

    {92, 0, 92, 18}
};

#define HIDDEN_START_WALL_COUNT \
    (sizeof(hiddenStartWalls) / sizeof(hiddenStartWalls[0]))



void errorStop(char *msg)
{
    oledC_DrawString(
        0,
        20,
        1,
        1,
        (uint8_t *)msg,
        OLEDC_COLOR_DARKRED
    );

    for (;;)
        ;
}



void drawMaze(void)
{
    unsigned int i;
    int y;

    for (i = 0; i < WALL_COUNT; i++)
    {
        if (mazeWalls[i].x1 == mazeWalls[i].x2)
        {
            for (y = mazeWalls[i].y1;
                 y <= mazeWalls[i].y2;
                 y++)
            {
                oledC_DrawPoint(
                    mazeWalls[i].x1,
                    y,
                    OLEDC_COLOR_BLACK
                );
            }
        }

        else
        {
            oledC_DrawLine(
                mazeWalls[i].x1,
                mazeWalls[i].y1,
                mazeWalls[i].x2,
                mazeWalls[i].y2,
                1,
                OLEDC_COLOR_BLACK
            );
        }
    }

    oledC_DrawString(
        67,
        0,
        1,
        1,
        (uint8_t *)"START",
        OLEDC_COLOR_DARKGREEN
    );

    oledC_DrawString(
        0,
        90,
        1,
        1,
        (uint8_t *)"FINISH",
        OLEDC_COLOR_DARKGREEN
    );
}


int isWallPixel(int x, int y)
{
    unsigned int i;

    for (i = 0; i < WALL_COUNT; i++)
    {
        Wall w = mazeWalls[i];

        if (w.x1 == w.x2)
        {
            if (x == w.x1)
            {
                if (y >= w.y1 && y <= w.y2)
                    return 1;
            }
        }

        else if (w.y1 == w.y2)
        {
            if (y == w.y1)
            {
                if (x >= w.x1 && x <= w.x2)
                    return 1;
            }
        }
    }
    

    for (i = 0; i < HIDDEN_START_WALL_COUNT; i++)
    {
        Wall w = hiddenStartWalls[i];

        if (w.x1 == w.x2)
        {
            if (x == w.x1)
            {
                if (y >= w.y1 && y <= w.y2)
                    return 1;
            }
        }

        
        else if (w.y1 == w.y2)
        {
            if (y == w.y1)
            {
                if (x >= w.x1 && x <= w.x2)
                    return 1;
            }
        }
    }

    return 0;
}



int ballHitsWall(int cx, int cy)
{
    int dx;
    int dy;


    if (cx - BALL_RADIUS < 0)
        return 1;

    if (cx + BALL_RADIUS > 95)
        return 1;

    if (cy - BALL_RADIUS < 0)
        return 1;

    if (cy + BALL_RADIUS > 95)
        return 1;



    for (dy = -BALL_RADIUS; dy <= BALL_RADIUS; dy++)
    {
        for (dx = -BALL_RADIUS; dx <= BALL_RADIUS; dx++)
        {
            if ((dx * dx + dy * dy) <=
                (BALL_RADIUS * BALL_RADIUS))
            {
                if (isWallPixel(cx + dx, cy + dy))
                    return 1;
            }
        }
    }

    return 0;
}



void drawBall(int cx, int cy, uint16_t color)
{
    int dx;
    int dy;
    int maxDx;

    for (dy = -BALL_RADIUS; dy <= BALL_RADIUS; dy++)
    {
        maxDx = 0;

        for (dx = 0; dx <= BALL_RADIUS; dx++)
        {
            if ((dx * dx + dy * dy) <=
                (BALL_RADIUS * BALL_RADIUS))
            {
                maxDx = dx;
            }
        }

        oledC_DrawLine(
            cx - maxDx,
            cy + dy,
            cx + maxDx,
            cy + dy,
            1,
            color
        );
    }
}





void moveBallSafe(
    int *ballX,
    int *ballY,
    int moveX,
    int moveY)
{
    int steps;
    int i;

    int accX = 0;
    int accY = 0;

    int absX = abs(moveX);
    int absY = abs(moveY);

    int dirX = 0;
    int dirY = 0;

    int stepX;
    int stepY;

    int nextX;
    int nextY;

    if (moveX > 0)
        dirX = 1;
    else if (moveX < 0)
        dirX = -1;

    if (moveY > 0)
        dirY = 1;
    else if (moveY < 0)
        dirY = -1;

    steps = absX;

    if (absY > steps)
        steps = absY;

    if (steps == 0)
        return;


    for (i = 0; i < steps; i++)
    {
        stepX = 0;
        stepY = 0;

        accX += absX;
        accY += absY;

        if (accX >= steps)
        {
            stepX = dirX;
            accX -= steps;
        }

        if (accY >= steps)
        {
            stepY = dirY;
            accY -= steps;
        }

        
        if (stepX != 0 && stepY != 0)
        {
            nextX = *ballX + stepX;
            nextY = *ballY + stepY;


            if (!ballHitsWall(nextX, nextY) &&
                !ballHitsWall(nextX, *ballY) &&
                !ballHitsWall(*ballX, nextY))
            {
                *ballX = nextX;
                *ballY = nextY;
            }
            else
            {

                if (!ballHitsWall(nextX, *ballY))
                {
                    *ballX = nextX;
                }
                else if (!ballHitsWall(*ballX, nextY))
                {
                    *ballY = nextY;
                }
            }
        }

        else if (stepX != 0)
        {
            nextX = *ballX + stepX;

            if (!ballHitsWall(nextX, *ballY))
            {
                *ballX = nextX;
            }
        }


        else if (stepY != 0)
        {
            nextY = *ballY + stepY;

            if (!ballHitsWall(*ballX, nextY))
            {
                *ballY = nextY;
            }
        }
    }
}




typedef struct
{
    char name[13];
    unsigned int scoreTenths;
} HighScore;



HighScore top3[3] =
{
    {"---", 9999},
    {"---", 9999},
    {"---", 9999}
};


unsigned char difficultyLevel = 2;


void drawMainMenu(unsigned char selected)
{
    char difficultyText[20];

    oledC_setBackground(OLEDC_COLOR_WHITE);
    oledC_clearScreen();

    oledC_DrawString(
        22, 5, 1, 1,
        (uint8_t *)"MAZE GAME",
        OLEDC_COLOR_BLACK
    );

    sprintf(
        difficultyText,
        "Difficulty: %d",
        difficultyLevel
    );

    if (selected == 0)
    {
        oledC_DrawString(
            3, 28, 1, 1,
            (uint8_t *)">",
            OLEDC_COLOR_DARKRED
        );
    }

    oledC_DrawString(
        10, 28, 1, 1,
        (uint8_t *)difficultyText,
        OLEDC_COLOR_BLACK
    );


    if (selected == 1)
    {
        oledC_DrawString(
            3, 48, 1, 1,
            (uint8_t *)">",
            OLEDC_COLOR_DARKRED
        );
    }

    oledC_DrawString(
        10, 48, 1, 1,
        (uint8_t *)"START",
        OLEDC_COLOR_BLACK
    );


    if (selected == 2)
    {
        oledC_DrawString(
            3, 68, 1, 1,
            (uint8_t *)">",
            OLEDC_COLOR_DARKRED
        );
    }

    oledC_DrawString(
        10, 68, 1, 1,
        (uint8_t *)"TOP 3",
        OLEDC_COLOR_BLACK
    );
}


void selectDifficulty(void)
{
    unsigned int pot;
    unsigned char level;
    unsigned char oldLevel = 0;

    while (1)
    {
        pot = readPotentiometer();


        if (pot < 341)
        {
            level = 1;
        }
        else if (pot < 682)
        {
            level = 2;
        }
        else
        {
            level = 3;
        }


        if (level != oldLevel)
        {
            oledC_setBackground(OLEDC_COLOR_WHITE);
            oledC_clearScreen();

            oledC_DrawString(
                5, 5, 1, 1,
                (uint8_t *)"DIFFICULTY",
                OLEDC_COLOR_BLACK
            );


            if (level == 1)
            {
                oledC_DrawString(
                    5, 30, 1, 1,
                    (uint8_t *)"> LEVEL 1",
                    OLEDC_COLOR_DARKRED
                );

                oledC_DrawString(
                    5, 45, 1, 1,
                    (uint8_t *)"40 sec  x0.8",
                    OLEDC_COLOR_BLACK
                );
            }
            else if (level == 2)
            {
                oledC_DrawString(
                    5, 30, 1, 1,
                    (uint8_t *)"> LEVEL 2",
                    OLEDC_COLOR_DARKRED
                );

                oledC_DrawString(
                    5, 45, 1, 1,
                    (uint8_t *)"30 sec  x1.0",
                    OLEDC_COLOR_BLACK
                );
            }
            else
            {
                oledC_DrawString(
                    5, 30, 1, 1,
                    (uint8_t *)"> LEVEL 3",
                    OLEDC_COLOR_DARKRED
                );

                oledC_DrawString(
                    5, 45, 1, 1,
                    (uint8_t *)"20 sec  x1.2",
                    OLEDC_COLOR_BLACK
                );
            }

            oledC_DrawString(
                5, 75, 1, 1,
                (uint8_t *)"S1/S2 = OK",
                OLEDC_COLOR_BLACK
            );

            oldLevel = level;
        }


        if (S1_IS_PRESSED() ||
            S2_IS_PRESSED())
        {
            difficultyLevel = level;

            waitButtonsReleased();

            return;
        }


        DELAY_milliseconds(20);
    }
}


void showTop3(void)
{
    char line[25];
    int i;

    oledC_setBackground(OLEDC_COLOR_WHITE);
    oledC_clearScreen();

    oledC_DrawString(
        25, 3, 1, 1,
        (uint8_t *)"TOP 3",
        OLEDC_COLOR_BLACK
    );


    for (i = 0; i < 3; i++)
    {
        if (top3[i].scoreTenths >= 9999)
        {
            sprintf(
                line,
                "%d. ---",
                i + 1
            );
        }
        else
        {
            sprintf(
                line,
                "%d.%s %u.%u",
                i + 1,
                top3[i].name,
                top3[i].scoreTenths / 10,
                top3[i].scoreTenths % 10
            );
        }


        oledC_DrawString(
            3,
            25 + (i * 18),
            1,
            1,
            (uint8_t *)line,
            OLEDC_COLOR_BLACK
        );
    }


    oledC_DrawString(
        5, 82, 1, 1,
        (uint8_t *)"PRESS BUTTON",
        OLEDC_COLOR_BLACK
    );


    waitForAnyButton();
}


unsigned char mainMenu(void)
{
    unsigned int pot;
    unsigned char selected;
    unsigned char oldSelected = 255;

    while (1)
    {
        pot = readPotentiometer();



        if (pot < 341)
        {
            selected = 0;
        }
        else if (pot < 682)
        {
            selected = 1;
        }
        else
        {
            selected = 2;
        }


        if (selected != oldSelected)
        {
            drawMainMenu(selected);
            oldSelected = selected;
        }


        if (S1_IS_PRESSED() ||
            S2_IS_PRESSED())
        {
            waitButtonsReleased();



            if (selected == 0)
            {
                selectDifficulty();

                oldSelected = 255;
            }



            else if (selected == 1)
            {
                return 1;
            }



            else
            {
                showTop3();

                oldSelected = 255;
            }
        }


        DELAY_milliseconds(20);
    }
}







int isFinishReached(int ballX, int ballY)
{

    if (ballX >= 6 &&
        ballX <= 11 &&
        ballY >= 90)
    {
        return 1;
    }

    return 0;
}





int getGameTimeLimitTenths(void)
{
    if (difficultyLevel == 1)
        return 400;   

    if (difficultyLevel == 2)
        return 300;   

    return 200;       
}


void drawGameTimer(int tenths)
{
    char timeText[10];

    sprintf(
        timeText,
        "%2d.%d",
        tenths / 10,
        tenths % 10
    );


    oledC_DrawString(
        0,
        0,
        1,
        1,
        (uint8_t *)timeText,
        OLEDC_COLOR_DARKRED
    );
}









int qualifiesForTop3(unsigned int scoreTenths)
{
    return (scoreTenths < top3[2].scoreTenths);
}



void insertTop3(char *name, unsigned int scoreTenths)
{
    int position;
    int i;

    position = 3;

    for (i = 0; i < 3; i++)
    {
        if (scoreTenths < top3[i].scoreTenths)
        {
            position = i;
            break;
        }
    }

    if (position >= 3)
        return;

 
    for (i = 2; i > position; i--)
    {
        top3[i] = top3[i - 1];
    }

    strncpy(top3[position].name, name, 12);
    top3[position].name[12] = '\0';

    top3[position].scoreTenths = scoreTenths;
}



void enterPlayerName(char *name)
{
    unsigned int pot;

    char selectedLetter;
    char previousLetter = 0;

    int cursor = 0;
    int i;

    char infoText[20];



    for (i = 0; i < 13; i++)
    {
        name[i] = '\0';
    }

    name[0] = 'A';


    oledC_setBackground(OLEDC_COLOR_WHITE);
    oledC_clearScreen();


    oledC_DrawString(
        16,
        5,
        1,
        1,
        (uint8_t *)"ENTER NAME",
        OLEDC_COLOR_BLACK
    );


    oledC_DrawString(
        4,
        78,
        1,
        1,
        (uint8_t *)"S1+S2 = SAVE",
        OLEDC_COLOR_BLACK
    );


    while (1)
    {

        pot = readPotentiometer();

        selectedLetter =
            'A' + ((pot * 26UL) / 1024UL);

        if (selectedLetter > 'Z')
        {
            selectedLetter = 'Z';
        }



        if (selectedLetter != previousLetter)
        {
            name[cursor] = selectedLetter;


            if (cursor < 11 &&
                name[cursor + 1] == '\0')
            {
                name[cursor + 1] = '\0';
            }



            oledC_DrawRectangle(
                3,
                23,
                92,
                39,
                OLEDC_COLOR_WHITE
            );



            oledC_DrawString(
                5,
                27,
                1,
                1,
                (uint8_t *)name,
                OLEDC_COLOR_BLACK
            );



            oledC_DrawRectangle(
                3,
                45,
                92,
                63,
                OLEDC_COLOR_WHITE
            );


            sprintf(
                infoText,
                "POS:%d CHAR:%c",
                cursor + 1,
                selectedLetter
            );


            oledC_DrawString(
                5,
                49,
                1,
                1,
                (uint8_t *)infoText,
                OLEDC_COLOR_DARKRED
            );


            previousLetter = selectedLetter;
        }



        if (S1_IS_PRESSED() &&
            S2_IS_PRESSED())
        {

            DELAY_milliseconds(80);


            if (S1_IS_PRESSED() &&
                S2_IS_PRESSED())
            {

                name[cursor + 1] = '\0';



                while (S1_IS_PRESSED() ||
                       S2_IS_PRESSED())
                {
                    DELAY_milliseconds(10);
                }


                DELAY_milliseconds(100);


                oledC_setBackground(
                    OLEDC_COLOR_WHITE
                );

                oledC_clearScreen();


                oledC_DrawString(
                    25,
                    30,
                    1,
                    2,
                    (uint8_t *)"SAVED!",
                    OLEDC_COLOR_DARKGREEN
                );


                DELAY_milliseconds(500);



                return;
            }
        }



        if (S1_IS_PRESSED() &&
            !S2_IS_PRESSED())
        {
            DELAY_milliseconds(60);


            if (S1_IS_PRESSED() &&
                !S2_IS_PRESSED())
            {
                if (cursor < 11)
                {
                    cursor++;


                    name[cursor] =
                        selectedLetter;

                    name[cursor + 1] =
                        '\0';
                }



                previousLetter = 0;


                while (S1_IS_PRESSED())
                {
                    DELAY_milliseconds(10);
                }


                DELAY_milliseconds(50);
            }
        }



        else if (S2_IS_PRESSED() &&
                 !S1_IS_PRESSED())
        {
            DELAY_milliseconds(60);


            if (S2_IS_PRESSED() &&
                !S1_IS_PRESSED())
            {
                if (cursor > 0)
                {
                    cursor--;
                }


                previousLetter = 0;


                while (S2_IS_PRESSED())
                {
                    DELAY_milliseconds(10);
                }


                DELAY_milliseconds(50);
            }
        }


        DELAY_milliseconds(15);
    }
}


void showFinalScoreAndTop3(unsigned int scoreTenths)
{
    char line[25];
    int i;

    oledC_setBackground(OLEDC_COLOR_WHITE);
    oledC_clearScreen();


    sprintf(
        line,
        "TIME: %u.%u",
        scoreTenths / 10,
        scoreTenths % 10
    );

    oledC_DrawString(
        5,
        2,
        1,
        1,
        (uint8_t *)line,
        OLEDC_COLOR_DARKGREEN
    );


    oledC_DrawString(
        28,
        16,
        1,
        1,
        (uint8_t *)"TOP 3",
        OLEDC_COLOR_BLACK
    );


    for (i = 0; i < 3; i++)
    {
        if (top3[i].scoreTenths >= 9999)
        {
            sprintf(
                line,
                "%d. ---",
                i + 1
            );
        }
        else
        {

            sprintf(
                line,
                "%d.%.7s %u.%u",
                i + 1,
                top3[i].name,
                top3[i].scoreTenths / 10,
                top3[i].scoreTenths % 10
            );
        }

        oledC_DrawString(
            3,
            32 + (i * 15),
            1,
            1,
            (uint8_t *)line,
            OLEDC_COLOR_BLACK
        );
    }


    oledC_DrawString(
        4,
        82,
        1,
        1,
        (uint8_t *)"PRESS BUTTON",
        OLEDC_COLOR_DARKRED
    );

    waitForAnyButton();
}


int applyDifficultySpeed(int value)
{
    int result;

    if (value == 0)
        return 0;


    if (difficultyLevel == 1)
    {
        result = (value * 8) / 10;
    }
    else if (difficultyLevel == 3)
    {
        result = (value * 12) / 10;
    }
    else
    {
        result = value;
    }


    if (result == 0)
    {
        if (value > 0)
            result = 1;
        else
            result = -1;
    }

    return result;
}




int main(void)
{
    int rc;

    unsigned char deviceId;

    unsigned char xLow;
    unsigned char xHigh;

    unsigned char yLow;
    unsigned char yHigh;

    short xValue;
    short yValue;

    int filteredX;
    int filteredY;

    int ballX;
    int ballY;

    int oldBallX;
    int oldBallY;

    int moveX;
    int moveY;

    int gameStartTenths;
    int oldDisplayedTime;

    unsigned int scoreTenths;

    unsigned char inverseMode;

    unsigned char s1Holding;
    unsigned long s1HoldStart;

    char playerName[13];
    char errorText[20];
    char oldTimeText[10];




    SYSTEM_Initialize();

    User_Initialize();

    Timer1_Initialize();


    oledC_setBackground(OLEDC_COLOR_WHITE);
    oledC_clearScreen();



    i2c1_open();


 
    rc = i2cReadSlaveRegister(
        0x3A,
        0x00,
        &deviceId
    );

    if (rc != OK)
    {
        sprintf(
            errorText,
            "ID Error: %d",
            rc
        );

        errorStop(errorText);
    }


    if (deviceId != 0xE5)
    {
        errorStop("Bad Accel ID");
    }


    DELAY_milliseconds(2);



    rc = i2cWriteSlave(
        0x3A,
        0x2D,
        0x08
    );

    if (rc != OK)
    {
        sprintf(
            errorText,
            "Power Err: %d",
            rc
        );

        errorStop(errorText);
    }


    DELAY_milliseconds(20);



    while (1)
    {

        oledC_sendCommand(
            OLEDC_CMD_SET_DISPLAY_MODE_ON,
            NULL,
            0
        );



        gameTimerRunning = 0;

        blinkRequest = 0;



        mainMenu();




        filteredX = 0;
        filteredY = 0;


        ballX = 83;
        ballY = 4;



        gameTimeRemaining =
            getGameTimeLimitTenths();

        gameStartTenths =
            gameTimeRemaining;


        oldDisplayedTime = -1;

        inverseMode = 0;

        blinkRequest = 0;



        s1Holding = 0;
        s1HoldStart = 0;



        oledC_setBackground(OLEDC_COLOR_WHITE);
        oledC_clearScreen();

        drawMaze();

        drawBall(
            ballX,
            ballY,
            OLEDC_COLOR_BLACK
        );

        drawGameTimer(
            gameTimeRemaining
        );


        gameTimerRunning = 1;



        while (1)
        {

            if (S1_IS_PRESSED())
            {
                if (!s1Holding)
                {
                    s1Holding = 1;

                    s1HoldStart =
                        systemTenths;
                }
                else
                {
                    if ((systemTenths -
                         s1HoldStart) >= 20)
                    {
                        gameTimerRunning = 0;

                        oledC_sendCommand(
                            OLEDC_CMD_SET_DISPLAY_MODE_ON,
                            NULL,
                            0
                        );

                        waitButtonsReleased();


                        break;
                    }
                }
            }
            else
            {
                s1Holding = 0;
            }



            if (gameTimeRemaining !=
                oldDisplayedTime)
            {
                if (oldDisplayedTime >= 0)
                {
                    sprintf(
                        oldTimeText,
                        "%2d.%d",
                        oldDisplayedTime / 10,
                        oldDisplayedTime % 10
                    );

                    oledC_DrawString(
                        0,
                        0,
                        1,
                        1,
                        (uint8_t *)oldTimeText,
                        OLEDC_COLOR_WHITE
                    );
                }


                drawGameTimer(
                    gameTimeRemaining
                );


                oldDisplayedTime =
                    gameTimeRemaining;
            }


 
            if (blinkRequest)
            {
                blinkRequest = 0;

                inverseMode =
                    !inverseMode;


                if (inverseMode)
                {
                    oledC_sendCommand(
                        OLEDC_CMD_SET_DISPLAY_MODE_INVERSE,
                        NULL,
                        0
                    );
                }
                else
                {
                    oledC_sendCommand(
                        OLEDC_CMD_SET_DISPLAY_MODE_ON,
                        NULL,
                        0
                    );
                }
            }



            if (gameTimeRemaining <= 0)
            {
                gameTimerRunning = 0;


                oledC_sendCommand(
                    OLEDC_CMD_SET_DISPLAY_MODE_ON,
                    NULL,
                    0
                );


                oledC_setBackground(
                    OLEDC_COLOR_WHITE
                );

                oledC_clearScreen();


                oledC_DrawString(
                    12,
                    25,
                    2,
                    2,
                    (uint8_t *)"TIME!",
                    OLEDC_COLOR_DARKRED
                );


                oledC_DrawString(
                    5,
                    62,
                    1,
                    1,
                    (uint8_t *)"PRESS BUTTON",
                    OLEDC_COLOR_BLACK
                );



                waitForAnyButton();


                break;
            }


            

            rc = i2cReadSlaveRegister(
                0x3A,
                0x32,
                &xLow
            );

            if (rc != OK)
            {
                sprintf(
                    errorText,
                    "XL Error:%d",
                    rc
                );

                errorStop(errorText);
            }


            DELAY_milliseconds(2);


            rc = i2cReadSlaveRegister(
                0x3A,
                0x33,
                &xHigh
            );

            if (rc != OK)
            {
                sprintf(
                    errorText,
                    "XH Error:%d",
                    rc
                );

                errorStop(errorText);
            }


            DELAY_milliseconds(2);




            rc = i2cReadSlaveRegister(
                0x3A,
                0x34,
                &yLow
            );

            if (rc != OK)
            {
                sprintf(
                    errorText,
                    "YL Error:%d",
                    rc
                );

                errorStop(errorText);
            }


            DELAY_milliseconds(2);


            rc = i2cReadSlaveRegister(
                0x3A,
                0x35,
                &yHigh
            );

            if (rc != OK)
            {
                sprintf(
                    errorText,
                    "YH Error:%d",
                    rc
                );

                errorStop(errorText);
            }



            xValue =
                (short)(
                    ((unsigned short)xHigh << 8)
                    | xLow
                );

            yValue =
                (short)(
                    ((unsigned short)yHigh << 8)
                    | yLow
                );


            xValue -= X_CENTER;
            yValue -= Y_CENTER;



            filteredX =
                ((filteredX * 3)
                 + xValue) / 4;

            filteredY =
                ((filteredY * 3)
                 + yValue) / 4;


            moveX = 0;
            moveY = 0;




            if (filteredX > DEAD_ZONE ||
                filteredX < -DEAD_ZONE)
            {
                moveX =
                    -(filteredX /
                      SPEED_DIVIDER);
            }


            if (filteredY > DEAD_ZONE ||
                filteredY < -DEAD_ZONE)
            {
                moveY =
                    filteredY /
                    SPEED_DIVIDER;
            }



            if (moveX == 0 &&
                filteredX > DEAD_ZONE)
            {
                moveX = -1;
            }


            if (moveX == 0 &&
                filteredX < -DEAD_ZONE)
            {
                moveX = 1;
            }


            if (moveY == 0 &&
                filteredY > DEAD_ZONE)
            {
                moveY = 1;
            }


            if (moveY == 0 &&
                filteredY < -DEAD_ZONE)
            {
                moveY = -1;
            }


            moveX =
                applyDifficultySpeed(moveX);

            moveY =
                applyDifficultySpeed(moveY);



            if (moveX > MAX_SPEED)
                moveX = MAX_SPEED;

            if (moveX < -MAX_SPEED)
                moveX = -MAX_SPEED;

            if (moveY > MAX_SPEED)
                moveY = MAX_SPEED;

            if (moveY < -MAX_SPEED)
                moveY = -MAX_SPEED;


            oldBallX = ballX;
            oldBallY = ballY;



            moveBallSafe(
                &ballX,
                &ballY,
                moveX,
                moveY
            );



            if (isFinishReached(
                    ballX,
                    ballY))
            {
                gameTimerRunning = 0;



                scoreTenths =
                    gameStartTenths
                    - gameTimeRemaining;


                oledC_sendCommand(
                    OLEDC_CMD_SET_DISPLAY_MODE_ON,
                    NULL,
                    0
                );



                oledC_setBackground(
                    OLEDC_COLOR_WHITE
                );

                oledC_clearScreen();


                oledC_DrawString(
                    4,
                    22,
                    1,
                    2,
                    (uint8_t *)"WELL DONE!",
                    OLEDC_COLOR_DARKGREEN
                );


                oledC_DrawString(
                    5,
                    65,
                    1,
                    1,
                    (uint8_t *)"PRESS BUTTON",
                    OLEDC_COLOR_BLACK
                );


                waitForAnyButton();


                
                if (qualifiesForTop3(
                        scoreTenths))
                {
                    enterPlayerName(
                        playerName
                    );


                    insertTop3(
                        playerName,
                        scoreTenths
                    );
                }



                showFinalScoreAndTop3(
                    scoreTenths
                );



                break;
            }



            if (ballX != oldBallX ||
                ballY != oldBallY)
            {

                drawBall(
                    oldBallX,
                    oldBallY,
                    OLEDC_COLOR_WHITE
                );


                /*
                 * Draw new ball.
                 */
                drawBall(
                    ballX,
                    ballY,
                    OLEDC_COLOR_BLACK
                );
            }


            DELAY_milliseconds(25);
        }
    }


    return 0;
}
