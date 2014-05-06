#class auto
#use RCM40xx.LIB


// Serial Port A Bit Values Used for Various Festo Inputs
#define	Festo_Sense_Metallic		0
#define	Festo_Sense_In_Place		1
#define Festo_Sense_Colour			2
#define Festo_Sense_Riser_Down		3
#define Festo_Sense_Riser_Up		4
#define Festo_Sense_Ejector			5
#define Festo_Sense_Measure_Down	6	

// Serial Port B Bit Values Used for Various Festo Outputs
#define Festo_Riser_Down			2
#define	Festo_Riser_Up				3
#define Festo_Ejector				6
#define Festo_Measure_Down			7

// Prototype Definitions			

int checkHeight(workPiece);
int checkColour(workPiece);
int checkMaterial(workPiece);
void createWorkPiece(int height, int colour, int material);
int makeDecision(workPiece);
struct workPiece retrieveWorkPiece(int workPieceAge);
void systemControl(int onOff);
int getTimeStamp();
void makeReadings();
void enforceDecision(int decision);
void initialiseFestoBoard();
void stopMeasureDown();
void moveMeasureDown();
void stopEjector();
void activateEjector();
void stopFestoUp();
void stopFestoDown();
void moveFestoUp();
void moveFestoDown();


// Variable Defintions

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

// Structure to contain data for an individual Work Piece
struct workPiece {
	int number;
	float height;
	bool colour
	bool material;
	bool decision;
	int timeStamp;
}

// Tracks whether the system is currently on or off
bool systemOn;

int main(){

	int i = 0;
	auto INT16S  key;
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
	initialiseFestoBoard();

	// For Testing
	while(1){
		if (kbhit()){
			systemOn = TRUE:
			key = getchar();
			switch(key){
				case 0x71:
					exit(0);

				case 119:				// w
					while(checkRiserDown()){
						moveFestoUp();
					} 
					break;

				case 97:				// a
					stopEjector();
					activateEjector();
					break;

				case 115:				// s
					while(checkRiserUp()){
						moveFestoDown();
					}
					break;

				case 100:				// d
					while(!checkMeasureDown()){
						moveMeasureDown();
					}
					break;

				case 101:				// e
					while(checkMeasureDown()){
						moveMeasureUp();
					}
					break;

			}
		}
	}
}





//	Functions Used to make main tasks easier


// TODO: need to check this function works

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
int checkHeight(workPiece givenWorkPiece){
	if(givenWorkPiece.height <= heightUpper && givenWorkPiece.height >= heightLower){
		return 1;
	}
	return 0;
}

// Checks if the given workpiece is of the correct colour
// Return 0 if a different colour, 1 if of the same colour
int checkColour(workPiece givenWorkPiece){
	if(givenWorkPiece.colour == desiredColour){
		return 1;
	}
	return 0;
}

// Checks if the given workpiece is of the correct material
// Return 0 if a different material, 1 if of the same material
int checkMaterial(workPiece givenWorkPiece){
	if(givenWorkPiece.material == desiredMaterial){
		return 1;
	}
	return 0;
}

// Makes a decision on whether to keep or reject a given workPiece
// Return 0 to reject, 1 to keep
int makeDecision(workPiece givenWorkPiece){
	if(checkMaterial(givenWorkPiece) && checkColour(givenWorkPiece) && checkHeight(givenWorkPiece)){
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
	return BitRdPortI(PADR, Festo_Riser_Down);
}

// Checks if the Riser is Up on the Festo Board
// Return 1 if it is up, 0 if it is not
int checkRiserUp(){
	return BitRdPortI(PADR, Festo_Riser_Up);
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
	return BitRdPortI(PADR, Festo_Measure_Down);
}

// Checks the Height of the current workPiece
// Returns the current height as a float
float checkHeight(){
	// TODO: Add ADL Conversion Code Here
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
	stopMeasureUp();

	// If the Measure is currently down, move it up
	if(checkMeasureDown()){
		stopMeasureDown(); // Just in case this is still set
		while(checkMeasureDown()){ // Until the measure is no longer down, keep moving it up
			moveMeasureUp();
		}
		stopMeasureUp(); // Once it's up, stop the up bit
	}

	// If the Riser is currently up, set it to down
	if(checkRiserUp()){
		stopFestoUp(); // Just in case this is still set
		while(!checkRiserDown()){  // Until the festo riser is down, keep moving it down
			moveFestoDown();
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
			moveMeasureUp();
		}
		stopMeasureUp(); // Once it's up, stop the up bit
	}

	// If it is to be rejected
	if(decision == 0){
		// Perform these just in case they arent set correctly
		stopEjector();
		stopFestoUp();
		stopFestoDown();

		// Make the Riser move the lower position and eject
		while(!checkRiserDown()){ // While its not down, move the festo down
			moveFestoDown();
		}
		stopFestoDown();
		activateEjector();
	}

	// Else it is to be accepted
	else{
		// Perform these just in case they arent set correctly
		stopEjector();
		stopFestoDown();
		stopFestoUp();

		// Make the riser move the higher position and eject
		while(!checkRiserUp()){ // While its not up, move the festo up
			moveFestoUp();
		}
		stopFestoUp();
		activateEjector();
	}
}

// Moves the festo riser up and performs all the readings required, creating a new
// WorkPiece and adding to the workpiece history array and enforcing the decision made
void makeReadings(){

	if(systemOn){  // Only perform Readings if system is currently on

		struct workPiece tempPiece;

		// Just in case these haven't been set correctly
		stopEjector();
		stopFestoDown();
		stopFestoUp();

		// Make sure the measure device is up
		// If the Measure is currently down, move it up
		if(checkMeasureDown()){
			stopMeasureDown(); // Just in case this is still set
			while(checkMeasureDown()){ // Until the measure is no longer down, keep moving it up
				moveMeasureUp();
			}
			stopMeasureUp(); // Once it's up, stop the up bit
		}

		// Move the Festo Riser to the up position
		while(!checkRiserUp()){
			moveFestoUp();
		}
		stopFestoUp();

		// Move the Measure Device Down
		while(!checkMeasureDown()){
			moveMeasureDown();
		}
		stopMeasureDown();


		// Perform Readings for Colour, Material, Height
		tempPiece.colour = checkColour();
		tempPiece.height = checkHeight();
		tempPiece.material = checkMaterial();

		// Add the Work Piece Number and Timestamp
		tempPiece.number = WPN;
		tempPiece.timeStamp = getTimeStamp();

		// Add the Decision to the WorkPiece
		tempPiece.decision = FALSE:
		tempPiece.decision = makeDecision(tempPiece);

		// Enforce the decision
		enforceDecision(tempPiece.decision);

		// Add the new workPiece to the history, replacing the oldest current workPiece
		int workPieceIndex = WPN%16;
		workPieceHistory[workPieceIndex] = tempPiece;

		// Increment the WPN Counter
		WPN++;
	}
}

// todo: add code here
int getTimeStamp(){

	// get the current time and compare to the start time, the difference is the timestamp of this
	int currentTime;

	currentTime = read_rtc();
	return currentTime - startTime;
}

// Turns the System On or Off
// If given 0, turns system off. If given 1, turns system on
void systemControl(int onOff){
	systemOn = onOff;
}
