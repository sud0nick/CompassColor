#include <SoftwareSerial.h>
#define txPin    11
#define RedPin   3
#define GreenPin 5
#define BluePin  6

// Function prototypes
void writeColors(struct Color colors);
unsigned long getValFromString(String str);
int getCount(char *arr[]);
void setCurrentColor(String colorCode);
void setTargetColor(String colorCode);
void fade(int level, Color color);
void fadeIn();
void fadeOut();
void changeLight();

// Set up software serial pins
SoftwareSerial mySerial(txPin, NULL);

// Create a struct for holding color values
struct Color {
  unsigned long red, green, blue;
};

// Create a current color and target color struct
Color currentColor, targetColor;
                       
char * shiftColors[] = {"ff0346", "120aff", "b32f82", "08ffc5", "2e1fff", "ff0000", NULL};
char * partyColors[] = {"ff4b0a", "ff540a", "ff5d0d", "ff5c0a", "ff660d", "ff6e0d", "ff760d", "ff7e0d", "ff860d", "ff8e0d",
                        "ff960d", "ff9e0d", "ffa60d", "ffae0d", "ffb60d", "e7ff0a", "deff0a", "d6ff0a", "ceff0a", "c6ff0a",
                        "beff0a", "b6ff0a", "adff0a", "a5ff0a", "9dff0a", "95ff0a", "8dff0a", "85ff0a", "7cff0a", "74ff0a",
                        "6cff0a", "64ff0a", "5cff0a", "0aff74", "0aff7c", "0aff85", "0aff8d", "0aff95", "0aff9d", "0affa5",
                        "0affad", "0affb6", "08ffb5", "08ffbd", "08ffc5", "08ffce", "08ffd6", "08ffde", "08ffe6", "087bff",
                        "0873ff", "086bff", "0a6cff", "0a64ff", "0a5cff", "0a54ff", "0a4bff", "0a43ff", "0a3bff", "0a33ff",
                        "0a2bff", "0a23ff", "0a1bff", "0a12ff", "0a0aff", "120aff", "1b0aff", "b508ff", "bd08ff", "c508ff",
                        "ce08ff", "d608ff", "e608ff", "de08ff", "ef08ff", "f708ff", "ff08ff", "ff08f7", "ff08ef", "ff08e6",
                        "ff08de", "ff08d6", "ff08ce", "ff08c5", "ff08bd", "ff08b5", "ff08ad", "ff0862", "ff085a", "ff0852",
                        "0818FF", "ff0a4b", "ff0a43", "ff0a3b", "ff0a33", "ff0a2b", "ff0a23", NULL};

// Variables
boolean running, party, shift, reverse, pulse;
int partyCount, index, shiftIndex, shiftCount;
int brightness = 0, increment = 0, lightDelay = 5;
unsigned long timer;

void setup() {
  pinMode(RedPin, OUTPUT);
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin, OUTPUT);
  
  // Set the boolean values to false at startup except for running
  party = shift = reverse = pulse = false;

  // Set a default color at startup
  setCurrentColor("1212FF");
  fadeIn();
  running = true;
  
  // Get a count of the colors in each mode array
  partyCount = getCount(partyColors);
  shiftCount = getCount(shiftColors);
  
  Serial.begin(9600);
  Serial.println("Serial running...");
  mySerial.begin(9600);
  Serial.println("bluetooth serial running...");
}

void loop() {
  if (mySerial.available() > 0) {
    String recvBuffer = "";
    while (mySerial.available() > 0) {
      recvBuffer += (char)mySerial.read();
      if (recvBuffer.length() == 6) {
        if (recvBuffer == "xxonxx") {
          fadeIn();
          running = true;
        } else if (recvBuffer == "xxoffx") {
          fadeOut();
          running = false;
        } else if (recvBuffer == "xshift") {

          party = false;
          shift = true;
          shiftIndex = 0;
          timer = NULL;
      
          // Set the target color to the first color in the shiftColors array
          // so it can begin fading into it.
          setTargetColor(shiftColors[0]);
          
        } else if (recvBuffer == "xpulse") {
          
          pulse = true;
          shift = party = false;
          
        } else if (recvBuffer == "xparty") {
          
          index = 0;
          lightDelay = 4;
          party = true;
          shift = false;
      
          //Start at the beginning of the spectrum
          setCurrentColor(partyColors[0]);
          
        } else {
          Serial.println(recvBuffer);
          shift = party = pulse = false;
          memset(&currentColor, 0, sizeof(currentColor));
          currentColor = (Color){getValFromString(recvBuffer.substring(0,2)),
                                 getValFromString(recvBuffer.substring(2,4)),
                                 getValFromString(recvBuffer.substring(4,6))};
          writeColors(currentColor);
        }
        break;
      }
    }
  }
  
  // Check if the light should be running
  if (running) {
    // Write the colors out to the pins
    changeLight();
    
    // Select the next color from the array if spectrum mode is on
    if (party) {
      reverse = (index == 0) ? false : (index == partyCount) ? true : reverse;
      index += (reverse == true) ? -1 : 1;
      setTargetColor(partyColors[index]);
    } else if (shift) {
      // If the current color is not the same as the target color perform the following operations
      if (currentColor.red != targetColor.red ||
          currentColor.green != targetColor.green ||
          currentColor.blue != targetColor.blue) {
            // Compare the current red color with the target red.  If it is greater than the target, decrement by one
            // if it is less than the target, increment by one, otherwise it stays the same.
            currentColor.red   += (currentColor.red < targetColor.red) ? 1 :
                                  (currentColor.red > targetColor.red) ? -1 : 0;
            
            // Compare the current green color with the target green.  If it is greater than the target, decrement by one
            // if it is less than the target, increment by one, otherwise it stays the same.
            currentColor.green += (currentColor.green < targetColor.green) ? 1 :
                                  (currentColor.green > targetColor.green) ? -1 : 0;
            
            // Compare the current blue color with the target blue.  If it is greater than the target, decrement by one
            // if it is less than the target, increment by one, otherwise it stays the same.
            currentColor.blue  += (currentColor.blue < targetColor.blue) ? 1 :
                                  (currentColor.blue > targetColor.blue) ? -1 : 0;
          } else {
            // At this point the current color has become the target color.  Now the target color needs
            // to be set to the next color in the shiftColors array.  If we have reached the NULL terminator
            // loop back to the beginning of the array and start again.
            shiftIndex = (shiftIndex <= shiftCount) ? (shiftIndex + 1) : 0;
            if (!timer) {
              timer = millis();
            } else if ((millis()-timer) >= 60000UL) {
              setTargetColor(shiftColors[shiftIndex]);
              timer = NULL;
            }
          }
    }
  } else {
    // Turn the LED off
    writeColors((Color){0,0,0});
  }
}

void changeLight() {
  if (!shift) {
    if (party) {
      // I don't want the light to fade on spectrum mode so I don't include the brightness here
      writeColors(currentColor);
    
      // Set the current color to the next color in the spectrum array
      currentColor = targetColor;

    } else {
      // Make the fading effect
      // This will cause the light to glow on and off
      if (pulse) {
        brightness += increment;
        increment = (brightness <= 0 || brightness >= 255) ? -increment : increment;
        fade(brightness, currentColor);
      } else {
        fade(255, currentColor);
      }
    }
    delay(lightDelay);
  } else {
    writeColors(currentColor);
    
    // THIS DELAY IS IMPORTANT!!!  Without it the colors will abruptly change from one to
    // another.  Chaning this number will determine how quickly the colors fade to each other.
    delay(7);
  }
}

int getCount(char *arr[]) {
  int x = 0;
  char * currentString;
  while ((currentString = *(++arr)) != NULL) {
    x++;
  }
  return x-1;
}

unsigned long getValFromString(String str) {
  String temp = "0x" + str;
  char hexBuf[5];
  temp.toCharArray(hexBuf, 5);
  return strtoul(hexBuf, 0, 16);
}

void writeColors(struct Color colors) {
  analogWrite(RedPin, colors.red);
  analogWrite(GreenPin, colors.green);
  analogWrite(BluePin, colors.blue);
}

void fadeIn() {
  if (!running) {
    writeColors((Color){0,0,0});
    int _brightness;
    for (_brightness = 0; _brightness != 255; _brightness++) {
      fade(_brightness, currentColor);
      delay(5);
    }
    brightness = _brightness;
    increment = -1;
  }
}

void fadeOut() {
  int _brightness;
  for (_brightness = brightness; _brightness != 0; _brightness--) {
    fade(_brightness, currentColor);
    delay(5);
  }
  brightness = _brightness;
  increment = 1;
}

void fade(int level, Color color) {
  int rBright = constrain(level, 0, color.red);
  int gBright = constrain(level, 0, color.green);
  int bBright = constrain(level, 0, color.blue);
  writeColors((Color){rBright, gBright, bBright});
}

void setCurrentColor(String colorCode) {
  memset(&currentColor, 0, sizeof(currentColor));
  currentColor.red   = getValFromString(colorCode.substring(0,2));
  currentColor.green = getValFromString(colorCode.substring(2,4));
  currentColor.blue  = getValFromString(colorCode.substring(4,6));
}

void setTargetColor(String colorCode) {
  memset(&targetColor, 0, sizeof(targetColor));
  targetColor.red   = getValFromString(colorCode.substring(0,2));
  targetColor.green = getValFromString(colorCode.substring(2,4));
  targetColor.blue  = getValFromString(colorCode.substring(4,6));
}
