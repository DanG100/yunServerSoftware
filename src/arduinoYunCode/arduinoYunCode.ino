#include <Servo.h>
#include <string.h>

//uncomment to enable hardware serial debug messages
#define DEBUG 0 

#define DELIMS "#:!?"

#define LED_FAST_DELAY 50
#define LED_SLOW_DELAY 300
#define MOTOR_WATCHDOG_DELAY 200
#define MOTOR_IDLE 93
#define MOTOR_MAX 190
#define MOTOR_MIN 0
#define L_STICK_DEADZONE 9
#define R_STICK_DEADZONE 15
#define MIN_STRING 10

#define MTR1_SIG 5  
#define MTR1_POS 6
#define MTR1_GND 7

#define MTR2_SIG 2
#define MTR2_POS 3
#define MTR2_GND 4

//uncomment next line to invert left axis
//#define INVERT_LEFT_AXIS 1

//uncomment next line to inver right axis
//#define INVERT_RIGHT_AXIS 1

char data_read[32];
int ledDelay = LED_SLOW_DELAY;
bool ledState =true;
bool sockStat = true;
bool updateOutputs = false;
Servo mtr1, mtr2;
long ledTimeout, motorWatchdog;

byte oneVal;
byte twoVal;
byte threeVal;

void setup() {
	Serial1.begin(115200); // Set the baud.
	while (!Serial1)
	{
	}
	Serial.begin(115200);
	pinMode(MTR1_SIG, OUTPUT);
	pinMode(MTR1_POS, OUTPUT);
	pinMode(MTR1_GND, OUTPUT);

	pinMode(MTR2_SIG, OUTPUT);
	pinMode(MTR2_POS, OUTPUT);
	pinMode(MTR2_GND, OUTPUT);

	digitalWrite(MTR1_POS, HIGH);
	digitalWrite(MTR1_GND, LOW);
	digitalWrite(MTR2_POS, HIGH);
	digitalWrite(MTR2_GND, LOW);

	mtr1.attach(MTR1_SIG);
	mtr2.attach(MTR2_SIG);
	mtr1.write(MOTOR_IDLE);
	mtr2.write(MOTOR_IDLE);

	digitalWrite(13, LOW); //have LED start in off state
	ledTimeout = millis() + ledDelay;
	motorWatchdog = millis();
}

void loop() {
	if (Serial1.available())
	{
		memset(data_read, 0, 32);
		Serial1.readBytesUntil('?', data_read, 32);

		//check to weed out garbage data on start-up, if reasonable data size between delimiters then accept packet 
		if(strstr(data_read, "ROB#"))
		{
			ledDelay = LED_SLOW_DELAY;
			updateOutputs = true;
		}	
	}


	if (updateOutputs)
	{
		updateOutputs = false;
		strtok(data_read, "#");
		char *lStick = strtok(NULL, DELIMS);
		char *rStick = strtok(NULL, DELIMS);
		twoVal   = map(atoi(lStick), 0, 255, MOTOR_MIN, MOTOR_MAX);
		threeVal = map(atoi(rStick), 0, 255, MOTOR_MIN, MOTOR_MAX);

		#ifdef DEBUG
			Serial.print(twoVal);
			Serial.print(" ");
			Serial.println(threeVal);
		#endif


		int lftMtr = MOTOR_IDLE;
		int rghtMtr = MOTOR_IDLE;
		
		if(twoVal > MOTOR_IDLE + L_STICK_DEADZONE || twoVal < MOTOR_IDLE - L_STICK_DEADZONE)
		{
			#ifdef INVERT_LEFT_AXIS
			  rghtMtr = map(twoVal,0,190,190,0); //right motor is inverted
			  lftMtr = twoVal;		
			#else
			  rghtMtr = map(twoVal,0,190,190,0); //left motor is inverted
			  lftMtr = twoVal;
			#endif
		}
		if(threeVal > MOTOR_IDLE + R_STICK_DEADZONE || threeVal < MOTOR_IDLE - R_STICK_DEADZONE)
		{
			#ifdef INVERT_RIGHT_AXIS
			  rghtMtr -= threeVal - MOTOR_IDLE;
			  lftMtr -= threeVal - MOTOR_IDLE;
			#else
			  lftMtr += threeVal - MOTOR_IDLE;
			  rghtMtr += threeVal - MOTOR_IDLE;
			#endif
			}

		mtr1.write(lftMtr);
		mtr2.write(rghtMtr);
		motorWatchdog = millis();
	}

	if (motorWatchdog + MOTOR_WATCHDOG_DELAY < millis())
	{
		mtr1.write(MOTOR_IDLE);
		mtr2.write(MOTOR_IDLE);
		motorWatchdog = millis();
		ledTimeout = LED_FAST_DELAY;
		#ifdef DEBUG
		Serial.println("watchdog not feed");
		#endif
	}

	if (ledTimeout < millis() && sockStat)
	{
		ledTimeout = millis() + ledDelay;
		if (ledState)
		{
			digitalWrite(13, LOW);
			ledState = false;
		}
		else
		{
			digitalWrite(13, HIGH);
			ledState = true;
		}
	}
}
