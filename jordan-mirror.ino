// This #include statement was automatically added by the Particle IDE.
#include <NCD2Relay.h>

#include "softap_http.h"

//Set Photon antenna connection to external antenna.  Not absolutely required in most instances.
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

//Do not attempt to connect to WiFi AP or Particle cloud on boot.
SYSTEM_MODE(MANUAL);

//Global variables for flasher timer.  Default will be 30 seconds or 30,000 milliseconds.
unsigned long tDuration = 30000;
unsigned long tActivated;

//Input timeout
unsigned long inputTripTimeout = 15000;
unsigned long lastInputTriggerTime;
bool triggered = false;

//Object reference to relay board.
NCD2Relay relay;

//Local functions.
void command(int id);
void tConfig(int len);

//Variables used for reading on board inputs.
bool tripped[6];

int debugTrips[6];

int minTrips = 5;

//Soft AP variables.
struct Page
{
	const char* url;
	const char* mime_type;
	const char* data;
};

const char index_html[] = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Flasher Duration</title><script type='text/javascript' src='scripts/values.js'></script></head><body><form><label>Please Enter Flasher Duration(Minimum 10, Maximum 120)</label><input type='numeric' name='duration' id='duration' value='' /><input type='submit' value='Submit' /></form><script type='text/javascript'>(function () {'use strict';document.getElementByID('duration').value=duration;})();</script></body></html>";

Page myPages[] = {
		{ "/index", "text/html", index_html},
		{ nullptr }
};
void myPage(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved);
STARTUP(softap_set_application_page_handler(myPage, nullptr));

void setup() {
    //Open Serial1 port for interfacing through S3B module.
    Serial1.begin(115200);
    //Initialize relay controller with status of on board address Jumpers
    relay.setAddress(0,0,0);
    //Get stored timer duration from memory
    int storedTDuration;
    EEPROM.get(0, storedTDuration);
    if(storedTDuration > 0 && storedTDuration < 121){
        tDuration = storedTDuration*1000;
    }
}

void loop() {
    //Check for command received through S3B
    if(Serial1.available() > 0){
        // delay(100);
        int data = Serial1.read();
        if(data == 1){
            command(1);
        }else{
            if(data > 10 && data < 120){
                tConfig(data);
            }
        }
    }
    
    //read on boad inputs and react to them closing.
    int status = relay.readAllInputs();
	int a = 0;
	for(int i = 1; i < 33; i*=2){
		if(status & i){
			debugTrips[a]++;
			if(debugTrips[a] >= minTrips){
				if(!tripped[a]){
					tripped[a] = true;
					int inputNumber = a+1;
					switch(inputNumber){
					    case 1 : 
					    //Input 1 tripped so send transmission to other units and execute command locally
					    //Check to see if the command to trigger has already fired, if so do not send again.
					    if(!triggered){
					        triggered = true;
					        lastInputTriggerTime = millis();
					        Serial1.write(1);
                            command(1);
                        }
                        break;
                        case 2 :
                        //Input 2 tripped so turn off relay, then go to soft AP mode for timer configuration.
                        relay.turnOffRelay(1);
                        WiFi.listen();
                        break;
                        case 3 :
                        //Input 3 tripped so turn off relay, then go into safe mode.
                        relay.turnOffRelay(1);
                        System.enterSafeMode();
                        break;
					}
				}
			}
		}else{
			debugTrips[a] = 0;
			if(tripped[a]){
				tripped[a] = false;
				int inputNumber = a+1;
				//Check to see if input 1 opened, if so clear triggered flag
				if(inputNumber == 1){
				    triggered = false;
				}
			}
		}
		a++;
	}
    
    //Check relay Timer
    if(millis() > tDuration+tActivated && tActivated != 0){
        relay.turnOffRelay(1);
        tActivated = 0;
    }
}

//Command handler
void command(int id){
    if(id == 1){
        //Start relay timer to flash light
        relay.turnOnRelay(1);
        tActivated = millis();
    }
}

//Timer config
void tConfig(int len){
    //set timer global variable(multiply by 1000 since we use millis
    tDuration = len*1000;
    //Store timer variable to memory
    EEPROM.put(0, len);
}

//Soft AP web interface.
void myPage(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved)
{
	Serial.printlnf("handling page %s", url);
	String u = String(url);
	
	int8_t idx = 0;
	for (;;idx++) {
		Page& p = myPages[idx];
		if (!p.url) {
			idx = -1;
			break;
		}
		else if (strcmp(url, p.url)==0) {
			break;
		}
	}
	Serial.println(idx);
	if(idx==-1){
		if(strstr(url,"values.js")){
			Serial.println("values.js started");
            int EEPROM_duration;
            
            EEPROM.get(0, EEPROM_duration);
			//Stored Duration
			String duration = "duration=\"";
			duration.concat(EEPROM_duration);
			duration.concat("\";");

			cb(cbArg, 0, 200, "application/javascript", nullptr);
			result->write(duration);
			Serial.println("devices.js finished");
		}
		else{
			if(strstr(url,"index")){
				//This runs on submit
				if(strstr(url,"?")){
					Serial.println("index? started");
					String parsing = String(url);
					
					int start = parsing.indexOf("duration=");
					parsing = parsing.substring(start+9);
					
					int duration = parsing.toInt();
					if(duration > 10 && duration < 121){
					    Serial.printf("Timer duration: %i\n", duration);
					    Serial1.write(duration);
					    EEPROM.put(0, duration);
					    delay(1000);
					}
					
					
					System.reset();
					
					Serial.println("sending redirect");
					Header h("Location: /complete\r\n");
					cb(cbArg, 0, 302, "text/plain", &h);
					return;
				}
				else{
					Serial.println("oops, something went wrong on index");
					Serial.println("404 error");
					cb(cbArg, 0, 404, nullptr, nullptr);
				}
			}
		}
	}else{
		Serial.println("Normal page request");
		cb(cbArg, 0, 200, myPages[idx].mime_type, nullptr);
		result->write(myPages[idx].data);
		Serial.println("result written");
	}
	return;
}
