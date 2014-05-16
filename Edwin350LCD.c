#class auto


#define MAXPIECES 16

/********** SERIAL PORT SETTINGS **********/
#define COUTBUFSIZE 255
#define CINBUFSIZE 255

#define DOUTBUFSIZE 63
#define DINBUFSIZE 63

#ifndef _232BAUD
#define _232BAUD 19200
#endif


/********** KEYPAD SETTINGS **********/
//0x33 = 19200 
#define BAUDRATESET 0x33

// LCD Interface Keypad
#define UP              0x51 // UP ARROW
#define DOWN            0x56 // DOWN ARROW
#define LEFT            0x57 // LEFT ARROW
#define RIGHT           0x55 // RIGHT ARROW
#define F1              0x52 // F1 ARROW
#define F2              0x4D // F2 ARROW
#define ENTER           0x4C // ENTER ARROW

/********** uC/OS-II SETTINGS **********/
#define  TASK_STK_SIZE    512   // Size of each task's stacks (# of bytes)
// Redefine uC/OS-II configuration constants as necessary
#define OS_MAX_TASKS    10      // Maximum number of tasks system can create (less stat and idle tasks)
#define STACK_CNT_512   OS_MAX_TASKS+1      // number of 512 byte stacks (application tasks + prog stacks)


/********** MACROS & CONSTANTS **********/
#define COLOUR_MASK 0xF0
#define MATERIAL_MASK 0x0C
#define DECISION_MASK 0x03

#define DS2 2
#define DS3 3

//Master Modes
#define STARTUP 1
#define DOWNLOAD 2
#define THRESHOLD 3

//Display modes
#define DISPLAY_TIME 1
#define DISPLAY_THRESH 2
#define DISPLAY_PIECES 3

//Threshold Setting Mode
#define THRESHOLD_LOW 1
#define THRESHOLD_HIGH 2

//Download modes
#define DOWNLOAD_IDLE 1
#define DOWNLOAD_REQUEST 2
#define DOWNLOAD_WAIT 3
#define DOWNLOAD_READING 4

//Threshold modes
#define THRESHOLD_IDLE 1
#define THRESHOLD_REQUEST 2
#define THRESHOLD_WAIT 3
#define THRESHOLD_SENDING 4

//Flag
#define THRESHOLD_READY 0


/********** LIBRARIES & INCLUDES **********/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#use "ucos2.lib"
#use RCM40xx.LIB

//// GLOBAL VARIABLES TO BE TAKEN TO THE START

    //RealTime Mode

    
    #define REALTIMEDISPLAY 1
    #define SETTINGSDISPLAY 2
    #define OFFLINEDISPLAY 3

    #define REALTIME 1
    #define SETTINGS 2
    #define OFFLINE 3
    
    //Offline Mode
   
    #define COLOUR 1
    #define MATERIAL 2
    #define HEIGHT 3
    #define TIME 4
    #define DECISION 5 
    #define LOWERTHRESH 6
    #define UPPERTHRESH 7

    char setThresh, top_level_state, Offline_menu_state;

    //Threshold Setting Mode
    #define LOW_THRESH 1
    #define HIGH_THRESH 2

    typedef struct {
    uint16_t upper;
    uint16_t lower;
    } Threshold;

    Threshold currentThresh;






/********** DATA STRUCTURES **********/
typedef struct {
    uint16_t id;
    uint8_t analysis;
    uint16_t height;
    uint16_t size;
    uint32_t time;
    Threshold thr;
} Workpiece;



/********** FUNCTION PROTOTYPES **********/
char* getType(Workpiece);
char* getDecision(Workpiece);
void LCDputs(char col, char row, char *str);
void LCDputc(char col, char row, char chr);
void initSerial(void);
void initLCD(void);


/********** OS TASKS **********/
void TaskLCD(void *data);
void TaskKeypad(void *data);


/********** GLOBAL VARIABLES **********/
char sync, currWorkpiece, totalWorkpieces;
char masterMode, displayMode, downloadMode, thresholdMode, thresSetMode;
Workpiece wp[MAXPIECES];


/*** SETUP DUMMY WORKPIECES ***/
void populate_workpieces(void)
{
    struct tm t;
    tm_rd(&t);
    wp[0].id = 1;
    wp[0].size = 3;
    wp[0].time = mktime(&t);
    wp[0].thr.upper = 44;
    wp[0].thr.lower = 9;
    wp[0].id = 1;
    wp[0].analysis = 0x11;
    wp[0].height = 20;
    wp[1].id = 2;
    wp[1].analysis = 0x15;
    wp[1].height = 32;
    wp[2].id = 3;
    wp[2].analysis = 0x04;
    wp[2].height = 56;

    totalWorkpieces = 1;
}


int main(void)
{
    sync = 0;
    currWorkpiece = 0;
    currentThresh.lower = 20;
    currentThresh.upper = 70;
    populate_workpieces();
    masterMode = REALTIMEDISPLAY;
    top_level_state = REALTIME;
    setThresh == LOW_THRESH;
    Offline_menu_state = COLOUR;

    displayMode = DISPLAY_TIME;
    downloadMode = DOWNLOAD_IDLE;
    thresholdMode = THRESHOLD_IDLE;

    // Initialize I/O pins
    brdInit();

    //Initialize serial ports
    initSerial();

    //Reset sequence for LCD/Keypad
    initLCD();

    OSInit();

    OSTaskCreate(TaskKeypad, NULL, TASK_STK_SIZE, 10);
    OSTaskCreate(TaskLCD, NULL, TASK_STK_SIZE, 15);

    OSStart();

    return 0;
}



void TaskKeypad(void *data)
{
    //Retrieves input buffer from LCD keypad and prints to terminal

    auto UBYTE err;
    char cIn;

    

    ///////////////////////////

    for (;;) {
        //only process if data was received on serD
        if (serDrdUsed()) {
            cIn = serDgetc();

            switch (masterMode) {
            case REALTIMEDISPLAY: //Real Time Mode
                /*************** INITIAL SCREEN ON STARTUP/DISPLAY MODE ***************/
                switch (cIn) {
               
                    case F1: // Switch between top level states

                    switch (top_level_state) {
                        case REALTIME: 
                        /// CHECK IF SYSTEM IS RUNNING OR NOT

                        // IF IT IS THEN CANT GO THROUGH MENU

                        // IF IT ISNT ON THEN GO TO SETTINGS AND OFFLINE MODE
                        top_level_state = SETTINGS; //Switch from Real Time Mode to Settings Mode 
                        break;
                    case SETTINGS: 
                        top_level_state = OFFLINE; //Switch from Settings Mode to Offline Mode
                        break;
                    case OFFLINE:
                        top_level_state = REALTIME; //Switch from Offline Mode to Real Time Mode
                        break;
                    }
            }
            break;

    case OFFLINEDISPLAY:
            
        switch (cIn) 
        {

            case F2:
                switch (Offline_menu_state)
                {
                    case COLOUR:
                        Offline_menu_state = MATERIAL; 
                    break;
                    case MATERIAL:
                        Offline_menu_state = HEIGHT;
                    break;
                    case HEIGHT:
                        Offline_menu_state = TIME;   
                    break;
                    case TIME:
                        Offline_menu_state = DECISION;
                    break;
                    case DECISION:
                        Offline_menu_state = LOWERTHRESH;
                    break;
                    case LOWERTHRESH:
                        Offline_menu_state = UPPERTHRESH;
                    break;
                    case UPPERTHRESH:
                        Offline_menu_state = COLOUR;
                    break;
                }


                case RIGHT:
                    if (currWorkpiece = totalWorkpieces-1)
                    {
                      currWorkpiece = 0;
                    }
                    else currWorkpiece++;
                    break;

                case LEFT:
                    if (currWorkpiece == 0)
                    {
                      currWorkpiece = totalWorkpieces-1;
                    }
                    else currWorkpiece--;
                    break;
        
            case SETTINGSDISPLAY:         //Settings Mode

                switch (cIn)
                {
                
                    case ENTER:
                    if(setThresh == LOW_THRESH){
                        setThresh = HIGH_THRESH;
                    }
                    else if(setThresh == HIGH_THRESH){
                        masterMode = REALTIMEDISPLAY;
                        displayMode = REALTIME;
                    }
                    break;

                    case UP:
                    if(setThresh == LOW_THRESH){
                        if(currentThresh.lower != currentThresh.upper - 1){
                           currentThresh.lower++; 
                        }
                        else if(setThresh == HIGH_THRESH){
                            if(currentThresh.upper < 99){
                                currentThresh.upper++;    
                            }
                        }
                    }    
                    break;

                case DOWN:
                if(setThresh == LOW_THRESH)
                {
                        if(currentThresh.lower > 1){
                           currentThresh.lower--;
                    }
                    else if(setThresh == HIGH_THRESH)
                    {
                        if(currentThresh.upper != currentThresh.lower + 1)
                        {
                            currentThresh.upper--;    
                        }
                    }
                }    
                break;
            } //   switch(cIn)
            break;
        }
        break;
    }

} //switch (masterMode)
       
                    
OSTimeDly(OS_TICKS_PER_SEC/20);
    } //for (;;)
}


void TaskLCD(void *data)
{
    //Refresh LCD display

    auto UBYTE err;
    char LCDbuf[40], timebuf[20]; //holds the next LCD output
    struct tm t;

    for (;;) {
        //Update LED
        if (sync) {
            BitWrPortI(PBDR, &PBDRShadow, 0, DS2); //Turn LED on
        }
        else {
            BitWrPortI(PBDR, &PBDRShadow, 1, DS2); //Turn LED off
        }

        switch (masterMode) {
            case REALTIMEDISPLAY: //Real Time Mode
                /*************** INITIAL SCREEN ON STARTUP/DISPLAY MODE ***************/
                sprintf(LCDbuf, "WP:%2u/%2u ", wp[currWorkpiece].id, totalWorkpieces);

                switch (top_level_state) {
                    case REALTIME:
                        mktm(&t, wp[currWorkpiece].time);
                        strftime(timebuf, sizeof timebuf, "%c", &t);
                        sprintf(LCDbuf, "%s TIMESTAMP%s", LCDbuf, timebuf);
                        LCDputs(1, 1, LCDbuf);
                        break; 

                    case SETTINGS: 
                        sprintf(LCDbuf, "%sSettings Mode ", LCDbuf);
                        LCDputs(1,1, LCDbuf);
                        break;
                    case OFFLINE:
                        sprintf(LCDbuf, "%sOffline Mode ", LCDbuf);
                        LCDputs(1,1, LCDbuf);
                        break;
                    }
                    break;


            case OFFLINEDISPLAY:
                //sprintf(LCDbuf, "WP:%2u/%2u ", wp[currWorkpiece].id, totalWorkpieces);

                switch (Offline_menu_state){
                    case COLOUR:
                    sprintf(LCDbuf, "%sColour: ", LCDbuf);
                    LCDputs(1, 1, LCDbuf);
                    break;
                    case MATERIAL:
                        sprintf(LCDbuf, "%sMaterial: ", LCDbuf);
                        LCDputs(1, 1, LCDbuf);
                    break;
                    case HEIGHT:
                        sprintf(LCDbuf, "%sHeight: ", LCDbuf); 
                        LCDputs(1, 1, LCDbuf);  
                    break;
                    case TIME:
                        sprintf(LCDbuf, "%sTime: ", LCDbuf);
                        LCDputs(1, 1, LCDbuf);
                    break;
                    case DECISION:
                        sprintf(LCDbuf, "%sDecision: ", LCDbuf);
                        LCDputs(1, 1, LCDbuf);
                    break;
                    case LOWERTHRESH:
                        sprintf(LCDbuf, "%sLower Thresh: ", LCDbuf);
                        LCDputs(1, 1, LCDbuf);
                    break;
                    case UPPERTHRESH:
                        sprintf(LCDbuf, "%sUpper Thresh: ", LCDbuf);
                        LCDputs(1, 1, LCDbuf);
                    break;
                }


            case SETTINGSDISPLAY:
            /*************** THRESHOLD MODE ***************/
            switch(setThresh){
            case LOW_THRESH:
            //sprintf(LCDbuf, "WP:%2u/%2u ", wp[currWorkpiece].id, totalWorkpieces);
            sprintf(LCDbuf, "%sThreshold Mode      Low: %u            ",LCDbuf, currentThresh.lower);
            break;

            case HIGH_THRESH:
            //sprintf(LCDbuf, "WP:%2u/%2u ", wp[currWorkpiece].id, totalWorkpieces);
            sprintf(LCDbuf, "%sThreshold Mode      High: %u           ",LCDbuf, currentThresh.upper);
            break;

            }
            LCDputs(1, 1, LCDbuf);
            break;

        } //switch (masterMode)

        OSTimeDly(OS_TICKS_PER_SEC/10);
    } //for (;;)

} //TaskLCD

char* getType(Workpiece wp)
{
    if (wp.analysis & 0x04) {
        //NON-METAL
        if (wp.analysis & 0x10) {
            //BLACK
            return "Black ";
        }
        else {
            //ORANGE
            return "Orange";
        }
    }
    else {
        //METAL
        return "Metal ";
    }
}

char* getDecision(Workpiece wp)
{
    if (wp.analysis & 0x01)
        return "PASS";
    else
        return "FAIL";
}

void LCDputs(char col, char row, char *str)
{
    char s[DOUTBUFSIZE];
    sprintf(s, "%c%c%c%c%s", 254, 71, col, row, str); //col, row
    //sprintf(s,"%c%c%s", 0xFE, 0x58, str); //clear first
    serDputs(s);
}

void LCDputc(char col, char row, char chr)
{
    char s[DOUTBUFSIZE];
    sprintf(s, "%c%c%c%c%c", 254, 71, col, row, chr); //col, row
    serDputs(s);
}

void initSerial(void)
{
    //init Serial port C
    serCopen(_232BAUD);
    serCwrFlush();
    serCrdFlush();

    //init Serial port D (LCD Keypad)
    serDopen(_232BAUD);
    serDwrFlush();
    serDrdFlush();
}

void initLCD(void)
{
    char s[DOUTBUFSIZE];

    //DATA LOCK OFF
    sprintf(s,"%c%c%c%c%c", 0xFE, 0xCA, 0xF5, 0xA0, 0);
    serDputs(s);
    while(serDwrUsed());

    //SET REMEMBER (1 = Remember)
    sprintf(s,"%c%c%c", 0xFE, 0x93, 1);
    serDputs(s);
    while(serDwrUsed());

    //SET DEFAULT START SCREEN
    sprintf(s,"%c%cDefault Start ScreenQUT...S1129...Rev 01", 0xFE, 0x40);
    serDputs(s);
    while(serDwrUsed());

#if BAUDRATESET > 0
    //CHANGE BAUD RATE
    sprintf(s,"%c%c%c", 0xFE, 0x39, BAUDRATESET);
    serDputs(s);
    while(serDwrUsed());
#endif

    //SET DEBOUNCE TIME (increments of 6.554ms)*8 = ~52ms
    sprintf(s,"%c%c%c", 0xFE, 0x55, 8);
    serDputs(s);
    while(serDwrUsed());

    //SET AUTO TRANSMIT ON
    sprintf(s,"%c%c", 0xFE, 0x41);
    serDputs(s);
    while(serDwrUsed());

    //SET AUTO REPEAT MODE OFF
    sprintf(s,"%c%c%c", 0xFE, 0x7E, 0);
    serDputs(s);
    while(serDwrUsed());

    //ASSIGN DEFAULT VALUES TO KEYPAD
    sprintf(s,"%c%cABCDEFGHIJKLMNOPQRSTUVWXYabcdefghijklmnopqrstuvwxy", 0xFE, 0xD5);
    serDputs(s);
    while(serDwrUsed());

    //SET DATA LOCK ON (Setting Lock & Communication Lock)
    sprintf(s,"%c%c%c%c%c", 0xFE, 0xCA, 0xF5, 0xA0, 0x18);
    serDputs(s);
    while(serDwrUsed());

    //SWITCHING DISPLAY ON
    sprintf(s,"%c%c%c", 0xFE, 0x42, 0);
    serDputs(s);
    while(serDwrUsed());

    //SET & SAVE BRIGHTNESS
    sprintf(s,"%c%c%c", 0xFE, 0x98, 255);
    serDputs(s);
    while(serDwrUsed());

    //SET & SAVE CONTRAST
    sprintf(s,"%c%c%c", 0xFE, 0x91, 128);
    serDputs(s);
    while(serDwrUsed());

    //AUTO SCROLL OFF
    sprintf(s,"%c%c", 0xFE, 0x52);
    serDputs(s);
    while(serDwrUsed());

    //UNDERLINE CURSOR OFF
    sprintf(s,"%c%c", 0xFE, 0x4B);
    serDputs(s);
    while(serDwrUsed());

    //BLOCK CURSOR OFF
    sprintf(s,"%c%c", 0xFE, 0x54);
    serDputs(s);
    while(serDwrUsed());

    //Clear LCD
    sprintf(s,"%c%c", 0xFE, 0x58);
    serDputs(s);
    while(serDwrUsed());
}