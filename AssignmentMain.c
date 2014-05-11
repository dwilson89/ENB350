
//task to work the machine
need to set up the rest of the task stuff
//semphores for 
WPN
workPieceHistory
FestoTask
Console
thresholds semphores


void FestoTask (void *data){

	//assume everyting is already in correct positions

	data = data;

	for(;;){

		//state 2 - correct positions 
		initialiseFestoBoard();
		
		//reset positions

		OSTimeDly(OS_TICKS_PER_SEC);

		//state 3 - block in position

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

		//state 6 - rise platform
		while(!checkRiserUp()){

			OSTimeDly(OS_TICKS_PER_SEC);

		}

		stopFestoUp();		

		//delay after state 6
		OSTimeDly(OS_TICKS_PER_SEC);

		//state 8 - check height

		moveMeasureDown();

		while(!checkMeasureDown()){

			OSTimeDly(OS_TICKS_PER_SEC);
		}

		stopMeasureDown();

		//OSTimeDly(OS_TICKS_PER_SEC);

		tempPiece.height = checkHeight();

		OSTimeDly(OS_TICKS_PER_SEC);

		stopMeasureDown();

		//put it back into position
		while(checkMeasureDown){

			OSTimeDly(OS_TICKS_PER_SEC);

		}	

		OSTimeDly(OS_TICKS_PER_SEC);
		
		//state 9

		//checks theshold
		if(makeDecision() == 1){

			//true
			OSTimeDly(OS_TICKS_PER_SEC);

		} else {

			//go back down
			moveFestoDown();
			while(!checkRiserDown()){
				OSTimeDly(OS_TICKS_PER_SEC);

			}
			stopFestoDown();

		}		

					//eject up top
		activateEjector();
		while(checkEjector()){

			OSTimeDly(OS_TICKS_PER_SEC);

		}

		OSTimeDly(OS_TICKS_PER_SEC);
		stopEjector();

		workPieceHistory[WPN] = tempPiece;
		
		WPN++;
	}

}


//stop task
void Task2 (void *data){


	for (;;) {

        //see if stop has been hit if not

        //if it has stop task1 and do nothing until released
        OSTimeDly(OS_TICKS_PER_SEC);
    }



}
