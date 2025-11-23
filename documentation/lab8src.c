/* Name: main.c
 * Author: Saidivya Ashok
 * Description: Source code for attiny13 chip for ECE120 LAB8 (Coin Acceptor Simulator + Debounce Function)
 * Last Edit: July 30, 2020
 */

#include <avr/io.h>
#include <util/delay.h>


/* Description - Check if quarter switch (or B if in debounce mode) is pressed */
int is_quarter_B(){
    //check if PB4 is pressed
    if(!(PINB & (1 << PB4))){
        return 1;
    }
    return 0;
}

/* Description - Check if dime switch (or A if in debounce mode) is pressed */
int is_dime_A(){
    //check if PB3 is pressed
    if(!(PINB & (1 << PB3))){
        return 1;
    }
    return 0;
}

/* coin simulator function */
void coin(){
	if (is_quarter_B()) {
        PORTB |= (1 << PB2);    // quarter inserted, T = 1
        _delay_ms(50);
        PORTB |= (1 << PB1);    // quarter inserted, CLK = 1
        _delay_ms(50);
        PORTB &= ~(1 << PB2);   // turn off T
        _delay_ms(50);
        PORTB &= ~(1 << PB1);   // turn off CLK
        _delay_ms(850);
	}
    if (is_dime_A()) {
        _delay_ms(50);
        PORTB |= (1 << PB1);    // dime inserted, CLK = 1
        _delay_ms(100);
        PORTB &= ~(1 << PB1);   // turn off CLK
        _delay_ms(850);
	}
}

/* declaration of FSM states */
enum btns_state {
	UP_ARMED,
	UP_WAIT,
	DOWN_ARMED,
	DOWN_WAIT
};

/* Description - Initialize and start counter for A */
void start_a() {
	// We're running at 1.2 MHz, with a pre-scaler value of 1024, this is
	// a timer update frequency of 1.17 kHz (tick period of 0.85 ms). We want
	// a delay of 50 ms, so we need to wait 43 clock ticks.
    OCR0A = TCNT0 + 43;
	TIFR0 = (TIFR0 & ~(1 << OCF0B)) | (1 << OCF0A);
}

/* Description - Initialize and start counter for B */
void start_b(){
    // We're running at 1.2 MHz, with a pre-scaler value of 1024, this is
    // a timer update frequency of 1.17 kHz (tick period of 0.85 ms). We want
    // a delay of 50 ms, so we need to wait 43 clock ticks.
    OCR0B = TCNT0 + 43;
    TIFR0 = (TIFR0 & ~(1 << OCF0A)) | (1 << OCF0B);
}

/* Description - Check if counter for A has expired */
int check_counterA(){
    if(TIFR0 & (1<<OCF0A)){
        return 1;
    }
    return 0;
}

/* Description - Check if counter for B has expired */
int check_counterB(){
    if(TIFR0 & (1<<OCF0B)){
        return 1;
    }
    return 0;
}

/* Debounce function */
void btns() {
    //Upon entry into the function, both A and B are in the UP_ARMED state
	enum btns_state a_state = UP_ARMED;
	enum btns_state b_state = UP_ARMED;
    PORTB &= ~(1 << PB2);   // A = 0; turn LED OFF
    PORTB &= ~(1 << PB1);   // B = 0; turn LED OFF
	
    //perform functionality depending on FSM state
	do {
		switch (a_state) {
        //UP_ARMED = wait for switch to be pressed then turn LED ON & start counter
		case UP_ARMED:
			if (is_dime_A()) {
				a_state = DOWN_WAIT;
                PORTB |= (1 << PB2);    // A = 1; turn LED ON
				start_a();
			}
			break;
		case DOWN_ARMED:
         //DOWN_ARMED = wait for switch to be released then turn LED OFF & start counter
			if (!is_dime_A()) {
				a_state = UP_WAIT;
                PORTB &= ~(1 << PB2);   // A = 0; turn LED OFF
				start_a();
			}
			break;
		case UP_WAIT:
        //UP_WAIT = wait for counter to expire
            if (check_counterA()){
                a_state = UP_ARMED;
            }
			break;
		case DOWN_WAIT:
        //DOWN_WAIT = wait for counter to expire
            if (check_counterA()){
                a_state = DOWN_ARMED;
            }
			break;
		}
		
		// ... same FSM for B
        
        switch (b_state) {
        case UP_ARMED:
        //UP_ARMED = wait for switch to be pressed then turn LED ON & start counter
            if (is_quarter_B()) {
                b_state = DOWN_WAIT;
                PORTB |= (1 << PB1);    // B = 1; turn LED ON
                start_b();
            }
            break;
        case DOWN_ARMED:
        //DOWN_ARMED = wait for switch to be released then turn LED OFF & start counter
            if (!is_quarter_B()) {
                b_state = UP_WAIT;
                PORTB &= ~(1 << PB1);   // B = 0; turn LED OFF
                start_b();
            }
            break;
        case UP_WAIT:
        //UP_WAIT = wait for counter to expire
            if (check_counterB()){
                b_state = UP_ARMED;
            }
            break;
        case DOWN_WAIT:
        //DOWN_WAIT = wait for counter to expire
            if (check_counterB()){
                b_state = DOWN_ARMED;
            }
            break;
        }
	} while (a_state != UP_ARMED && b_state != UP_ARMED);
    // exit function if both are in UP_ARMED state (go back to IDLE state)
}

int main(void){
    // output pin
    DDRB |= (1 << PB1);     // LED1 on (PB1)  (CLK) or (output B)
    DDRB |= (1 << PB2);     // LED2 on (PB2)  (T) or (output A)
    // input pin
    DDRB &= ~(1 << PB4);    // switch1 on (PB4) (Quarter) or (input B)
    PORTB |= (1 << PB4);    // enable pull-up resistor for PB4
    DDRB &= ~(1 << PB3);    // switch2 on (PB3) (Dime) or (input A)
    PORTB |= (1 << PB3);    // enable pull-up resistor for PB3
    DDRB &= ~(1 << PB0);    // input P (to toggle between coin and debounce mode) on (PB0)
    PORTB |= (1 << PB0);    // enable pull-up resistor for PB0
    
    TCCR0B |= (1<<CS02) | (1<<CS00)| (0<<CS01);  //set pre-scalar to 1024
    
    while(1){
        if(PINB & (1 << PB0)){ //check if PB0 is NOT connected to GND (P=1)
            coin();  // go to coin acceptor function
        }
        else{ // if PB0 IS connected to GND
            btns(); // go to debounce function
        }
    }
}





