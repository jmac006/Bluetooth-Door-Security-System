/*	Name: Justin Mac (861086907)
 *	Lab Section: 21
 *	CS120B Project
 *	Exercise Description: Keypad connected to PORTC, LCD on PORTD, with RS on A0 and Enable on A1.
 	This exercise displays the key that is pressed.
 */ 

#include <avr/io.h>
#include "bit.h"
#include "timer.h"
#include "io.c"
#include "usart.h"

// Returns '\0' if no key pressed, else returns char '1', '2', ... '9', 'A', ...
// If multiple keys pressed, returns leftmost-topmost one
// Keypad must be connected to port C
unsigned char GetKeypadKey() {

	PORTC = 0xEF; // Enable col 4 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('1'); }
	if (GetBit(PINC,1)==0) { return('4'); }
	if (GetBit(PINC,2)==0) { return('7'); }
	if (GetBit(PINC,3)==0) { return('*'); }

	// Check keys in col 2
	PORTC = 0xDF; // Enable col 5 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('2'); }
	if (GetBit(PINC,1)==0) { return('5'); }
	if (GetBit(PINC,2)==0) { return('8'); }
	if (GetBit(PINC,3)==0) { return('0'); }
	// ... *****FINISH*****

	// Check keys in col 3
	PORTC = 0xBF; // Enable col 6 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('3'); }
	if (GetBit(PINC,1)==0) { return('6'); }
	if (GetBit(PINC,2)==0) { return('9'); }
	if (GetBit(PINC,3)==0) { return('#'); }
	// ... *****FINISH*****

	// Check keys in col 4
	PORTC = 0x7F;
	asm("nop");
	if (GetBit(PINC,0)==0) { return('A'); }
	if (GetBit(PINC,1)==0) { return('B'); }
	if (GetBit(PINC,2)==0) { return('C'); }
	if (GetBit(PINC,3)==0) { return('D'); }
	// ... *****FINISH*****

	return('\0'); // default value

}

//------------------------Tasks---------------------------------
typedef struct task {
	int state;					// Task's current state
	unsigned long period;		// Task period
	unsigned long elapsedTime;	// Time elapsed since last task
	int (*TickFct)(int);		// Task tick function
} task;
//------------------------Tasks---------------------------------

//------------------------Shared Variables-----------------------
unsigned char keypadOutput;
//------------------------End Shared Variables-------------------

//-----------------------Keypad_SM-------------------------------
enum KeypadStates {keypadStart, keypadGet };

int GetKeypadTick(int state) {
	switch (state) {
		case keypadStart:
			state = keypadGet;
			break;
		case keypadGet:
			break;
	}
	
	switch (state) {
		case keypadStart:
			break;
		case keypadGet:
			keypadOutput = GetKeypadKey();
			break;
	}
	
	return state;
}
//-----------------------Keypad_SM-------------------------------

//------------------------LED_SM---------------------------------
enum LCD_States { LCD_Start, LCD_Display };

int LCDTick(int state) {
	switch(state) {
		case LCD_Start:
			state = LCD_Display;
			break;
		case LCD_Display:
			break;
		default:
			state = LCD_Start;
			break;
	}
	
	switch (state) {
		case LCD_Start:
			break;
		case LCD_Display:
			if (keypadOutput != NULL) {
				LCD_Cursor(1);
				LCD_WriteData(keypadOutput);
			}
			break;
		default:
		break;
	}

	return state;
}
//------------------------LED_SM---------------------------------


int main()
{
	DDRC = 0xF0; PORTC = 0x0F; // PC7..4 outputs init 0s, PC3..0 inputs init 1s
	DDRA = 0xFF; PORTA = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	unsigned long int keypadPeriod = 5;
	unsigned long int LCDPeriod = 10;
	
	LCD_init();
	
	//Declare an array of tasks
	static task task1, task2;
	task *tasks[] = { &task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = keypadStart;//Task initial state.
	task1.period = keypadPeriod;//Task Period.
	task1.elapsedTime = keypadPeriod;//Task current elapsed time.
	task1.TickFct = &GetKeypadTick;//Function pointer for the tick.

	// Task 2
	task2.state = LCD_Start;//Task initial state.
	task2.period = LCDPeriod;//Task Period.
	task2.elapsedTime = LCDPeriod;//Task current elapsed time.
	task2.TickFct = &LCDTick;//Function pointer for the tick.

	// Set the timer and turn it on
	TimerSet(5); //period
	TimerOn();
	
	unsigned short i; // Scheduler for-loop iterator
	while(1) {
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}

	// Error: Program should not exit!
	return 0;
}

