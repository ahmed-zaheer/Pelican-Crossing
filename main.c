/**
  ******************************************************************************
  * @file    Pelican/main.c 
  * @author  William
  * @version V1.0.0
  * @date    February 14th 2013
  * @brief   Pelican system tst program
  ******************************************************************************
  * 
  * This file provides a test program for the Pelican hardware.
  * 
  * The program cycles through the 6 lights, one at a time, in the order 
  *    RED, AMBER, GREEN, DONTWALK, WALK, WAIT 
  * with 1 sec on and 1 sec off. 
  * 
  * The probe points are measured int the RED, AMBER and DONTWALK states
  * If the button is pressed, all lights go off got 2 sec and then the cycle
  * starts again
  *
  ******************************************************************************  
  */ 

/* Includes ------------------------------------------------------------------*/
#include "pelican.h"


/* Private typedef -----------------------------------------------------------*/
#define REDONOFF 0
#define AMBERONOFF 1
#define GREENONOFF 2
#define DONTWALKONOFF 3
#define WALKONOFF 4
#define WAITONOFF 5
#define BUTTONPRESSED 6
// declare the car Running state 
#define Car_running 7
#define Pedestrian_Waiting 8
#define Car_Slowing_Down 9
#define Car_Stop 10
#define Pedestrian_Crossing 11
#define Pedestrian_Stopping 12
#define Car_Ready_To_Move 13

// declare the time T1, T6
/*
#define T1 10;
#define T2 10;
#define T3 25;
#define T4 15;
#define T5 5;
#define T6 30;
#define T7 7;
*/

/* Private variables ---------------------------------------------------------*/
int cycleCounter = 0 ;  // unit of cycles - 10 is 0.5 sec

volatile int redProbe = 0 ;
volatile int amberProbe = 0 ;
volatile int dontwalkProbe = 0 ;

int state = REDONOFF ;
int start ;
int intermediateValue;

int red_sFashed;
int green_sFashed;
int amber_sFashed;
int walkFashed;
int wait_Fashed;
int dontWalk_Fashed;
int amberFaulty =0;
// declare the time T1, T6

int T1 = 10;
int T2 = 10;
int T3 = 25;
int T4 = 15;
int T5 = 5;
int T6 = 30;
int T7 = 7;

//declare the Operational state 
int OperationalState;

// variable to make differences between start mode and operational mode


/* Private function prototypes -----------------------------------------------*/

void stateLogic(int nextState, PelicanSignal ps) ;

void stateLogicProbe(int nextState, PelicanSignal ps, volatile int *probeValue, ProbePoint pp) ;

void startOperationalMode(void);
void ToggleWaitLight(void);
void redDontWalkFailures(void); 

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void) {
	PelicanConfig() ;
  state = REDONOFF ;
  cycleCounter = 0 ;
	// checker if lights have flashed 
	start =0;
	red_sFashed =0;
	green_sFashed = 0;
	amber_sFashed =0;
	walkFashed = 0;
	wait_Fashed = 0;
	dontWalk_Fashed = 0;
	// set the operationalState default to Car_running state 
	OperationalState = Car_running;

	

  while (1) {
		
		ResetSysTickCounter(5) ;  // 50 ms cycle
		if( start == 0) {

    //ResetSysTickCounter(5) ;  // 50 ms cycle

    switch (state) {
    case REDONOFF:
      stateLogicProbe(AMBERONOFF, RED_S, &redProbe, RED_PP) ;			
		  red_sFashed =1;
      break ;
    case AMBERONOFF:
      stateLogicProbe(GREENONOFF, AMBER_S, &amberProbe, AMBER_PP) ;
		  //stateLogic(GREENONOFF, AMBER_S) ;
		amber_sFashed = 1;
      break ;
    case GREENONOFF:
      stateLogic(DONTWALKONOFF, GREEN_S) ;
		  green_sFashed = 1;
      break ;
    case DONTWALKONOFF:
      stateLogicProbe(WALKONOFF, DONTWALK_S, &dontwalkProbe, DONTWALK_PP) ;
		  //stateLogic(WALKONOFF, DONTWALK_S) ;
		  dontWalk_Fashed = 1;
      break ;
    case WALKONOFF:
      stateLogic(WAITONOFF, WALK_S) ;
		  walkFashed = 1;
      break ;
    case WAITONOFF:
      stateLogic(REDONOFF, WAIT_S);
			wait_Fashed = 1;			
      break ;
    case BUTTONPRESSED:
			
			if ( red_sFashed && amber_sFashed && green_sFashed && dontWalk_Fashed &&  wait_Fashed && walkFashed  )
			{
				start =1;
				SignalSet(RED_S);
				SignalSet(DONTWALK_S);
				ResetSysTickCounter(5) ;  
				WaitSysTickCounter() ;
				startOperationalMode();
			} 
			
      // all lights should be off anyway
      // stay in this state for 2 sec
      if (cycleCounter >= 40) {
        state = REDONOFF ;
        cycleCounter = 0 ;
      } else {
        cycleCounter++ ;
      }}}

    // wait for next cycle
    WaitSysTickCounter() ;
  }
}
void startOperationalMode( void){
	// switch operation state 
	while (1) {
		if( start == 1) { 
			ResetSysTickCounter(50) ;  // 50 ms cycle
			switch (OperationalState) { 

				case Car_running :
					dontwalkProbe= 0;
					SignalReset(RED_S);
				  SignalReset(AMBER_S);
					SignalSet(GREEN_S);				    
				  SignalSet(DONTWALK_S);
				 
					intermediateValue = ProbeVoltage(DONTWALK_PP) ;
					if (intermediateValue >= dontwalkProbe ) { dontwalkProbe = intermediateValue; }
				
					if( dontwalkProbe < 150 ) { 
						redDontWalkFailures();	/// there is an error call function red or DontWalk failures 
					}
					
					if (!(ButtonTestReset(30))) {	cycleCounter++ ; }
					else {  
						state = BUTTONPRESSED ; 
						OperationalState = Pedestrian_Waiting; 
						cycleCounter = 0;	 // reset the voltage when the state changed 
					}
					break ;
				 
				case Pedestrian_Waiting:
					SignalSet(WAIT_S);				 
					if ( cycleCounter <= T6 ) { cycleCounter ++;    }
					else { 
						OperationalState = Car_Slowing_Down;  
						cycleCounter = 0; 
					} 				   
					break ;
					
				case Car_Slowing_Down :
					SignalReset(GREEN_S);
					intermediateValue = ProbeVoltage(AMBER_PP) ;
					if (intermediateValue >= amberProbe ) { amberProbe = intermediateValue; } 
					
					if(amberProbe < 400  || amberFaulty ==1 ) {
						SignalSet(RED_S); 
						amberFaulty = 1 ; 
					}				 
					else if (amberFaulty ==0){	  
						SignalSet (AMBER_S); //Amber on if not faulty 
					}
					
				  if ( cycleCounter <= T1 ) { cycleCounter ++;    } 
					else { 
						OperationalState = Car_Stop;
						cycleCounter = 0; 
					} 
					intermediateValue =0;
					break;					 
			 					 
				case Car_Stop :
					SignalReset(AMBER_S);
					SignalSet(RED_S);
					// testing the red light for possible failures 
					redProbe = 0 ;// reset the voltage 
					intermediateValue = ProbeVoltage(RED_PP) ;
					if (intermediateValue >= redProbe ) { redProbe = intermediateValue; }
					
					if( redProbe < 150 ) { 
						redDontWalkFailures();	/// there is an error call function red or DontWalk failures 
					}
					
					if ( cycleCounter <= T2 ) { cycleCounter ++; } 
					else { 
						OperationalState =  Pedestrian_Crossing; 
						cycleCounter = 0; 
					} 
					intermediateValue =0; 
					break;
					 
				case Pedestrian_Crossing :
					SignalReset(DONTWALK_S);
					SignalReset(WAIT_S);						 
					SignalSet(WALK_S);
					   
					if ( cycleCounter <= T3 ) { cycleCounter ++;}
					else { 
						OperationalState = Pedestrian_Stopping; 
						cycleCounter = 0; 
					}
					break;
					
				case Pedestrian_Stopping :
					SignalReset(WALK_S);
					SignalSet (DONTWALK_S); 
					   
					if ( cycleCounter <= T4 ) {  cycleCounter ++; } 
					else{ 
						OperationalState = Car_Ready_To_Move;
						cycleCounter = 0 ;
					} 
					break;
		 					 					 
				case Car_Ready_To_Move :
					SignalSet(  AMBER_S) ;
					if ( cycleCounter <= T5 ) { cycleCounter ++; } 
					else { 
						OperationalState = Car_running; 
						cycleCounter = 0 ; 
					}
					break;  				   				

			}   
		}
		WaitSysTickCounter();
	}	
}


/*
 * If the light has a probe point, take the measurement
 * This is done at the start - even though the light may be off at the start
 */
void stateLogicProbe(int nextState, PelicanSignal ps, volatile int* probeVar, ProbePoint pp) {
	int state0 = state ;
  if (probeVar) {
    *probeVar = ProbeVoltage(pp) ;
  }
  stateLogic(nextState, ps) ;
	// reset the voltage if the state has changed
	if (state != state0) {
		*probeVar = 0 ;
	}
}

/*
 * In each state
 *   - check if button pressed; if so, 
 *        + reset the light
 *        + go to the ButtonPressed state
 *   - if in first sec, set light
 *   - if in second sec, reset light
 *   - at end, reset light and go to next state
 */
void stateLogic(int nextState, PelicanSignal ps) {
  if (ButtonTestReset(30)) {
    state = BUTTONPRESSED ;
    SignalReset(ps) ;
		cycleCounter = 0 ;
  } else {
    if (cycleCounter < 20) { SignalSet(ps) ; }
    if (cycleCounter >= 20) { SignalReset(ps) ; }
    if (cycleCounter >= 40) {
      state = nextState ;
      cycleCounter = 0 ;
    } else {
      cycleCounter++ ;
    }
  }
}
void redDontWalkFailures(void) {
	SignalReset(RED_S) ;
	SignalReset(AMBER_S) ;
	SignalReset(GREEN_S) ;
	SignalReset(DONTWALK_S) ;
	SignalReset(WALK_S) ;
		// reset the counter 
	cycleCounter = 0 ;
	// Toggle Wait light
	ToggleWaitLight();
	
} 
void ToggleWaitLight(void){
	
	ResetSysTickCounter(50) ; 
	
	if ( cycleCounter < 5) { SignalSet(WAIT_S) ; }
  if (cycleCounter >= 5)  { SignalReset(WAIT_S);}
  if (cycleCounter >= 10) { cycleCounter = 0 ; return;}
     else {cycleCounter++ ;}

	WaitSysTickCounter();

	ToggleWaitLight();

 }


