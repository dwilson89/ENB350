
#class auto
#use RCM40xx.LIB

/*
*********************************************************************************************************
*                                               CONFIGURATION
*********************************************************************************************************
*/

// Redefine uC/OS-II configuration constants as necessary
#define OS_MAX_EVENTS         	4      // Maximum number of events (semaphores, queues, mailboxes)
#define OS_MAX_TASKS           	4      // Maximum number of tasks system can create (less stat and idle tasks)

//#define OS_TASK_CREATE_EN    		0      // Disable normal task creation
//#define OS_TASK_CREATE_EXT_EN  	1      // Enable extended task creation
#define OS_TASK_STAT_EN      		1      // Enable statistics task creation
#define OS_TASK_CREATE_EN      1       // enable creation of task
#define OS_TASK_DEL_EN         1       // enable deletion of task
#define OS_TIME_DLY_HMSM_EN  		1      // Enable OSTimeDlyHMSM
#define OS_SEM_EN          		1      // Enable semaphore usage
#define STACK_CNT_512        		8      // number of 512 byte stacks (application tasks + stat task + prog stack)
#define OS_SCHED_LOCK_EN       1
#define OS_TASK_SUSPEND_EN     1
#define TASK_DELAY 75

#use "ucos2.lib"

///////
// change serial buffer name and size here
///////
#define CINBUFSIZE  255
#define COUTBUFSIZE 255

#define DINBUFSIZE  255
#define DOUTBUFSIZE 255

///////
// change serial baud rate here
///////
#ifndef _232BAUD
#define _232BAUD 19200
#endif

// Analogue-to-digital Stuff
#define ADC_SCLKBRATE          115200ul
#define GAIN_ADC               1 // 11.11volts festo height
#define CHANNEL_ADC            0 // channel 0

#define S2_BIT  4

// RCM40xx boards have no pull-up on serial Rx lines, and we assume in this
// sample the possibility of disconnected or non-driven Rx line.  This sample
// has no need of asynchronous line break recognition.  By defining the
// following macro we choose the default of disabled character assembly during
// line break condition.  This prevents possible spurious line break interrupts.
#define RS232_NOCHARASSYINBRK

/*
**********************************************************
						Definitions Constants
**********************************************************
*/
// Serial Port A Bit Values Used for Various Festo Inputs
#define Festo_Sense_Metallic			0
#define Festo_Sense_In_Place			1
#define Festo_Sense_Colour				2
#define Festo_Sense_Riser_Down			3
#define Festo_Sense_Riser_Up			4
#define Festo_Sense_Ejector				5
#define Festo_Sense_Measure_Down		6

// Serial Port B Bit Values Used for Various Festo Outputs
#define Festo_Riser_Down				2
#define Festo_Riser_Up					3
#define Festo_Ejector					6
#define Festo_Measure_Down				7

#define FALSE							0
#define TRUE							1

//constants for multitasking

#define          TASK_STK_SIZE     512                /* Size of each task's stacks (# of bytes)       */

#define          TASK_START_ID       0                /* Application tasks IDs                         */
#define          FESTO_TASK_ID       1
#define          STOP_TASK_ID        2
#define          TASK_3_ID           3
#define          TASK_4_ID           4
#define          TASK_5_ID           5

#define          TASK_START_PRIO    10                /* Application tasks priorities                  */
#define          FESTO_TASK_PRIO    14
#define          STOP_TASK_PRIO     10
#define          TASK_3_PRIO        13
#define          TASK_4_PRIO        14
#define          TASK_5_PRIO        15

/*
**********************************************************
					Variables Definitions
**********************************************************
*/

OS_EVENT 	* WPNSem; // Semaphore for WorkPiece Access
OS_EVENT 	* WPNCountSem;
OS_EVENT 	* LHeightSem;
OS_EVENT 	* UHeightSem;

// Structure to contain data for an individual Work Piece
struct workPiece {
	int number;
	float height;
	int colour;
	int material;
	int decision;
	unsigned long timeStamp;
};

// Store all the values to check against a work piece
float heightUpper;
float heightLower;
int desiredColour;
int desiredMaterial;

// Stores current work piece number
int WPN;

// Stores the time when the board started
unsigned long startTime;

// Stores the last 16 workpieces
struct workPiece workPieceHistory[16];

// Tracks whether the system is currently on or off
int systemOn;

// Tracks whether the system is calibrated or not
int calibrated;

// For Calibration, the height values for each piece is;
unsigned int firstPiece, secondPiece;

// Tracks which baord it is being tested on
int isPlusBoard = 1;

/*
************************************************************
						Function Prototypes
************************************************************
*/

//helper function prototypes
struct workPiece retrieveWorkPiece(int workPieceAge);
int checkHeightCorrect(struct workPiece givenWorkPiece);
int checkColourCorrect(struct workPiece givenWorkPiece);
int checkMaterialCorrect(struct workPiece givenWorkPiece);
int makeDecision(struct workPiece givenWorkPiece);
int checkInPlace();
int checkMaterial();
int checkColour();
int checkRiserDown();
int checkRiserUp();
int checkEjector();
int checkMeasureDown();
float checkHeight();
void moveFestoDown();
void moveFestoUp();
void stopFestoDown();
void stopFestoUp();
void activateEjector();
void stopEjector();
void moveMeasureDown();
void stopMeasureDown();
void initialiseFestoBoard();
void enforceDecision(int decision);
unsigned long getTimeStamp();
void systemControl(int onOff);
void calibrateHeight();

//multitasking prototypes
static void TaskStartCreateTasks(void);
void FestoTask(void *data);
void StopTask(void *data);



/*
************************************************************
                         Main Function and Tasks
************************************************************
*/

int main(){

	int i = 0;
	auto int  key;

   // For Testing
   heightUpper = 30;
   heightLower = 26;
   desiredColour = 1;
   desiredMaterial = 0;

   calibrated = 1;
	// Initialise Rabbit Ports
	brdInit();

	// Set initial system control to off for safety purposes
	systemOn = FALSE;

	// Set All Port A Values to Input
	WrPortI(SPCR, &SPCRShadow, 0x80);

	// Set Required Port B Values to Output
	BitWrPortI(PBDDR, &PBDDRShadow, 1, Festo_Riser_Down);
	BitWrPortI(PBDDR, &PBDDRShadow, 1, Festo_Riser_Up);
	BitWrPortI(PBDDR, &PBDDRShadow, 1, Festo_Ejector);
	BitWrPortI(PBDDR, &PBDDRShadow, 1, Festo_Measure_Down);

	// Initialize the history of WorkPieces
	for(i = 0; i < 16; i++){
		workPieceHistory[i].number = 0;
		workPieceHistory[i].height = 0;
		workPieceHistory[i].colour = TRUE;
		workPieceHistory[i].material = TRUE;
		workPieceHistory[i].decision = FALSE;
		workPieceHistory[i].timeStamp = 0;
	}

	// Get the time at creation
	startTime = read_rtc();

	// Initialise the Festo Board
	//initialiseFestoBoard();
   systemOn = TRUE;
   printf("System online\n");

   OSInit();
   WPNSem = OSSemCreate(1);
   LHeightSem = OSSemCreate(1);
	UHeightSem = OSSemCreate(1);
   WPNCountSem = OSSemCreate(1);

   TaskStartCreateTasks();
   OSStart();

	// For Testing
   /*
	while(1){

			key = getchar();

			switch(key){
				case 0x71:
					exit(0);

				case 119:				// w
               printf("Making Readings\n");
               printf("Colour Is %d\n", workPieceHistory[0].colour);
               printf("Materal Is %d\n", workPieceHistory[0].material);
               printf("Decision is %d\n", workPieceHistory[0].decision);
           		break;

				case 97:				// a
            	printf("Ejector Activate\n");
					//activateEjector();
					break;

            case 122:				// z
            	printf("Measure Height\n");
					printf("Reading is %d\n", checkHeight());
					break;

				case 115:				// s
            	printf("Festo Down\n");
               stopFestoUp();
					moveFestoDown();
					break;

				case 100:				// d
            	printf("Measure Down\n") ;
					moveMeasureDown();
					break;

				case 101:				// e
            	printf("Measure Up\n");
					stopMeasureDown();
					break;

            default:
            	break;
			}
	}*/

}

static void TaskStartCreateTasks(void){

   OSTaskCreate(FestoTask, (void *)0, TASK_STK_SIZE, FESTO_TASK_PRIO);
   OSTaskCreate(StopTask, (void *)0, TASK_STK_SIZE, STOP_TASK_PRIO);
}


/*
************************************************************

************************************************************
*/
void FestoTask (void *data){

   struct workPiece tempPiece;
   int workPieceIndex;
   float tempHeight = 0;
   float currentHeight;
   int i = 0;
   INT8U err;

	data = data;

	for(;;){
      tempHeight = 0;
		//state 2 - correct position

		initialiseFestoBoard();

		//reset positions

		OSTimeDly(OS_TICKS_PER_SEC);
      printf("about to calibrate\n");
		// If system isn't calibrated, calibrate it now
		if(!calibrated){
			calibrateHeight();
         printf("calibration complete 2\n");
		}
      printf("check for block\n");
		//state 3 - block in position
      //printf("check for block\n");
		while(!checkInPlace()){

			OSTimeDly(OS_TICKS_PER_SEC);
		}

		//delay after state 3
		OSTimeDly(OS_TICKS_PER_SEC);

		//state 4 -check colour

		tempPiece.colour = checkColour();

		//delay after state 4
		OSTimeDly(OS_TICKS_PER_SEC);

		//state 5 - check material
		tempPiece.material = checkMaterial();

		//delay after state 5
		OSTimeDly(OS_TICKS_PER_SEC);

		moveFestoUp();

      //printf("Check up %d\n", checkRiserUp());
		//state 6 - rise platform
		while(!checkRiserUp()){

			OSTimeDly(OS_TICKS_PER_SEC);

		}
      //printf("Is up \n");
		stopFestoUp();

		//delay after state 6
		OSTimeDly(OS_TICKS_PER_SEC);

		//state 8 - check height
      if(!isPlusBoard){
      	//printf("Measure \n");
			moveMeasureDown();

			while(!checkMeasureDown()){

				OSTimeDly(OS_TICKS_PER_SEC);
			}
      }

      //printf("Measure down \n");
      // Sample a number of heights to get the average height
      for(i = 0; i < 10; i++){
      	currentHeight = checkHeight();
         tempHeight = tempHeight + currentHeight;
         OSTimeDly(1);
      }
		tempPiece.height = tempHeight / 10;
      printf("height is %f\n", tempPiece.height);

		OSTimeDly(OS_TICKS_PER_SEC);

      if(!isPlusBoard){
			stopMeasureDown();

			//put it back into position
			while(checkMeasureDown()){

				OSTimeDly(OS_TICKS_PER_SEC);

			}
      }

		OSTimeDly(OS_TICKS_PER_SEC);
      printf("Measure up \n");
		//state 9

     // Add the Work Piece Number and Timestamp
		tempPiece.number = WPN;
		tempPiece.timeStamp = getTimeStamp();

		// Add the Decision to the WorkPiece
		tempPiece.decision = FALSE;
		tempPiece.decision = makeDecision(tempPiece);

		// Enforce the decision
		enforceDecision(tempPiece.decision);

      OSSemPend(WPNCountSem, 0, &err);
		// Add the new workPiece to the history, replacing the oldest current workPiece
		workPieceIndex = WPN%16;
     	// Increment the WPN Counter
      WPN++;
      OSSemPost(WPNCountSem);

      OSSemPend(WPNSem, 0, &err);
      workPieceHistory[workPieceIndex] = tempPiece;
      OSSemPost(WPNSem);

	}

}

//stop task
void StopTask (void *data){
   int end = 0;
   data = data;
	for (;;) {

      if (!BitRdPortI(PBDR, S2_BIT)){
         systemOn = !systemOn;								//set valid switch

      	if (systemOn == FALSE){
				//suspendtask
            OSTaskSuspend(FESTO_TASK_PRIO);
            printf("STOP");

      	} else {

           //turn task back on
           OSTaskResume(FESTO_TASK_PRIO);
           printf("START");
         }
		}
      //while (BitRdPortI(PBDR, S2_BIT)){};

   	OSTimeDly(OS_TICKS_PER_SEC);
   }
}

/*
************************************************************
						Helper Functions
************************************************************
*/
// Retrieves a workPiece from the 16 saved workPieces based on age
// workPieceAge = the age of the desired workpiece, where 0
// is the oldest workPiece and 15 is the most recent workPiece
struct workPiece retrieveWorkPiece(int workPieceAge){

	// Find the corresponding index for the desired workpiece
	// Note we are subtracting 1 as WPN is the next workPiece to be added
	int workPieceIndex = ((WPN - 1)%16)+workPieceAge;

	// Return the corresponding workPiece
	return workPieceHistory[workPieceIndex];

}

// Checks if the given workpiece is within the height limits
// Return 0 if outside of limits, 1 if within limits
int checkHeightCorrect(struct workPiece givenWorkPiece){
   INT8U err;
   OSSemPend(UHeightSem, 0, &err); OSSemPend(LHeightSem, 0, &err);
   if(givenWorkPiece.height <= heightUpper && givenWorkPiece.height >= heightLower){
      OSSemPost(UHeightSem); OSSemPost(LHeightSem);
      return 1;
	}

	return 0;
}

// Checks if the given workpiece is of the correct colour
// Return 0 if a different colour, 1 if of the same colour
int checkColourCorrect(struct workPiece givenWorkPiece){
	if(givenWorkPiece.colour == desiredColour){
		return 1;
	}
	return 0;
}

// Checks if the given workpiece is of the correct material
// Return 0 if a different material, 1 if of the same material
int checkMaterialCorrect(struct workPiece givenWorkPiece){
	if(givenWorkPiece.material == desiredMaterial){
		return 1;
	}
	return 0;
}

// Makes a decision on whether to keep or reject a given workPiece
// Return 0 to reject, 1 to keep
int makeDecision(struct workPiece givenWorkPiece){
	if(checkMaterialCorrect(givenWorkPiece) && checkColourCorrect(givenWorkPiece) && checkHeightCorrect(givenWorkPiece)){
		return 1;
	}
	return 0;
}


// Checks if there is a workPiece currently in place on the Festo machine
// Return 1 if it is in place, 0 if it is not
int checkInPlace(){
	return BitRdPortI(PADR, Festo_Sense_In_Place);
}

// Checks the Material Value from the Festo Board
// Return 1 if it is metallic, 0 if it is not
int checkMaterial(){
	return BitRdPortI(PADR, Festo_Sense_Metallic);
}

// Checks the Colour Value from the Festo Board
int checkColour(){
	return BitRdPortI(PADR, Festo_Sense_Colour);
}

// Checks if the Riser is Down on the Festo Board
// Return 1 if it is down, 0 if it is not
int checkRiserDown(){
	//return BitRdPortI(PADR, Festo_Riser_Down);
   return BitRdPortI(PADR, Festo_Sense_Riser_Down);
}

// Checks if the Riser is Up on the Festo Board
// Return 1 if it is up, 0 if it is not
int checkRiserUp(){
	//return BitRdPortI(PADR, Festo_Riser_Up);
   return BitRdPortI(PADR, Festo_Sense_Riser_Up);
}

// Checks if the Ejector is in place on the Festo Board
// Return 1 if it is, 0 if it is not
int checkEjector(){
	return BitRdPortI(PADR, Festo_Sense_Ejector);
}

// Checks if the Measurement is down? On the Festo Board
// TODO: Check exactly what this refers to
// Return 1 if it is, 0 if it is not
int checkMeasureDown(){
	return BitRdPortI(PADR, Festo_Sense_Measure_Down);
}

// Checks the Height of the current workPiece, sampled 10 times
// Returns the current height as a float
float checkHeight(){
   float voltageRead;
	voltageRead = anaInVolts(CHANNEL_ADC, GAIN_ADC);
	return voltageRead;
}

// Moves the Riser on the Festo Down
void moveFestoDown(){
	BitWrPortI(PBDR, &PBDRShadow, 1, Festo_Riser_Down);
}

// Moves the Riser on the Festo Up
void moveFestoUp(){
	BitWrPortI(PBDR, &PBDRShadow, 1, Festo_Riser_Up);
}

// Stops the Riser on the Festo Going Down
void stopFestoDown(){
	BitWrPortI(PBDR, &PBDRShadow, 0, Festo_Riser_Down);
}

// Stops the Riser on the Festo Going Up
void stopFestoUp(){
	BitWrPortI(PBDR, &PBDRShadow, 0, Festo_Riser_Up);
}

// Activates the Ejector on the Festo Board
void activateEjector(){
	BitWrPortI(PBDR, &PBDRShadow, 1, Festo_Ejector);
}

// Stops the Ejector on the Festo Board
void stopEjector(){
	BitWrPortI(PBDR, &PBDRShadow, 0, Festo_Ejector);
}

// Moves the Measure Down on the Festo Board
void moveMeasureDown(){
	BitWrPortI(PBDR, &PBDRShadow, 1, Festo_Measure_Down);
}

// Stops Moving the Measure Down on the Festo Board
void stopMeasureDown(){
	BitWrPortI(PBDR, &PBDRShadow, 0, Festo_Measure_Down);
}

// Initialises the Festo Board for First Use, ensuring Ejector is off and
// the measure device is up and the riser is in the low position

void initialiseFestoBoard(){

	// Stop Everything Just in Case of Rogue Values
	stopMeasureDown();
	stopEjector();
	stopFestoUp();
	stopFestoDown();

	// If the Measure is currently down, move it up
	if(checkMeasureDown()){
		stopMeasureDown(); // Just in case this is still set
		while(checkMeasureDown()){ // Until the measure is no longer down, keep moving it up
			OSTimeDly(OS_TICKS_PER_SEC);
		}
	}

	// If the Riser is currently up, set it to down
	if(checkRiserUp()){
		stopFestoUp(); // Just in case this is still set
      moveFestoDown();
		while(!checkRiserDown()){  // Until the festo riser is down, keep moving it down
         OSTimeDly(OS_TICKS_PER_SEC);
		}
		stopFestoDown(); // Once its down, set the Down bit off
	}
}

// Calibrate the Height Readings
void calibrateHeight(){

	printf("Beginning Calibration\n");
	printf("Please insert the 25mm Piece\n");

	// Wait until the piece is in place, then give enough time to move hand back away
  	while(!checkInPlace()){
		OSTimeDly(OS_TICKS_PER_SEC);
	}

		//delay after state 3
		OSTimeDly(OS_TICKS_PER_SEC);
    	moveFestoUp();
	// Move the Festo to the upper position

   while(!checkRiserUp()){

		OSTimeDly(OS_TICKS_PER_SEC);
	}
	stopFestoUp();

	// Delay Briefly for safety and to make sure it is set in the right place
	OSTimeDly(OS_TICKS_PER_SEC);

    // Move the height reading down so the height can be read and wait until
    // it is confirmed that the height measure is down
   if(!isPlusBoard){
		moveMeasureDown();

		while(!checkMeasureDown()){

			OSTimeDly(OS_TICKS_PER_SEC);

		}
   }
	// Read the first value

	firstPiece = anaIn(0, SINGLE, GAIN_ADC);
	// Reset back to the base position by moving the measure back up and the festo
	// back to the lower position
   if(!isPlusBoard){
		stopMeasureDown();
   }

	OSTimeDly(OS_TICKS_PER_SEC);

	moveFestoDown();
	while(checkRiserUp()){
		OSTimeDly(OS_TICKS_PER_SEC);
	}
	stopFestoDown();

	printf("Please remove the 25mm Piece\n");
   while(checkInPlace()){OSTimeDly(OS_TICKS_PER_SEC);};
	printf("Please insert the 28mm Piece\n");
   while(!checkInPlace()){OSTimeDly(OS_TICKS_PER_SEC);};
	OSTimeDly(OS_TICKS_PER_SEC);

	// Move the Festo to the upper position
	moveFestoUp();
	while(!checkRiserUp()){

		OSTimeDly(OS_TICKS_PER_SEC);

	}

	stopFestoUp();

	// Delay Briefly for safety and to make sure it is set in the right place
	OSTimeDly(OS_TICKS_PER_SEC);

    // Move the height reading down so the height can be read and wait until
    // it is confirmed that the height measure is down

   if(!isPlusBoard){
		moveMeasureDown();

		while(!checkMeasureDown()){

			OSTimeDly(OS_TICKS_PER_SEC);

		}
   }

	// Read the first value

	secondPiece = anaIn(0, SINGLE, GAIN_ADC);

	// Reset back to the base position by moving the measure back up and the festo
	// back to the lower position

	if(!isPlusBoard){
		stopMeasureDown();
	}
	OSTimeDly(OS_TICKS_PER_SEC);

	moveFestoDown();
	while(checkRiserUp()){
		OSTimeDly(OS_TICKS_PER_SEC);
	}
	stopFestoDown();

	printf("Calibrating, you can remove the 28mm Piece Now\n");
	anaInCalib(0, SINGLE, GAIN_ADC, firstPiece, 25, secondPiece, 28);
	anaInEEWr(0, SINGLE, GAIN_ADC);
	printf("Calibration Complete\n");

	calibrated = 1;

}

// Enforces the Decision Requirements
// If the Decision is a 0 (reject) then it moves to the lower riser position and ejects
// If the Decision is a 1 (accept) then it moves to the higher riser position and ejects

void enforceDecision(int decision){

	// If the Measure is currently down, move it up
	if(checkMeasureDown()){
		stopMeasureDown(); // Just in case this is still set
		while(checkMeasureDown()){ // Until the measure is no longer down, keep moving it up
			OSTimeDly(OS_TICKS_PER_SEC);
		}
	}

	// If it is to be rejected
	if(decision == 0){
		// Perform these just in case they arent set correctly
		stopEjector();
		stopFestoUp();
		stopFestoDown();

      moveFestoDown();
		// Make the Riser move the lower position and eject
		while(!checkRiserDown()){ // While its not down, move the festo down
			OSTimeDly(OS_TICKS_PER_SEC);
		}
		stopFestoDown();

	}

	// Else it is to be accepted
	else{
		// Perform these just in case they arent set correctly
		stopEjector();
		stopFestoDown();
		stopFestoUp();

      moveFestoUp();
		// Make the riser move the higher position and eject
		while(!checkRiserUp()){ // While its not up, move the festo up
			OSTimeDly(OS_TICKS_PER_SEC);
		}
		stopFestoUp();
	}

   activateEjector();
	while(checkEjector()){

		OSTimeDly(OS_TICKS_PER_SEC);

	}

	OSTimeDly(OS_TICKS_PER_SEC);
	stopEjector();
}

// Retrieves the time (in seconds) since the system started
unsigned long getTimeStamp(){

	// get the current time and compare to the start time, the difference is the timestamp of this
	unsigned long currentTime;

	currentTime = read_rtc();
	return currentTime - startTime;
}

// Turns the System On or Off
// If given 0, turns system off. If given 1, turns system on
void systemControl(int onOff){
	systemOn = onOff;
}


