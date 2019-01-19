#include "gpiolib_addr.h"
#include "gpiolib_reg.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

/*
Group 44
Group Members: Tianyi Zhang, Ted Liu, Jacky Ho
Date: November 13, 2018
Description: This code will detect the inputs of 2 lasers to check movement of objects in
and out of a room.

*/



//HARDWARE DEPENDENT CODE BELOW
#ifndef MARMOSET_TESTING

//This function should initialize the GPIO pins
GPIO_Handle initializeGPIO()
{
	GPIO_Handle gpio;
	gpio = gpiolib_init_gpio();
	if(gpio == NULL){
		perror("Could not initialize GPIO");
	}
	return gpio;
}


//This function should accept the diode number (1 or 2) and output
//a 0 if the laser beam is not reaching the diode, a 1 if the laser
//beam is reaching the diode or -1 if an error occurs.
#define LASER1_PIN_NUM 4
#define LASER2_PIN_NUM 17
int laserDiodeStatus(GPIO_Handle gpio, int diodeNumber)
{
	if(gpio == NULL){
		return -1;
	}
	//if diodeNumber is 1, check the status of the pin 4
	if(diodeNumber == 1){
		uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));
		//returns true if the diode detects the laser
		if(level_reg & (1 << LASER1_PIN_NUM)){
			return 1;
		}
		//false otherwise
		else{
			return 0;
		}
	}
	//if diodeNumber is 2, check the status of the pin 17
	else if(diodeNumber == 2){
		uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));
		//returns true if the diode detects the laser
		if(level_reg & (1 << LASER2_PIN_NUM)){
			return 1;
		}
		//returns false otherwise
		else{
			return 0;
		}
	}
	//if not diode 1 or 2, then error
	else{
		return -1;
	}
}

#endif
//END OF HARDWARE DEPENDENT CODE

//This function will output the number of times each laser was broken
//and it will output how many objects have moved into and out of the room.

//laser1Count will be how many times laser 1 is broken (the left laser).
//laser2Count will be how many times laser 2 is broken (the right laser).
//numberIn will be the number  of objects that moved into the room.
//numberOut will be the number of objects that moved out of the room.
void outputMessage(int laser1Count, int laser2Count, int numberIn, int numberOut)
{
	printf("Laser 1 was broken %d times \n", laser1Count);
	printf("Laser 2 was broken %d times \n", laser2Count);
	printf("%d objects entered the room \n", numberIn);
	printf("%d objects exitted the room \n", numberOut);
}

//This function accepts an errorCode. You can define what the corresponding error code
//will be for each type of error that may occur.
void errorMessage(int errorCode)
{
	fprintf(stderr, "An error occured; the error code was %d \n", errorCode);
}


#ifndef MARMOSET_TESTING
//This defines the mainline function for our program
int main(const int argc, const char* const argv[]){
	//error if no time is given
	if(argc < 2){
		printf("Error, no time given: exitting\n");
		return -1;
	}
	//Initialize the GPIO pins
	GPIO_Handle gpio = initializeGPIO();
	//variables that we're keeping track of
	int laserLCount = 0;
	int laserRCount = 0;
	int numberIn = 0;
	int numberOut = 0;
	//timeLimit from input
	int timeLimit = atoi(argv[1]);
	//checks to make sure time is positive
	if (timeLimit < 0 ){
		return -1;
	}
	//start time from the beginning of the program
	time_t startTime = time(NULL); 
	
	//in is left then right
	//out is right then left
	//States:
	//NONE_BROKEN = both diodes detects lasers, will be the resting state
	//IN1 = left laser broken, right laser on
	//IN2 = both lasers broken
	//IN3 = left laser restored, right laser still broken
	//OUT1 = right laser broken, left laser on
	//OUT2 = both lasers broken
	//OUT3 = right laser restored, left laser still broken
	enum State{START, NONE_BROKEN, IN1, IN2, IN3, OUT1, OUT2, OUT3, DONE};
	enum State s = START;
	//This will loop until the state is Done
	while (s != DONE){
		//gets the status of the two diodes at the beginning of the loop
		int laser1state = laserDiodeStatus(gpio, 1);
		int laser2state = laserDiodeStatus(gpio, 2);
		//this state machine will check through all the permutations of lasers broken and restored
		//based on the ordering of these, the state machine will track the number of objects entering and exiting
		//The state machine will also track the number of times each laser is broken
		switch(s){
			//Start case, will go straight to the resting state NONE_BROKEN
			case START:
				if ((time(NULL) - startTime) < timeLimit){
					s = NONE_BROKEN;	
				}
				//out of time
				else{
					s = DONE;
				}
				break;
				
			case NONE_BROKEN:
				//Resting state where no lasers are broken
				if ((time(NULL) - startTime) < timeLimit){
					//checks if left laser was broken
					if (!laser1state){
						s = IN1;
						laserLCount++;
					}
					//checks if right laser was broken
					else if (!laser2state){
						s = OUT1;
						laserRCount++;
					}
				}
				//out of time
				else{
					s = DONE;
				}
				break;
				
			case IN1:
				//first state when the object is potentially entering
				//occurs when the left laser gets broken first
				if ((time(NULL) - startTime) < timeLimit){
					//checks if right laser is broken
					if (!laser2state){
						//if yes, add to laserRCount and go to IN2
						s = IN2;
						laserRCount++;
					}
					//checks if the object is backing out
					else if (laser1state){
						//if yes, return to previous state
						s = NONE_BROKEN;
					}
				}
				//out of time
				else{
					s = DONE;
				}
				break;
			
			case IN2:
				//second state when the object is potentially entering
				//occurs when the left laser is broken, then right is broken
				if ((time(NULL) - startTime) < timeLimit){
					//checks if the left laser is restored
					if (laser1state){
						//if yes, go to IN3
						s = IN3;
					}
					//checks if object is backing out
					else if (laser2state){
						//if yes, return to previous state
						s = IN1;
					}
				}
				//out of time
				else{
					s = DONE;
				}
				break;
				
			case IN3:
				//third state when the object is potentially entering
				//occurs when the left laser is broken, the right is broken, then left is restored
				if ((time(NULL) - startTime) < timeLimit){
					//checks if right laser is restored
					if (laser2state){
						//if yes, go back to NONE_BROKEN and add 1 to numberIn
						s = NONE_BROKEN;
						numberIn++;
					}
					//checks if object is backing out
					else if (!laser1state){
						//if yes, return to previous state and add 1 to laserLCount
						s = IN2;
						laserLCount++;
					}
				}
				//out of time
				else{
					s = DONE;
				}
				break;
							
			case OUT1:
				//first state when the object is potentially exiting
				//occurs when the right laser if broken first and left one still on
				if ((time(NULL) - startTime) < timeLimit){
					//checks if left laser is broken
					if (!laser1state){
						//if yes, got to OUT2 and add one to laserLCount
						s = OUT2;	
						laserLCount++;
					}
					//checks if the object is backing in
					else if (laser2state){
						//if yes, return to previous state
						s = NONE_BROKEN;
					}
				}
				//out of time
				else{
					s = DONE;
				} 
				break;
				
			case OUT2:
				//second state when the object is potentially exiting
				//occurs when the right laser is broken first then the left one gets broken
				if ((time(NULL) - startTime) < timeLimit){
					//checks if the right laser is restored
					if (laser2state){
						//if yes, go to OUT3
						s = OUT3;
					}
					//checks if object is backing in
					else if (laser1state){
						//if yes, go to previous state
						s = OUT1;
					}
				}
				//out of time
				else{
					s = DONE;
				}
				break;
				
			case OUT3:
				//third state when the object is potentially exiting
				//occurs when the right laser is broken, then the left is broken, then right is restored
				if ((time(NULL) - startTime) < timeLimit){
					//checks if the left laser is restored
					if (laser1state){
						//if yes, go back to NONE_BROKEN and add one to numberOut
						s = NONE_BROKEN;
						numberOut++;
					}
					//checks if object is backing in
					else if(!laser2state){
						//if yes, go to previous state and add one to laserRCount
						s = OUT2;
						laserRCount++;
					}
				}
				else{
					s = DONE;
				}
				break;
				
			case DONE:
				break;
				
			default:
				break;
		}
		
		usleep(10000);	
	}
	
	
	outputMessage(laserLCount, laserRCount, numberIn, numberOut);
	gpiolib_free_gpio(gpio);
	return 0;
}

#endif
