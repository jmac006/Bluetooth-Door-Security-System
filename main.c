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
unsigned char soundType = 1;
unsigned char isLocked = 1; //boolean value
unsigned char passAttempt = 0; //counts number of incorrect password attempts

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
const double alarmSound[16] = {G4, G4, G4, G4, C4, C4, C4, C4, G4, G4, G4, G4, C4, C4, C4, C4};
const double correctPinSound[16] = {C4, C4, C4, C4, E4, E4, E4, E4, A4, A4, A4, A4, C5, C5, C5, C5};
const double incorrectPin[16] = {C5, C5, C5, C5, C5, C5, C5, C5, C4, C4, C4, C4, C4, C4, C4, C4};

#define PIR_DDR  DDRA
#define PIR_READ PINA
#define PIR_PIN  0

unsigned char pir_read()
{
	PIR_DDR &= ~(1 << PIR_PIN); // Set input
	return ((PIR_READ & ( 1 << PIR_PIN) ) >> PIR_PIN); 
}

void ADC_init() { //taken from Lab 8
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
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


//------------------------Bluetooth_SM--------------------------
enum Bluetooth_States { bluetoothInit, bluetoothReceive1, bluetoothReceive2, bluetoothReceive3 } bluetoothStates;

unsigned char password1 = 0x01;
unsigned char password2 = 0x02;
unsigned char password3 = 0x03;
unsigned char trypass1, trypass2, trypass3;

int BluetoothTick(int state) {
	switch(state) { //Transitions
		case bluetoothInit:
			state = bluetoothInit;
			//if usart receives bluetooth signal, parameter is USARTnum
			if( USART_HasReceived(0) ) {
				bluetoothData = USART_Receive(0);
				USART_Flush(0);
				state = bluetoothReceive1;
			}
			break;
		case bluetoothReceive1:
			state = bluetoothReceive1;
			if( USART_HasReceived(0) ) {
				bluetoothData = USART_Receive(0);
				USART_Flush(0);
				state = bluetoothReceive2;
			}
			break;
		case bluetoothReceive2:
			state = bluetoothReceive2;
			if( USART_HasReceived(0) ) {
				bluetoothData = USART_Receive(0);
				USART_Flush(0);
				state = bluetoothReceive3;
			}
			break;
		case bluetoothReceive3:
			state = bluetoothReceive3;
			if( USART_HasReceived(0) ) {
				bluetoothData = USART_Receive(0);
				USART_Flush(0);
				state = bluetoothInit;
			}
			break;
		default:
			state = bluetoothInit;
			break;
	}
	
	switch (state) { //Actions
		case bluetoothInit:
			soundType = 0; //no sound
			break;
		case bluetoothReceive1:
			ledOutput = bluetoothData;
			trypass1 = bluetoothData;
			if(bluetoothData == 0xFF) { //lock the door
				isLocked = 1;
				soundType = 0;
				state = bluetoothInit;
			}
			break;
		case bluetoothReceive2:
			ledOutput = bluetoothData;
			trypass2 = bluetoothData;
			if(bluetoothData == 0xFF) {
				isLocked = 1;
				soundType = 0;
				state = bluetoothInit;
			}
			break;
		case bluetoothReceive3:
			if(bluetoothData == 0xFF) {
				isLocked = 1;
				soundType = 0;
				state = bluetoothInit;
			}
			ledOutput = bluetoothData;
			trypass3 = bluetoothData;
			if(trypass1 == password1 && trypass2 == password2 && trypass3 == password3) {
				isLocked = 0; //unlock the door
				ledOutput = 0xFF;
				soundType = 5; //correct password sound
				passAttempt = 0; //reset the password attempts
				state = bluetoothInit;
			}
			else {
				isLocked = 1; //door should still be locked
				passAttempt++;
				soundType = 3; //incorrect password sound
				state = bluetoothInit;
			}
			if(passAttempt == 3) {
				soundType = 4; //alarm sound if user incorrectly enters pass 3 times
			}
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
			PORTC = 0x00;
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
#define PIR_sensor PA0
enum PIR_States { IR_On } PIRStates;

//PIR Sensor information found from: 
//https://www.exploreembedded.com/wiki/PIR_motion_Sensor_interface_with_Atmega128
int IRTick(int state) {
	DDRA = (0 << PIR_sensor);
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
			if(((PINA) & (1<<PIR_sensor)) == 1 && isLocked == 1) { //Motion detected
				ledOutput = 0x01;
				soundType = 4; //alarm sound
			}
			break;
		default:
			break;
	}

	return state;
}

//------------------------Speaker_SM--------------------------
enum SpeakerStates{SpeakerSetup, LockSound, UnlockSound, FailSound, AlarmSound, CorrectSound} speakerState;

void SpeakerTick(){
	static unsigned char index;
	// state transitions
	switch(speakerState){

		case SpeakerSetup:
			if(soundType == 1){
				soundType = 0;
				speakerState = LockSound;
			}
			else if(soundType == 2){
				soundType = 0;
				speakerState = UnlockSound;
			}
			else if(soundType == 3){
				soundType = 0;
				speakerState = FailSound;
			}
			else if(soundType == 4){
				soundType = 0;
				speakerState = AlarmSound;
			}
			else if(soundType == 5){
				soundType = 0;
				speakerState = CorrectSound;
			}
			else{
				speakerState = SpeakerSetup;
			}
			break;

		case LockSound:
			if(index < 16){
				speakerState = LockSound;
			}
			else{
				index = 0;
				speakerState = SpeakerSetup;
			}
			break;
		
		case UnlockSound:
			if(index < 16){
				speakerState = UnlockSound;
			}
			else{
				index = 0;
				speakerState = SpeakerSetup;
			}
			break;
		
		case FailSound:
			if(index < 16){
				speakerState = FailSound;
			}
			else{
				index = 0;
				speakerState = SpeakerSetup;
			}
			break;

		case CorrectSound:
			if(index < 16){
				speakerState = CorrectSound;
			}
			else{
				index = 0;
				speakerState = SpeakerSetup;
			}
			break;	
		
		case AlarmSound:
			if(index < 16){
				speakerState = AlarmSound;
				if (index == 15 && isLocked == 1) {
					index = 0; //repeat alarm
				}
			}
			else{
				index = 0;
				speakerState = SpeakerSetup;
			}
			break;
		
		default:
			speakerState = SpeakerSetup;
			break;
	}

	switch(speakerState){ //Actions
		case SpeakerSetup:
			set_PWM(0);
			break;
		
		case LockSound:
			set_PWM(lockSound[index]);
			index++;
			break;	
		
		case UnlockSound:
			set_PWM(unlockSound[index]);
			index++;
			break;

		case FailSound:
			set_PWM(incorrectPin[index]);
			index++;
			break;

		case AlarmSound:
			set_PWM(alarmSound[index]);
			index++;
			break;

		case CorrectSound:
			set_PWM(correctPinSound[index]);
			index++;
			break;
		
		default: 
			break;
	}
}

int main()
{
	DDRA = 0x00; PORTA = 0xFF; //initialize A to input
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00; //initialize C to output
	DDRD = 0x02; PORTD = 0xFD; //TX = output, rest are inputs
	
	unsigned long int bluetoothPeriod = 10;
	unsigned long int ledPeriod = 10;
	unsigned long int PIRPeriod = 5;
	unsigned long int speakerPeriod = 5;

	unsigned long int systemPeriod = findGCD(bluetoothPeriod,ledPeriod);
	systemPeriod = findGCD(systemPeriod, PIRPeriod);
	systemPeriod = findGCD(systemPeriod, speakerPeriod);

	PWM_on();
	set_PWM(0);
	ADC_init();
	initUSART(0); //initialize to USART0
	
	static task task1, task2, task3, task4;
	task *tasks[] = { &task1, &task2, &task3, &task4 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	//Bluetooth Task
	task1.state = bluetoothInit;
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
	task3.period = PIRPeriod;
	task3.elapsedTime = PIRPeriod;
	task3.TickFct = &IRTick;

	//Speaker Task
	task4.state = SpeakerSetup;
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

