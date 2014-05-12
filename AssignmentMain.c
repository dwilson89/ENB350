
#class auto
#use RCM40xx.LIB

/*
*********************************************************************************************************
*                                               CONFIGURATION
*********************************************************************************************************
*/

// Redefine uC/OS-II configuration constants as necessary
#define OS_MAX_EVENTS         	3      // Maximum number of events (semaphores, queues, mailboxes)
#define OS_MAX_TASKS           	4      // Maximum number of tasks system can create (less stat and idle tasks)

//#define OS_TASK_CREATE_EN    		0      // Disable normal task creation
//#define OS_TASK_CREATE_EXT_EN  	1      // Enable extended task creation
#define OS_TASK_STAT_EN      		1      // Enable statistics task creation
#define OS_MBOX_EN         		1      // Enable mailboxes
#define OS_MBOX_POST_EN      		1      // Enable MboxPost
#define OS_TIME_DLY_HMSM_EN  		1      // Enable OSTimeDlyHMSM
#define OS_SEM_EN          		1      // Enable semaphore usage
#define STACK_CNT_512        		8      // number of 512 byte stacks (application tasks + stat task + prog stack)

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
#define Festo_Sense_Riser_Down		3
#define Festo_Sense_Riser_Up			4
#define Festo_Sense_Ejector			5
#define Festo_Sense_Measure_Down		6

// Serial Port B Bit Values Used for Various Festo Outputs
#define Festo_Riser_Down				2
#define Festo_Riser_Up					3
#define Festo_Ejector					6
#define Festo_Measure_Down				7

#define FALSE								0
#define TRUE								1

//constants for multitasking

#define          TASK_STK_SIZE     512                /* Size of each task's stacks (# of bytes)       */

#define          TASK_START_ID       0                /* Application tasks IDs                         */
#define          FESTO_TASK_ID       1
#define          STOP_TASK_ID           2
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

//multitasking prototypes
static void TaskStartCreateTasks(void);
void FestoTask(void *data);
void StopTask(void *data);


/*
************************************************************

************************************************************
*/

int main(){

	int i = 0;
	auto int  key;
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
            	printf("Festo Up\n");
               stopFestoDown();
               moveFestoUp();
           		break;

				case 97:				// a
            	printf("Ejector Activate\n");
					activateEjector();
					break;

            case 122:				// z
            	printf("Ejector Deactivate\n");
					stopEjector();
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

            case 109:
            	printf("Checking Material\n");
               printf("%d\n", checkMaterial());
               break;

            case 110:
            	printf("Checking Colour\n");
               printf("%d\n", checkColour());
               break;

         	case 112:
            	printf("Making Readings\n");
               FestoTask(void);
               printf("Colour Is %d\n", workPieceHistory[0].colour);
               printf("Materal Is %d\n", workPieceHistory[0].material);
               printf("Decision is %d\n", workPieceHistory[0].decision);
               break;


            default:
            	break;
			}
	}
   */
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

	//assume everyting is already in correct positions
   struct workPiece tempPiece;
   int workPieceIndex;

	data = data;

	for(;;){

   	printf(" Init\n");
		//state 2 - correct positions
		initialiseFestoBoard();

		//reset positions

		OSTimeDly(OS_TICKS_PER_SEC);

		//state 3 - block in position
      printf("check for block\n");
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
      printf("Check up %d\n", checkRiserUp());
		//state 6 - rise platform
		while(!checkRiserUp()){

			OSTimeDly(OS_TICKS_PER_SEC);

		}
      printf("Is up \n");
		stopFestoUp();

		//delay after state 6
		OSTimeDly(OS_TICKS_PER_SEC);

		//state 8 - check height

      printf("Measure \n");
		moveMeasureDown();

		while(!checkMeasureDown()){

			OSTimeDly(OS_TICKS_PER_SEC);
		}

      printf("Measure down \n");

		tempPiece.height = checkHeight();

		OSTimeDly(OS_TICKS_PER_SEC);

		stopMeasureDown();

		//put it back into position
		while(checkMeasureDown()){

			OSTimeDly(OS_TICKS_PER_SEC);

		}

		OSTimeDly(OS_TICKS_PER_SEC);
      printf("Measure up \n");
		//state 9

     // Add the Work Piece Number and Timestamp
		tempPiece.number = WPN;
		tempPiece.timeStamp = getTimeStamp();

		// Add the Decision to the WorkPiece
		tempPiece.decision = FALSE;
		//tempPiece.decision = makeDecision(tempPiece);

		// Enforce the decision
		enforceDecision(tempPiece.decision);

		// Add the new workPiece to the history, replacing the oldest current workPiece
		workPieceIndex = WPN%16;
		workPieceHistory[workPieceIndex] = tempPiece;

		// Increment the WPN Counter
		WPN++;
	}
}

//stop task
void StopTask (void *data){

   data = data;
	for (;;) {

        //see if stop has been hit if not
        //if it has stop task1 and do nothing until released
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
	if(givenWorkPiece.height <= heightUpper && givenWorkPiece.height >= heightLower){
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

// Checks the Height of the current workPiece
// Returns the current height as a float
float checkHeight(){
	// TODO: Add ADL Conversion Code Here

   return 0;
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
   printf("Measure down %d \n",checkMeasureDown());
	if(checkMeasureDown()){
		stopMeasureDown(); // Just in case this is still set
		while(checkMeasureDown()){ // Until the measure is no longer down, keep moving it up
			OSTimeDly(OS_TICKS_PER_SEC);
		}
	}
   printf("Riser Up %d \n",checkRiserUp());
   printf("Riser Down %d \n",checkRiserDown());
	// If the Riser is currently up, set it to down
	if(checkRiserUp()){
		stopFestoUp(); // Just in case this is still set
      moveFestoDown();
      printf("%d \n",checkRiserDown());
		while(!checkRiserDown()){  // Until the festo riser is down, keep moving it down
         printf("not down %d \n",checkRiserDown());
         OSTimeDly(OS_TICKS_PER_SEC);
		}
		stopFestoDown(); // Once its down, set the Down bit off
	}

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