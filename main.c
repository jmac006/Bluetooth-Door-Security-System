/*	Name: Justin Mac (861086907)
 *	Lab Section: 21
 *	CS120B Project
 *	Exercise Description: Keypad connected to PORTC, LCD on PORTD, with RS on A0 and Enable on A1.
 	This exercise displays the key that is pressed.
 */ 

#include <avr/io.h>
#include "bit.h" //getBit function
#include "timer.h" //timer functions
#include "usart.h" //contains USART functions for bluetooth
#include "scheduler.h" //contains the task struct + GCD function
//-------------------Global Variables-----------------------

unsigned char ledOutput; //lights up LEDs in hex
unsigned char bluetoothData; //bluetooth data from USART

//-------------------End Global Variables-------------------


//------------------------Bluetooth_SM---------------------------------
enum Bluetooth_States { Bluetooth_Wait, Bluetooth_Receive } bluetoothStates;

int BluetoothTick(int state) {
	switch(state) { //Transitions
		case Bluetooth_Wait:
			state = Bluetooth_Wait;
			//if usart receives bluetooth signal, parameter is USARTnum
			if( USART_HasReceived(0) ) {
				bluetoothData = USART_Receive(0);
				USART_Flush(0);
				state = Bluetooth_Receive;
			}
			break;
		case Bluetooth_Receive:
			state = Bluetooth_Wait;
			break;
		default:
			state = Bluetooth_Wait;
			break;
	}
	
	switch (state) { //Actions
		case Bluetooth_Wait:
			break;
		case Bluetooth_Receive:
			ledOutput = bluetoothData;
			break;
		default:
			break;
	}

	return state;
}

enum LED_States { LED_Reset, LED_On } LEDStates;

int LEDTick(int state) {
	switch(state) { //Transitions
		case LED_Reset:
			//PORTA = 0x00;
			if(ledOutput != 0x00) {
				state = LED_On;
			}
			break;
		case LED_On:
			if(ledOutput == 0x00) {
				state = LED_Reset;
			}
			break;
		default:
			state = LED_Reset;
			break;
	}

	switch(state) { //Action
		case LED_Reset:
			break;
		case LED_On:
			PORTA = ledOutput;
			break;
		default:
			state = LED_Reset;
			break;
	}

	return state;
}



int main()
{
	//DDRC = 0xF0; PORTC = 0x0F; // PC7..4 outputs init 0s, PC3..0 inputs init 1s
	DDRB = 0xFF; PORTB = 0x00;
	DDRA = 0xFF; PORTA = 0x00; //initialize A to output
	DDRD = 0x02; PORTD = 0xFD; //RX/TX input
	
	unsigned long int bluetoothPeriod = 10;
	unsigned long int ledPeriod = 10;
	initUSART(0); //initialize to USART0
	
	//Declare an array of tasks
	//static task task1, task2;
	//task *tasks[] = { &task1, &task2};
	static task task1, task2;
	task *tasks[] = { &task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = Bluetooth_Wait;//Task initial state.
	task1.period = bluetoothPeriod;//Task Period.
	task1.elapsedTime = bluetoothPeriod;//Task current elapsed time.
	task1.TickFct = &BluetoothTick;//Function pointer for the tick.

	task2.state = LED_Reset;//Task initial state.
	task2.period = ledPeriod;//Task Period.
	task2.elapsedTime = ledPeriod;//Task current elapsed time.
	task2.TickFct = &LEDTick;//Function pointer for the tick.

	// Set the timer and turn it on
	TimerSet(10); //period
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

