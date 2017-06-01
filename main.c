/*	Name: Justin Mac (861086907)
 *	Lab Section: 21
 *	CS120B Project
 *	Project Description: Door Security System. Bluetooth module connected to 
 	PD0(RX), PD1(TX). Speaker connected to PB6.
 */ 

#include <avr/io.h>
#include "bit.h" //getBit function
#include "timer.h" //timer functions
#include "usart.h" //contains USART functions for bluetooth
#include "scheduler.h" //contains the task struct + findGCD function

//-------------------Global Variables-----------------------

unsigned char ledOutput; //lights up LEDs in hex
unsigned char bluetoothData; //bluetooth data from USART
unsigned char sound = 5;
unsigned char isLocked = 0; //boolean value

void ADC_init() { //taken from Lab 8
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

//------------------------Bluetooth_SM--------------------------
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

//------------------------LED_SM--------------------------
enum LED_States { LED_Reset, LED_On } LEDStates;

int LEDTick(int state) {
	switch(state) { //Transitions
		case LED_Reset:
			//PORTC = 0x00;
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

	switch(state) { //Actions
		case LED_Reset:
			break;
		case LED_On:
			PORTC = ledOutput;
			break;
		default:
			state = LED_Reset;
			break;
	}

	return state;
}

//------------------------IR_SM--------------------------
enum IR_States { IR_On } IRStates;

int IRTick(int state) {
	unsigned char IRflag = 0xFF; //modify
	unsigned short tempADC = ADC;
	switch(state) { //Transitions
		case IR_On:
			state = IR_On;
			break;
		default:
			state = IR_On;
			break;
	}

	switch(state) { //Actions
		case IR_On:
			ledOutput = (char)tempADC;
			PORTD = (char)tempADC >> 8;
			break;
		default:
			state = IR_On;
			break;
	}

	return state;
}

//PWM functions from lab 9
void set_PWM(double frequency) {

	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08; //stops timer/counter
		else TCCR3B |= 0x03; // resumes/continues timer/counter
		if (frequency < 0.954) OCR3A = 0xFFFF;
		else if (frequency > 31250) OCR3A = 0x0000;
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; // resets counter
		current_frequency = frequency;
	}
}

void PWM_on() { //Function from lab 9
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0);
}

void PWM_off() { //Function from lab 9
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

//Frequency to play notes
#define C4 261.63
#define D4 293.66
#define E4 329.63
#define F4 249.23
#define G4 392.00
#define A4 440.00
#define B4 493.88
#define C5 523.25

const double lockSound[16] = {C5, C5, B4, B4, A4, A4, G4, G4, F4, F4, E4, E4, D4, D4, C4, C4};
const double unlockSound[16] = {C4, C4, D4, D4, E4, E4, F4, F4, G4, G4, A4, A4, B4, B4, C5, C5};
//const double lockedOut[16] = {C4, C4, C4, C4, C4, C4, C4, C4};
const double correctPinSound[16] = {C4, C4, C4, C4, E4, E4, E4, E4, A4, A4, A4, A4, C5, C5, C5, C5};
const double incorrectPin[16] = {C5, C5, C5, C5, C5, C5, C5, C5, C4, C4, C4, C4, C4, C4, C4, C4};
//------------------------Speaker_SM--------------------------
enum SpeakerStates{SpeakInit, SpeakOff, SpeakLock, SpeakUnlock, SpeakFail, SpeakLockout, SpeakCorrect} speakstate;

void SpeakerTick(){
	static unsigned char index;
	// state transitions
	switch(speakstate){
		case SpeakInit:
			index = 0;
			speakstate = SpeakOff;
			break;

		case SpeakOff:
			if(sound == 1){
				sound = 0;
				speakstate = SpeakLock;
			}
			else if(sound == 2){
				sound = 0;
				speakstate = SpeakUnlock;
			}
			else if(sound == 3){
				sound = 0;
				speakstate = SpeakFail;
			}
			else if(sound == 4){
				sound = 0;
				speakstate = SpeakLockout;
			}
			else if(sound == 5){
				sound = 0;
				speakstate = SpeakCorrect;
			}
			else{
				speakstate = SpeakOff;
			}
			break;

		case SpeakLock:
			if(index < 16){
				speakstate = SpeakLock;
			}
			else{
				index = 0;
				speakstate = SpeakOff;
			}
			break;
		
		case SpeakUnlock:
			if(index < 16){
				speakstate = SpeakUnlock;
			}
			else{
				index = 0;
				speakstate = SpeakOff;
			}
			break;
		
		case SpeakFail:
			if(index < 16){
				speakstate = SpeakFail;
			}
			else{
				index = 0;
				speakstate = SpeakOff;
			}
			break;

		case SpeakCorrect:
			if(index < 16){
				speakstate = SpeakCorrect;
			}
			else{
				index = 0;
				speakstate = SpeakOff;
			}
			break;	
		
		case SpeakLockout:
			if(isLocked){
				speakstate = SpeakLockout;
			}
			else{
				index = 0;
				speakstate = SpeakOff;
			}
			break;
		
		default:
			speakstate = SpeakInit;
			break;
	}

	// state actions
	switch(speakstate){
		case SpeakInit:
			break;
		
		case SpeakOff:
			set_PWM(0);
			break;
		
		case SpeakLock:
			set_PWM(lockSound[index]);
			index++;
			break;	
		
		case SpeakUnlock:
			set_PWM(unlockSound[index]);
			index++;
			break;

		case SpeakFail:
			set_PWM(incorrectPin[index]);
			index++;
			break;

		case SpeakLockout:
			set_PWM(C4);
			index++;
			break;

		case SpeakCorrect:
			set_PWM(correctPinSound[index]);
			index++;
			break;
		
		default: 
			break;
	}
}


int main()
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00; //initialize C to output
	DDRD = 0x02; PORTD = 0xFD; //RX/TX input
	
	unsigned long int bluetoothPeriod = 10;
	unsigned long int ledPeriod = 10;
	unsigned long int IRPeriod = 5;
	unsigned long int speakerPeriod = 5;

	unsigned long int systemPeriod = findGCD(bluetoothPeriod,ledPeriod);
	systemPeriod = findGCD(systemPeriod, IRPeriod);
	systemPeriod = findGCD(systemPeriod, speakerPeriod);

	PWM_on();
	set_PWM(0);
	ADC_init();
	initUSART(0); //initialize to USART0
	
	static task task1, task2, task3, task4;
	task *tasks[] = { &task1, &task2, &task3, &task4 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	//Bluetooth Task
	task1.state = Bluetooth_Wait;
	task1.period = bluetoothPeriod;
	task1.elapsedTime = bluetoothPeriod;
	task1.TickFct = &BluetoothTick;

	//LED Task
	task2.state = LED_Reset;
	task2.period = ledPeriod;
	task2.elapsedTime = ledPeriod;
	task2.TickFct = &LEDTick;

	//IR Task
	task3.state = IR_On;
	task3.period = IRPeriod;
	task3.elapsedTime = IRPeriod;
	task3.TickFct = &IRTick;

	//Speaker Task
	task4.state = SpeakOff;
	task4.period = speakerPeriod;
	task4.elapsedTime = speakerPeriod;
	task4.TickFct = &SpeakerTick;

	// Set the timer and turn it on
	TimerSet(systemPeriod); //period
	TimerOn();
	
	unsigned short i;
	while(1) {
		for ( i = 0; i < numTasks; i++ ) {
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}

	//Program should not exit
	return 0;
}

