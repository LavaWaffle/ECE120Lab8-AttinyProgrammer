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
:1000000009C016C015C01EC013C012C011C010C058
:100010000FC00EC011241FBECFE9CDBF20E0A0E667
:10002000B0E001C01D92A436B207E1F730D06FC036
:10003000E7CF26E131E0019708F40895F9013197FF
:10004000F1F7F9CF1F920F920FB60F9211248F93F1
:100050009F93AF93BF938091600090916100A091B6
:100060006200B09163000196A11DB11D80936000F4
:1000700090936100A0936200B0936300BF91AF9131
:100080009F918F910F900FBE0F901F9018958FB575
:1000900083608FBD89B7826089BF83B7816083BF6A
:1000A00012BE789483E886B9B99ABA9A8FB58F7CD4
:1000B0008FBDC198C2988FB7F894BB98C39A8FBF71
:1000C0008FB7F894BC98C49A8FBFB3990FC08FB5FF
:1000D0008F7C8FBDC19A88EC90E0ABDF8FB58F7CB1
:1000E0008FBDC19884E690E0A4DFEFCFB499EDCF47
:1000F000C29A84E190E09DDF8FB58F7C8FBDC19A5D
:1001000088EC90E096DFC29884E190E0E6CFF89426
:02011000FFCF1F
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
