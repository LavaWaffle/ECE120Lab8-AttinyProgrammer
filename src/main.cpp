/* Name: main.cpp
 * Author: Anand Parab
 * Description: Source code for Arduino Uno to program attiny13 chip for ECE120 LAB8 (Coin Acceptor Simulator + Debounce Function)
 * Last Edit: November 12, 2025
 */

#include "Adafruit_AVRProg.h"

Adafruit_AVRProg avrprog = Adafruit_AVRProg();

/*
 * Pins to target
 */
#define AVRPROG_SCK 13
#define AVRPROG_MISO 12
#define AVRPROG_MOSI 11
#define AVRPROG_RESET 10

#define LED_PROGMODE LED_BUILTIN
#define LED_ERR LED_BUILTIN

#define LED_SUCCESS 2

#define BUTTON A5 // use the board's reset button!
#define PIEZOPIN 8


// ===================================================================
// =================== STEP 1: CHIP DEFINITION =======================
// ===================================================================
//
// This is the correct way to define the image.
// We follow the ATtiny85 example and the Adafruit_AVRProg.h file.
// The hex code is placed *inside* the struct initializer.
//
const image_t attiny13a_image PROGMEM = {
    // char image_name[30];
    "lab8attiny.ino.hex",
    // char image_chipname[12];
    "ATtiny13A",
    // uint16_t image_chipsig;
    0x9007,
    // byte image_progfuses[10];
    {0x3F, 0x62, 0xFB, 0xFF, 0, 0, 0, 0, 0, 0},
    // byte image_normfuses[10];
    {0x3F, 0x62, 0xFB, 0xFF, 0, 0, 0, 0, 0, 0},
    // byte fusemask[10];
    {0x3F, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0},
    // uint16_t chipsize; (from your .h file)
    1024,
    // byte image_pagesize; (from your .h file)
    32,
    // byte image_hexcode[];
    {R"(
:1000000009C016C015C025C013C012C011C010C051
:100010000FC00EC011241FBECFE9CDBF20E0A0E667
:10002000B0E001C01D92A436B207E1F737D0D0C0CE
:10003000E7CF82B7855D89BD88B7837F886088BF39
:10004000089582B7855D86BF88B7837F846088BF47
:1000500008951F920F920FB60F9211248F939F93C2
:10006000AF93BF938091600090916100A091620076
:10007000B09163000196A11DB11D80936000909323
:100080006100A0936200B0936300BF91AF919F9114
:100090008F910F900FBE0F901F901895B99ABA9A32
:1000A000BC98C49ABB98C39AB898C09A83B7856025
:1000B00083BFFF24F394B09B3BC0B4991FC0C29A86
:1000C00087E99AE30197F1F700C00000C19A87E938
:1000D0009AE30197F1F700C00000C29887E99AE31C
:1000E0000197F1F700C00000C1989FED2CE183E07B
:1000F000915020408040E1F700C00000B399DBCF71
:1001000087E99AE30197F1F700C00000C19A8FE2F6
:1001100095E70197F1F700C00000C1989FED2CE131
:1001200083E0915020408040E1F700C00000C3CF41
:10013000C298C19810E000E0D0E0C0E0C230D10524
:10014000E9F0C330D10551F12197F9F0B3992CC0F2
:10015000C29A77DFC3E0D0E00230110541F10330ED
:10016000110581F10130110549F1B499A4CFC19A6B
:1001700060DF03E010E0209709F79DCFB39BECCF41
:10018000C2985FDFC1E0D0E0E7CF88B7869586955B
:100190008F258170C82F90E0D92FDECF08B602FEE0
:1001A000DBCFC2E0D0E0D8CFD0E0C0E0D5CFB49B69
:1001B000E2CFC1983EDF01E010E0DDCF08B603FEDC
:1001C000DACF79CF08B603FED6CF02E010E0D3CF66
:0401D000F894FFCFD1
:00000001FF
)"}};
// Note: The R"(...)" must be inside the curly braces { ... }


// ===================================================================
// =================== STEP 2: PROGRAMMING LOGIC =====================
// ===================================================================

void setup() {
  Serial.begin(115200); /* Initialize serial for status msgs */

  while (!Serial)
    ;
  delay(100);

  Serial.println("\nATtiny13A Standalone Programmer");

  pinMode(PIEZOPIN, OUTPUT);
  pinMode(LED_SUCCESS, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP); // button for next programming

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

  // === Point the programmer at OUR chip data ===
  const image_t *targetimage = &attiny13a_image;

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

  // Beep for success!
#if !defined(ESP32)
  tone(PIEZOPIN, 4000, 200);
#endif

  Serial.println("\n\n--- SUCCESS! ---");
  Serial.println("Chip is programmed. Ready for the next one.");

  digitalWrite(LED_SUCCESS, HIGH);
  delay(500);
  digitalWrite(LED_SUCCESS, LOW);
  delay(500);

  // This loop() will run again, waiting for the next 'G' or button press.
  // This allows you to program chips in bulk.
}
