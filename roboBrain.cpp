#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include "HeaderSimRobo.h"
#include "robot.h"
#include "roboBrain.h"
#include "Simulator.h" //quick-and-dirty solution to let the cheat compass take information from Simulator class

using namespace std;



roboBrain::roboBrain(double h, double e, double n, Interface& Linterface):
Controller(Linterface), heading(h), pos(e, n),headingChange(0),desiredHeading(0),
partCount(0), charsReceived(0), sentenceStart(false), wheelCount(0), bufferSpot(0)
{ }

const waypoint roboBrain::waypoints[] = {
		{   0.00,   0.00},
		{- 26.42,  21.83},
		{- 19.53,  30.55},
		{   0.29,  14.32},
		{  11.72,  28.72},
		{  23.83,  19.39},
		{   9.70,   2.77},
		{   6.24,   5.57},
		{   3.36,   2.49},
		{   6.91,-  0.11},
		{   3.93,-  3.28},
};
const int roboBrain::wpcount=sizeof(roboBrain::waypoints)/sizeof(waypoint);

void roboBrain::guide(){
	if(nowpoint == 0){
			fillBuffer();
			if(interface.button()){
				nowpoint = 1;
				setOffSet();
			}
	} else {
		const int wpcount = sizeof(waypoints)/sizeof(waypoint);
		if(dot((waypoints[nowpoint]- waypoints[nowpoint - 1]),waypoints[nowpoint] - pos) < 0){
			nowpoint += 1;
		}
		if(nowpoint >= wpcount){
			headingChange=400;
					return;
		}
		desiredHeading = static_cast<waypoint>(waypoints[nowpoint]-pos).heading();

		headingChange = desiredHeading - heading;
		if(headingChange > 180){
			headingChange -= 360;
		}
		else if (headingChange < -180){
			headingChange += 360;
		}
	}
}

void roboBrain::control(){
	if(nowpoint == 0) return;
	else {
		if(headingChange >= 300){
			interface.throttle.write(150);
			interface.steering.write(150);
			return;
		}
		interface.throttle.write(140);
		interface.steering.write(headingChange * double (50)/180+150);
	}
}

void roboBrain::setOffSet(){
	for(int i = 0; i < bufferMax; i++){
		if(i > bufferDiscard) offSet += bufferSpot;
		bufferSpot--;
		if(bufferSpot < 0) bufferSpot = bufferMax;
	}
	offSet /= (bufferMax - bufferDiscard);
}

void roboBrain::navigateCompass(){
	updateTime();
	int16_t g[3];
	interface.readGyro(g);
	zDN=g[2];
	yawRate = double(g[2] - offSet)/ 0x7FFF * 250;
	heading -= yawRate * dt;
}

void roboBrain::updateTime(){
  double oldTime = epochTime;
  epochTime = interface.time();
  dt = epochTime - oldTime;
}



void roboBrain::fillBuffer(){
	int16_t g[3];
	interface.readGyro(g);
	ofBuffer[bufferSpot] = g[2];
	bufferSpot++;
	if(bufferSpot >= 1500) bufferSpot = 0;
}

void roboBrain::navigateOdometer(){
  uint32_t oldWheelCount = wheelCount;
  interface.readOdometer(timeStamp, wheelCount, dtOdometer);
  uint32_t newWheelCount = wheelCount - oldWheelCount;
  waypoint dir={sin(heading*PI/180),cos(heading*PI/180)};
  pos+=dir*fp(wheelRadius * (PI / 2) * newWheelCount);
}

void roboBrain::navigateGPS(){
	//TO BE CLEANED ONCE COMPLETED --> start by reading in the NMEA sentence from the simulator as possible.
	//				Then look at the received data an pull the latitude, longitude, speed and heading.
	//				Use math to convert latitude and longitude to northing and easting, then place
	//				these values in the robot's data. Once I have finished the basic version, I need to 
	//				account for the .4 second lag and create projected easting, northing data from that.

//	if(interface.checkPPS() != pps){	//if PPS doesn't match, reset and prep for a new sentence
//		sentenceDone = false;
//		charsReceived = 0;
//		partCount = 0;
//		pps = interface.checkPPS();
//	}
//	if(sentenceDone) return;		//if I've taken in an entire sentence, then I don't need to do anything else here.
	while(interface.checkNavChar()){
		char ch = interface.readChar();
		if(ch == '$') sentenceStart = true;
		if(!sentenceStart) continue;

		nmeaReceived[charsReceived] = ch; //interface.readChar(); //<--Maybe decomment this later if ch stops being our character tester
		if(nmeaReceived[charsReceived])
		if(nmeaReceived[charsReceived] == ',' || nmeaReceived[charsReceived] == '*'){
			partitions[partCount] = charsReceived;
			partCount += 1;
		}
		charsReceived += 1;
		if(partCount == timeSpot + 1 && charsReceived == partitions[timeSpot] + 1 && strncmp(nmeaReceived, "$GPRMC", 6) != 0){
 			sentenceStart = false;
 			charsReceived = 0;
 			partCount = 0;
 			continue;

		}
		if(partCount == statusSpot + 1 && nmeaReceived[partitions[statusSpot] + 1] == 'V'){
		  	sentenceStart = false;	//if status is void, throw away the received data and move forward
		  	charsReceived = 0;
		  	partCount = 0;
		  	continue;
		}
		if(partCount == checksumSpot + 1 && charsReceived == partitions[checksumSpot] + 3){	//there should be just two characters received after the checksum asterisk if sentence is done
			sentenceStart = false;
  			char checksum = 0;
  			for(int i = 1; i < partitions[checksumSpot]; i++) {
  				checksum ^= nmeaReceived[i];
  			}
  			nmeaReceived[partitions[checksumSpot] + 3] = '\0';
  			if(checksum == strtol(nmeaReceived + partitions[checksumSpot] + 1, NULL, 16)){	//validate checksum
				printf("Parsing RMC data...");
				for(int i = 0; i < charsReceived; i++){
					if(nmeaReceived[i] == ',' || nmeaReceived[i] == '*')
						nmeaReceived[i] = '\0';
				}
				charsReceived = 0;
				partCount = 0;


				//Take data from the desired partitions and use atod function to translate them into numbers I can use
				double latpos = atof(nmeaReceived + partitions[latSpot] + 1);
				int degrees = floor(latpos)/100;
				double minutes = (latpos - degrees * 100);
				double latdd = degrees + minutes/60;

				if(nmeaReceived[partitions[nsSpot]+1] == 'S') latdd = -latdd;


				double longpos = atof(nmeaReceived + partitions[longSpot] + 1);
				degrees = floor(longpos)/100;
				minutes = (longpos - degrees * 100);
				double longdd = degrees + minutes/60;


				if(nmeaReceived[partitions[ewSpot]+1] == 'W') longdd = -longdd;


				heading = atof(nmeaReceived + partitions[headingSpot] + 1);
				if(lat0 > 90 && long0 > 180){
					lat0 = latdd;
					long0 = longdd;
				}
				else{
					//compare new lat, long with original, then give a new easting and northing to pos
					pos.northing() = (latdd - lat0)*re*PI/180;
					pos.easting()  = (longdd - long0)*re*cos(lat0)/180;
				}
				break;
			}
  			else{	//if checksum is invalid, throw away the sentence and keep going
  				charsReceived = 0;
  				partCount = 0;
  			}
		}
	}
}


void roboBrain::showVector() const {
	printf(",%06.2f,%06.2f,,%i,%06.2f,%06.2f,%06.2f, %07.2f\n",pos.easting(), pos.northing(), nowpoint,waypoints[nowpoint].easting(), waypoints[nowpoint].northing(),desiredHeading,headingChange);
}
