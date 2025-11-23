/* Name: main.cpp
 * Author: Anand Parab
 * Description: Source code for Arduino Uno to program Attiny13A chip for ECE120 LAB8 (Coin Acceptor Simulator + Debounce Function)
 * Last Edit: November 12, 2025
 */

#include "Adafruit_AVRProg.h" 
#include "image.h" 

Adafruit_AVRProg avrprog = Adafruit_AVRProg();

// Arduino Uno pin definitions for programming
#define AVRPROG_SCK 13
#define AVRPROG_MISO 12
#define AVRPROG_MOSI 11
#define AVRPROG_RESET 10

// Arduino Uno pin definitions for status LEDs and button
#define LED_PROGMODE LED_BUILTIN
#define LED_ERR LED_BUILTIN

#define LED_SUCCESS 2

#define BUTTON A5 // use the board's reset button!

void setup() {
  // Start serial for output
  Serial.begin(115200); 

  while (!Serial);
  delay(100);

  Serial.println("\nATtiny13A Standalone Programmer");

  // Setup pins
  pinMode(LED_SUCCESS, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP); 

  avrprog.setProgramLED(LED_PROGMODE);
  avrprog.setErrorLED(LED_ERR);
  avrprog.setSPI(AVRPROG_RESET, AVRPROG_SCK, AVRPROG_MOSI, AVRPROG_MISO);
}

void loop(void) {
start:
  Serial.println("\nType 'G' or hit BUTTON to program the ATtiny13A");
  while (1) {
    if (Serial.read() == 'G') {
      break;
    }
    if (!digitalRead(BUTTON)) {
      while (!digitalRead(BUTTON)) {
        delay(10);
      }          // wait for release
      delay(10); // debounce
      break;
    }
  }

  // Get the image to program
  const image_t *targetimage = &attiny13a_image;

  // === Programming Sequence ===
  if (!avrprog.targetPower(true)) {
    avrprog.error("Failed to connect to target");
    goto start;
  }

  Serial.print(F("\nReading signature: "));
  uint16_t signature = avrprog.readSignature();
  Serial.println(signature, HEX);
  if (signature == 0 || signature == 0xFFFF) {
    avrprog.error(F("No target attached?"));
    goto start;
  }

  // === Check if the attached chip signature (e.g., 0x9007) matches our definition ===
  // We use pgm_read_word to read the value from PROGMEM
  if (pgm_read_word(&targetimage->image_chipsig) != signature) {
    avrprog.error(F("Signature doesn't match! Expected 0x9007"));
    goto start;
  }
  Serial.println(F("Found matching chip/image"));

  Serial.print(F("Erasing chip..."));
  avrprog.eraseChip();
  Serial.println(F("Done!"));

  // === Program the fuses we defined above ===
  if (!avrprog.programFuses(
        targetimage->image_progfuses)) { // get fuses ready to program
    avrprog.error(F("Programming Fuses fail"));
    goto start;
  }

  if (!avrprog.verifyFuses(targetimage->image_progfuses,
                           targetimage->fusemask)) {
    avrprog.error(F("Failed to verify fuses"));
    goto start;
  }

  // === Write our HEX file from PROGMEM to the chip's flash ===
  Serial.println("Writing Image to Chip");
  if (!avrprog.writeImage((const byte *)targetimage->image_hexcode, // Pass the pointer to the hex code
                          pgm_read_byte(&targetimage->image_pagesize),
                          pgm_read_word(&targetimage->chipsize))) {
    avrprog.error(F("Failed to write flash"));
    goto start;
  }

  Serial.println(F("\nVerifying flash..."));
  if (!avrprog.verifyImage((const byte *)targetimage->image_hexcode)) {
    avrprog.error(F("Failed to verify flash"));
    goto start;
  }
  Serial.println(F("\nFlash verified correctly!"));

  // Set fuses to 'final' state (we just use the same ones)
  if (!avrprog.programFuses(targetimage->image_normfuses)) {
    avrprog.error("Programming fuses fail");
    goto start;
  }

  if (!avrprog.verifyFuses(targetimage->image_normfuses,
                           targetimage->fusemask)) {
    avrprog.error("Failed to verify fuses");
    goto start;
  } else {
    Serial.println("Fuses verified correctly!");
  }
  // === End Programming Sequence ===

  Serial.println("\n\n--- SUCCESS! ---");
  Serial.println("Chip is programmed. Ready for the next one.");

  // Blink success LED
  digitalWrite(LED_SUCCESS, HIGH);
  delay(500);
  digitalWrite(LED_SUCCESS, LOW);
  delay(500);
}
