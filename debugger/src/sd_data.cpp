/*This file presents all the neccessary logic to create a data packet for efficient data transmission */

#include "sd_data.h"

const char* DIR_LOADCELL = "/LoadCell";
const char* DIR_SHT      = "/SHT45";
const char* DIR_AUDIO = "/ICS-43434";

bool Init_SD() {
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD Init Failed!");
        return false;
    }
    
    if (!SD.exists(DIR_LOADCELL) SD.mkdir(DIR_LOADCELL);
    if (!SD.exists(DIR_SHT) SD.mkdir(DIR_SHT);
    if (!SD.exists(DIR_AUDIO) SD.mkdir(DIR_AUDIO);
    
    String lcPath = String(DIR_LOADCELL) + "/data.csv";
    if (!SD.exists(lcPath)) {
        File f = SD.open(lcPath, FILE_WRITE);
        if (f) {
            f.println("Timestamp,Load_Value"); 
            f.close();
            Serial.println("Created LC file");
        }
    }

    String shtPath = String(DIR_SHT) + "/data.csv";
    if (!SD.exists(shtPath)) {
        File f = SD.open(shtPath, FILE_WRITE);
        if (f) {
            f.println("Timestamp,Temperature,Humidity"); 
            f.close();
            Serial.println("Created SHT file");
        }
    }

    String audioPath = String(DIR_AUDIO) + "/data.wav";
    if(!SD.exists(audioPath)){
        File f = SD.open(audioPath,FILE_WRITE);
        if(f){
            /* figure out a way to create the data file here if you can */
    }
    return true;
}

void LoadCell_write(char* time, float load) {
    String path = String(DIR_LOADCELL) + "/data.csv";
    File dataFile = SD.open(path.c_str(), FILE_APPEND);
    if (dataFile) {
        dataFile.print(time);
        dataFile.print(",");    
        dataFile.println(load);
        dataFile.close();
        Serial.println("LC Saved.");
    } else {
        Serial.println("Error writing LC");
    }
}

void SHT_write(char* time, float temp, float humi) {
    String path = String(DIR_SHT) + "/data.csv";
    File dataFile = SD.open(path.c_str(), FILE_APPEND);
    if (dataFile) {
        dataFile.print(time);
        dataFile.print(",");
        dataFile.print(temp);
        dataFile.print(",");
        dataFile.println(humi);
        dataFile.close();
        Serial.println("SHT Saved.");
    } else {
        Serial.println("Error writing SHT");
    }
}

void Audio_write(){
    /*I want to use this function to store data into the SD card i dont want the data to be saved from the audio.cpp file*/
}

void SD_Sleep() {
    SD.end(); 
}


