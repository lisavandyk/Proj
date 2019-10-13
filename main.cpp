/**
 * @file       main.cpp
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Mar 2015
 * @brief
 */

//#define BLYNK_DEBUG
#define BLYNK_PRINT stdout
#ifdef RASPBERRY
  #include <BlynkApiWiringPi.h>
#else
  #include <BlynkApiLinux.h>
#endif
#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>

//Include RTC functions
#include <CurrentTime.h>
#include <CurrentTime.cpp>
#include <mcp3004.h>
#include <wiringPiSPI.h>


static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);

static const char *auth, *serv;
static uint16_t port;

#include <BlynkWidgets.h>

void checkADC();
void readSysTime();

int interval;

long lastInt =0;
int timerID;
int timerID1;
int timerID2;

int HOUR,MIN,SEC;	//Values for real Time Clock
double LIGHT,HUMIDITY,TEMP;
double v = 3.3/1023;	//Value that converts ADC to output voltage
int w = 1;	//Value that tracks what interval data is being smapled at

int SYSHOUR;	//Values for system timer
int SYSMIN;
int SYSSEC;

double voltage;

#define INT_BUTTON 18
#define SYSTIME_BUTTON 24

#define BASE 100
#define SPI_CHAN 0

WidgetLED led3(V3);
BlynkTimer timer;
/*
void buttonLEDWidget()
{
	if(digitalRead(buttonPin) == HIGH){
		led3.off();
	}
	else{
		led3.on();
	}
}*/
void DACOutput(){
	voltage = (LIGHT/1023)*HUMIDITY;
	printf("DAC out: %f V\n", voltage);

}
void changeInterval()
{
	long interruptTime = millis();
	if(interruptTime-lastInt>200){
		if(w==3){
			w=1;
			//interval = 1000L;
			printf("interval is 1 second\n");
    			timer.deleteTimer(timerID);
			timer.deleteTimer(timerID1);
			timer.deleteTimer(timerID2);
			timerID = timer.setInterval(1000L,checkADC);
			timerID1 = timer.setInterval(1000L,readSysTime);
			timerID2 = timer.setInterval(1000L, DACOutput);
		}
		else if(w==1){
			w=2;
			//interval =2000L;
			printf("interval is 2 second\n");
                        timer.deleteTimer(timerID);
                        timerID = timer.setInterval(2000L,checkADC);
			timer.deleteTimer(timerID1);
			timerID1 = timer.setInterval(2000L,readSysTime);
			timer.deleteTimer(timerID2);
			timerID2 = timer.setInterval(2000L, DACOutput);

		}
		else if(w==2){
			w=3;
			//interval = 5000L;
			printf("inteval is 5 second\n");
                        timer.deleteTimer(timerID);
                        timerID = timer.setInterval(5000L,checkADC);
			timer.deleteTimer(timerID1);
			timerID1 = timer.setInterval(5000L,readSysTime);
			timer.deleteTimer(timerID2);
			timerID2= timer.setInterval(5000L, DACOutput);
		}
	}

	lastInt = interruptTime;
}

void resetSysTime()
{
	long interruptTime = millis();
	if (interruptTime - lastInt >200){
		SYSHOUR=0;
		SYSMIN =0;
		SYSSEC=0;
	}
	printf("Reset time");
	lastInt = interruptTime;
}

void checkADC()
{
	LIGHT = analogRead(100);
	HUMIDITY = (analogRead(101)*v);	//ADC reading converted into voltage
	TEMP = (((analogRead(102)*v)-0.5)*100);

	Blynk.virtualWrite(V4, LIGHT);
	Blynk.virtualWrite(V2, HUMIDITY);
	Blynk.virtualWrite(V1, TEMP);

	}

void displayTime()
{
	HOUR = getHours();
	MIN = getMins();
	SEC = getSecs();

	//sprintf(currentTime, "%02d:%02d:%02d", HOUR, MIN, SEC);
	Blynk.virtualWrite(V0,HOUR,":",MIN,":",SEC);
}

void sysTime()
{
     if(SYSSEC<60)
     {
	SYSSEC+=1;
     }

     else if(SYSSEC ==60)
     {
	SYSSEC=0;
	if(SYSMIN<60){
	    SYSMIN+=1;
	}
	else if(SYSMIN == 60){
	    SYSMIN=0;
	    if(SYSHOUR<24){
		SYSHOUR +=1;
	    }
	    else if(SYSHOUR=24){
		SYSHOUR=0;
	    }
	}
     }
}

void readSysTime()
{
     Blynk.virtualWrite(V5,SYSHOUR,":",SYSMIN,":",SYSSEC);
}

void setup()
{
    SYSHOUR = 0;
    SYSMIN = 0;
    SYSSEC = 0;
    voltage =0;
    interval = 1000L;
    timer.setInterval(1000L, displayTime);
    timerID = timer.setInterval(interval,checkADC);
    timer.setInterval(1000L,sysTime);
    
    timerID1 = timer.setInterval(interval, readSysTime);
    timerID2 = timer.setInterval(interval, DACOutput);
    
    
    wiringPiSPISetup(0,500000);
    mcp3004Setup(BASE,SPI_CHAN);
    printf("Setup is complete!\n");
}

void setup_GPIO()
{
    wiringPiSetup();
    pinMode(INT_BUTTON, INPUT);
    pinMode(SYSTIME_BUTTON, INPUT);

    pullUpDnControl(INT_BUTTON,PUD_UP);
    pullUpDnControl(SYSTIME_BUTTON, PUD_UP);

    wiringPiISR(INT_BUTTON,INT_EDGE_FALLING,&changeInterval);
    wiringPiISR(SYSTIME_BUTTON, INT_EDGE_RISING,&resetSysTime);
}

void loop()
{
    Blynk.run();
    timer.run();
}

int main(int argc, char* argv[])
{
    parse_options(argc, argv, auth, serv, port);

    Blynk.begin(auth, serv, port);

    setup();
    setup_GPIO();
   

    while(true) {
        loop();
    }

    return 0;
}

