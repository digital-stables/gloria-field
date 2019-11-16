
//
// This is the firmware for the Gloria Field Unit card using NFRL24 as cmmunication method
// and power comes from a solar panel and a capacitor
// there is a tank sensor a flow sensor
//
//
#include "Arduino.h"
//
// the framework's managers
//
#include <NoLCD.h>
#include <GravityTimeManager.h>
#include <NoDataStorageManager.h>
#include <DataStorageManagerInitParams.h>
#include <PowerStatusStruct.h>
#include <FlowSensorNetworkManager.h>
#include <FlowSensorStatus.h>
#include <CapacitorPowerManager.h>


//
// telepathon specific libraries
//
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//
// Initialization of the framework's objects and Managers 
//
NoLCD noLCD;
GeneralFunctions generalFunctions;
GravityTimeManager  timeManager( Serial); 
DataStorageManagerInitParams dataStorageManagerInitParams;
NoDataStorageManager dataStorageManager( dataStorageManagerInitParams, timeManager, Serial, noLCD );
SecretManager secretManager(timeManager);
CapacitorPowerManager aCapacitorPowerManager(noLCD , secretManager , dataStorageManager , timeManager,Serial);
FlowSensorNetworkManager aFlowSensorNetworkManager(Serial, aCapacitorPowerManager ,dataStorageManager, timeManager );

//
// Variable DEfinitions
//
//
// Communication
//
RF24 radio(9, 10); // CE, CSN         
const byte address[6] = "00001";     //Byte of array representing the address. This is the address where we will send the data. This should be same on the receiving side.

#define POST_TRANSMISSION_DELAY 4000
int sampleFrequencySeconds = 1;
unsigned long lastUpdate = 0;
unsigned long lastPublishedSec=0;
int publishFrequencySeconds=10;


//
// Sensors
//
// flow meters
//
#define flow_1 2
#define flow_2 3
#define NUMBER_FLOW_SENSORS 1

#define usbc  Serial

//
// Communication Method is NFRL240L
//
FlowMeterEventDataUnion aFlowMeterEventDataUnion;

boolean debug=false;
uint8_t fakeEventCounter=0;
   
//void generateEvent(){
//
//  long currentTime = timeManager.getCurrentTimeInSeconds();
//  float flowRate=5.4;
//  FlowMeterEventDataUnion aFlowMeterEventDataUnion;
//  aFlowMeterEventDataUnion.aFlowMeterEventData.reset();
//
//  fakeEventCounter++;
//  if(fakeEventCounter>=4){
//    fakeEventCounter=0;
//  }
//  //
// 
// // aFlowMeterEventDataUnion.aFlowMeterEventData.id=serialNumberHigh, serialNumberLow;
//  aFlowMeterEventDataUnion.aFlowMeterEventData.startTime = currentTime;
//  aFlowMeterEventDataUnion.aFlowMeterEventData.flowMeterId=1;
//  aFlowMeterEventDataUnion.aFlowMeterEventData.eventGroupStartTime=currentTime;
//  aFlowMeterEventDataUnion.aFlowMeterEventData.totalVolume+=1300;
////  aFlowMeterEventDataUnion.aFlowMeterEventData.highAddress=serialNumberHigh;
////   aFlowMeterEventDataUnion.aFlowMeterEventData.lowAddress= serialNumberLow;
//  aFlowMeterEventDataUnion.aFlowMeterEventData.endTime=currentTime+200;
//  aFlowMeterEventDataUnion.aFlowMeterEventData.averageflow=33.00;
//  
//  aFlowMeterEventDataUnion.aFlowMeterEventData.sampleFrequencySeconds=66;
//  const byte* eventData = reinterpret_cast<const byte*>(&aFlowMeterEventDataUnion.aFlowMeterEventData);
//  const char  *flowMeterEventUnstraferedFileName ="FMEUF";
//  const char  *EventsDirName="Events";
//  bool success=  dataStorageManager.storeEventRecord(EventsDirName,flowMeterEventUnstraferedFileName, eventData, sizeof(aFlowMeterEventDataUnion.aFlowMeterEventData) );
//  Serial.print("saved event=");
//  Serial.println(success);
//     
//}









void setup() {
  //
  // Start the managers
  //
  aCapacitorPowerManager.start();
  aFlowSensorNetworkManager.begin(NUMBER_FLOW_SENSORS,sampleFrequencySeconds, false);
  timeManager.start();
  dataStorageManager.start();


  
  //
  // Start the Communication 
  //

  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_MIN);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();  

  usbc.begin(57600);
  lastPublishedSec =  timeManager.getCurrentTimeInSeconds();
  lastUpdate=  lastPublishedSec;//timeManager.getCurrentTimeInSeconds();
  const char  *flowMeterEventUnstraferedFileName ="FMEUF";
  bool fileOk = dataStorageManager.deleteEventRecordFile(flowMeterEventUnstraferedFileName);
  if(fileOk)usbc.print("deleted event file at setup");
  dataStorageManager.storeLifeCycleEvent(timeManager.getCurrentTimeInSeconds(), PowerManager::LIFE_CYCLE_EVENT_SETUP_COMPLETED,  PowerManager::LIFE_CYCLE_EVENT_COMMA_VALUE);
}


void loop() {
  // put your main code here, to run repeatedly:
  // usbc.println("calling definestate");
  delay(1000);
  long now = timeManager.getCurrentTimeInSeconds();
  aCapacitorPowerManager.defineState();
  boolean flowActive=false;
  if(now >lastUpdate + sampleFrequencySeconds){   
      flowActive = aFlowSensorNetworkManager.updateValues();
      lastUpdate=now;
    // generateEvent();
  }
    
  if(now > (lastPublishedSec + publishFrequencySeconds)){
   
      
      if(!flowActive &&  aCapacitorPowerManager.canPublish()){
          usbc.println("about to send powerStatisiics=");
          PowerStatisticsStruct aPowerStatisticsStruct = aCapacitorPowerManager.getPowerStatisticsStruct(); 
          delay(POST_TRANSMISSION_DELAY);
          PowerStatusStruct aPowerStatusStruct = aCapacitorPowerManager.getPowerStatusStruct();        
          delay(POST_TRANSMISSION_DELAY);
         
          
          //
          // send the status of each flow meter
          //
           
          long now = timeManager.getCurrentTimeInSeconds();            
          for(int i=0;i<NUMBER_FLOW_SENSORS;i++){
            FlowSensorStatus flowSensorStatus;
            flowSensorStatus.sampleTime=now;
            flowSensorStatus.meterId=i;
            flowSensorStatus.currentFlow=aFlowSensorNetworkManager.getMeterCurrentFlow(i);
            flowSensorStatus.currentVolume=aFlowSensorNetworkManager.getMeterCurrentVolume(i);
          //if(debug)
             usbc.print("xbee is sending FlowSensorStatus=");
             usbc.println( i);
              delay(POST_TRANSMISSION_DELAY);
          }

  

            
          uint16_t index=0;
          const char  *flowMeterEventUnstraferedFileName ="FMEUF";
          const char  *EventsDirName="Events";
          int numberOfEvents = dataStorageManager.openEventRecordFile(flowMeterEventUnstraferedFileName,  sizeof(aFlowMeterEventDataUnion.aFlowMeterEventData));
          boolean keepGoing=true;
          usbc.print("numberOfEvents=");
          usbc.print(numberOfEvents);
          uint16_t counter=0;
          if(numberOfEvents>0){
              while(keepGoing){
                  keepGoing = dataStorageManager.readEventRecord(index, aFlowMeterEventDataUnion.flowMeterEventDataBytes, sizeof(aFlowMeterEventDataUnion.aFlowMeterEventData), true); 
                  if(keepGoing){
                   index++;
//                   usbc.print("xbee is sending Flowevent, keepGoing=");
//                   usbc.print(keepGoing);
//                   usbc.print(" ");
//                   usbc.println( index);
//                   usbc.print(aFlowMeterEventDataUnion.aFlowMeterEventData.startTime);
//                   usbc.print("#");
////                   usbc.print(aFlowMeterEventDataUnion.aFlowMeterEventData.endTime);
////                   usbc.print("#");
////                   usbc.print(aFlowMeterEventDataUnion.aFlowMeterEventData.eventGroupStartTime);
////                   usbc.print("#");
//                   usbc.print(aFlowMeterEventDataUnion.aFlowMeterEventData.averageflow);
//                   usbc.print("#");
//                   usbc.print(aFlowMeterEventDataUnion.aFlowMeterEventData.totalVolume);
//                   usbc.print("#");
//                   usbc.print(aFlowMeterEventDataUnion.aFlowMeterEventData.flowMeterId);
//                   usbc.print("#");
//                   usbc.print(aFlowMeterEventDataUnion.aFlowMeterEventData.sampleFrequencySeconds);
//                   usbc.print("#");
////              
//                   usbc.print(sizeof(aFlowMeterEventDataUnion.aFlowMeterEventData));
//                   usbc.println("#");
                   FlowMeterEventData aFlowMeterEventData;
                   aFlowMeterEventData.startTime=aFlowMeterEventDataUnion.aFlowMeterEventData.startTime;
                   aFlowMeterEventData.endTime=aFlowMeterEventDataUnion.aFlowMeterEventData.endTime;
                   aFlowMeterEventData.eventGroupStartTime=aFlowMeterEventDataUnion.aFlowMeterEventData.eventGroupStartTime;
                   aFlowMeterEventData.averageflow=aFlowMeterEventDataUnion.aFlowMeterEventData.averageflow;
                   aFlowMeterEventData.totalVolume=aFlowMeterEventDataUnion.aFlowMeterEventData.totalVolume;
                   aFlowMeterEventData.flowMeterId=aFlowMeterEventDataUnion.aFlowMeterEventData.flowMeterId;
                   aFlowMeterEventData.sampleFrequencySeconds=aFlowMeterEventDataUnion.aFlowMeterEventData.sampleFrequencySeconds;

                 
                   counter++;
                   delay(POST_TRANSMISSION_DELAY);
                  }
                  
              }
              dataStorageManager.closeEventRecordFile(true);
              boolean deleteok = dataStorageManager.deleteEventRecordFile(flowMeterEventUnstraferedFileName);
              usbc.print("deleted event file: ");  
              usbc.print(deleteok);  
            }
            

      }
      lastPublishedSec=timeManager.getCurrentTimeInSeconds(); 
  }

//
// this is only use for development and debugging
// since the unit will nt have anything connected to 
// the usb under normal operartion

if( Serial.available() != 0) {
    //lcd.clear();

    String command = Serial.readString();
    //
    // end of teleonome specific sensors
    //
    boolean commandProcessed =  aCapacitorPowerManager.processDefaultCommands( command);
    
  
    /*
     * teleonome specific sensors
     */

    /*
     * end f teleonome specific sensors
     */
    if(!commandProcessed){
      //
      // add device specific
     if(command.startsWith("GetSensorData")){

          Serial.flush();
        delay(100);
      }else if(command.startsWith("SetSampleFrequencySeconds")){
        //
        uint16_t potSam=GeneralFunctions::getValue(command, '#', 1).toInt();
        if(potSam>1 && potSam<1000){
          sampleFrequencySeconds=potSam;
          aFlowSensorNetworkManager.setSampleFrequencySeconds(sampleFrequencySeconds);
          Serial.println(F("Ok-SetSampleFrequencySeconds"));
        }else{
          Serial.println(F("Failure-SetSampleFrequencySeconds Invalid Parameter"));
        }
        Serial.flush();

      }else if(command.startsWith("SetPublishFrequencySeconds")){
        //SetPublishFrequencySeconds#60
        uint16_t potFreq=GeneralFunctions::getValue(command, '#', 1).toInt();
        if(potFreq>1 && potFreq<1000){
          publishFrequencySeconds=potFreq;
          Serial.println(F("Ok-SetPublishFrequencySeconds"));
        }else{
          Serial.println(F("Failure-SetPublishFrequencySeconds Invalid Parameter"));
        }
        Serial.flush();

      }else if(command.startsWith(F("ReadDiscreteRecords"))){

        Serial.println("Ok-ReadDiscreteRecords");
        Serial.flush();
          
      }else if(command.startsWith("GetEventRecords")){

       //GetEventRecords#FlowMeterEventData
        int numberOfEvents ;
        boolean keepGoing=true;
        
        const char  *flowMeterEventUnstraferedFileName ="FMEUF";
        const char  *EventsDirName="Events";
        String eventType=GeneralFunctions::getValue(command, '#', 1);
        if(eventType=="FlowMeterEventData"){
            numberOfEvents = dataStorageManager.openEventRecordFile(flowMeterEventUnstraferedFileName, sizeof(aFlowMeterEventDataUnion.aFlowMeterEventData));
                      
            uint16_t index=0;
            if(numberOfEvents){
              while(keepGoing){
                keepGoing = dataStorageManager.readEventRecord(index, aFlowMeterEventDataUnion.flowMeterEventDataBytes, sizeof(aFlowMeterEventDataUnion.aFlowMeterEventData), true);
                index ++;

//                Serial.print("2-index=");
//                Serial.print(index);  
//                Serial.print(" keepGoing=");
//                Serial.println(keepGoing);  
                
                if(keepGoing){
//                   Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.startTime);
//                   Serial.print("#");
//                   Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.endTime);
//                   Serial.print("#");
//                   Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.eventGroupStartTime);
//                   Serial.print("#");
//                   Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.averageflow);
//                   Serial.print("#");
//                   Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.totalVolume);
//                   Serial.print("#");
//                   Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.flowMeterId);
//                   Serial.print("#");
//                   Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.sampleFrequencySeconds);
//                   Serial.print("#");
//                   Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.numberOfSamples);
//                   Serial.print("#");
                   //
                   // loop over the samples
                   //
//                  Serial.print("2-numberSamples=");
//                  Serial.print(numberSamples);  
//                
//                   for(int i=0;i<numberSamples;i++){
//                    Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.samples[i].sampleTime);
//                    Serial.print("#");
//                    Serial.print(aFlowMeterEventDataUnion.aFlowMeterEventData.samples[i].flow);
//                    Serial.print("#");
//                   }      
                }
              }
              dataStorageManager.closeEventRecordFile(true);
              boolean fileOk = dataStorageManager.deleteEventRecordFile(flowMeterEventUnstraferedFileName);
              
            }
        }

        
        Serial.println("Ok-ReadEventRecords");
        Serial.flush();
      }else{
        //
        // call read to flush the incoming
        //
        Serial.read();
        Serial.println("Failure-Bad Command " + command);
        Serial.flush();
      }
    }

  }
  
    // this is the end of the loop, to calculate the energy spent on this loop
    // take the time substract the time at the beginning of the loop (the now variable defined above)
    // and also substract the seconds spent in powerdownMode
    // finally add the poweredDownInLoopSeconds to the daily total

    aCapacitorPowerManager.endOfLoopProcessing();
}
