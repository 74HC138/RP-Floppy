//includes
#include <floppy.hpp>
#include <array>
#include <vector>

#include "datarateDetector.pio.h"
//--------------------
//defines
#define ASSERT true
#define DEASSERT false
//--------------------
//enums and structures
enum FloppySignals {
    FD_DensitySelect = 0,
    FD_MotorEnA,
    FD_DriveSelB,
    FD_DriveSelA,
    FD_MotorEnB,
    FD_Dir,
    FD_Step,
    FD_WriteData,
    FD_WriteGate,
    FD_HeadSelect,
    FD_Index,
    FD_Track0,
    FD_WriteProtect,
    FD_ReadData,
    FD_DiskChange,

    FD_MAX = FD_DiskChange
};
struct PinSelect {
    int pinIndex;
    int mode;
    bool activeLow;
};
//--------------------
//constants
const PinSelect FDPins[] = {
    {0, OUTPUT, false}, //Density select
    {1, OUTPUT, false}, //Motor enable A
    {2, OUTPUT, false}, //Drive select B
    {3, OUTPUT, false}, //Drive select A
    {4, OUTPUT, false}, //Motor enable B
    {5, OUTPUT, false}, //Step direction
    {6, OUTPUT, false}, //Head step
    {7, OUTPUT, false}, //Write data
    {8, OUTPUT, false}, //Write gate
    {9, OUTPUT, false}, //Head select

    {10, INPUT, true},  //Index pulse
    {11, INPUT, true}, //Track 00 marker
    {12, INPUT, true}, //Write protect
    {13, INPUT, true}, //Read data
    {14, INPUT, true}  //Disk change or present
};
const int trackCount = 80; //number of tracks of the drive
const int seekSpeed = 10; //4ms between steps
//--------------------
//variables
int driveIndex = 2; //drive select to use
std::array<int, 2> headPosition = {0};
//--------------------
//functions

//sets floppy interface pin
//returns 0 on success
int setPin(int fdSignal, bool state) {
    if (fdSignal >= 0 && fdSignal <= FD_HeadSelect) {
        //this is actually a valid output pin index
        if (FDPins[fdSignal].activeLow == true) state = !state; //if active low invert
        digitalWrite(FDPins[fdSignal].pinIndex, state == false ? LOW : HIGH);
        return 0;
    } else return -1;
}
//gets state of floppy interface pin
//returns pin state or false on fail
bool getPin(int fdSignal) {
    if (fdSignal >= 0 && fdSignal <= FD_MAX) {
        //this is actually a valid pin to read
        bool state = digitalRead(FDPins[fdSignal].pinIndex);
        if (FDPins[fdSignal].activeLow == true) state = !state; //if active low invert
        return state;
    } else return false;
}
//selects drive
//returns 0 on success
int selectDrive(int drive) {
    setPin(FD_DriveSelA, DEASSERT);
    setPin(FD_DriveSelB, DEASSERT);

    switch (drive) {
        case 0:
            //select no drive
            return 0;
        case 1:
            //select drive 1 (A)
            setPin(FD_DriveSelA, ASSERT);
            delay(10); //10ms delay for drive electronics to get selected
            return 0;
        case 2:
            //select drive 2 (B)
            setPin(FD_DriveSelB, ASSERT);
            delay(10); //10ms delay for drive electronics to get selected
            return 0;
        default:
            //drive out of range
            return -1;
    }
}
//starts the motor of the drive
//returns 0 on success
int startMotor(int drive) {
    switch (drive) {
        case 0:
            //no drive to start motor
            return 0;
        case 1:
            //start motor of drive 1 (A)
            setPin(FD_MotorEnA, ASSERT);
            delay(100); //100ms delay for motor to spin up
            return 0;
        case 2:
            //start motor of drive 2 (B)
            setPin(FD_MotorEnB, ASSERT);
            delay(100); //100ms delay for motor to spin up
            return 0;
        default:
            //drive out of range
            return -1;
    }
}
//stops the motor of the drive
//returns 0 on success
int stopMotor(int drive) {
    switch (drive) {
        case 0:
            //stop all motors
            setPin(FD_MotorEnA, DEASSERT);
            setPin(FD_MotorEnB, DEASSERT);
            delay(100); //100ms delay for motor to spin down
            return 0;
        case 1:
            //stop motor of drive 1 (A)
            setPin(FD_MotorEnA, DEASSERT);
            delay(100); //100ms delay for motor to spin down
            return 0;
        case 2:
            //stop motor of drive 2 (B)
            setPin(FD_MotorEnB, DEASSERT);
            delay(100); //100ms delay for motor to spin down
            return 0;
        default:
            //drive out of range
            return -1;
    }
}
//initializes the state of the controll pins
void initState() {
    for (auto pin : FDPins) {
        if (pin.mode == INPUT) {
            pinMode(pin.pinIndex, INPUT);
        } else {
            pinMode(pin.pinIndex, OUTPUT);
            digitalWrite(pin.pinIndex, pin.activeLow == false ? LOW : HIGH);
        }
    }
}
//step in torward 0 track by 1
void stepIn() {
    Serial.printf("stepping in\n");
    setPin(FD_Step, DEASSERT);
    delayMicroseconds(250);
    setPin(FD_Dir, DEASSERT);
    delayMicroseconds(250);
    setPin(FD_Step, ASSERT);
    delayMicroseconds((seekSpeed * 1000) / 2);
    setPin(FD_Step, DEASSERT);
    delayMicroseconds(((seekSpeed * 1000) / 2) - 500);
}
//step out away from 0 track by 1
void stepOut() {
    Serial.printf("stepping out\n");
    setPin(FD_Step, DEASSERT);
    delay(1);
    setPin(FD_Dir, ASSERT);
    delay(1);
    setPin(FD_Step, ASSERT);
    delay(seekSpeed / 2);
    setPin(FD_Step, DEASSERT);
    delay((seekSpeed / 2) - 2);
}
//step head to specific track
int stepTo(int targetTrack, int driveIndex) {
    int index = driveIndex - 1;
    if (index < 0 || index >= headPosition.size()) return -1; //if drive is out of range
    if (targetTrack < 0 || targetTrack >= trackCount) return -1; //if target track is out of range
    selectDrive(driveIndex);

    Serial.printf("stepping from %d to %d\n", headPosition[index], targetTrack);
    while (headPosition[index] != targetTrack) {
        if (headPosition[index] > targetTrack) {
            //step inward
            stepIn();
            headPosition[index]--;
        } else {
            //step outward
            stepOut();
            headPosition[index]++;
        }
    }
    return 0;
}
//returns head to track 0
//return 0 success
int seek0(int driveIndex) {
    int index = driveIndex - 1;
    if (index < 0 || index >= headPosition.size()) return -1;
    selectDrive(driveIndex);

    Serial.printf("seeking track 0...\n");
    for (int timeOut = 120; timeOut > 0; timeOut--) {
        Serial.printf("%d: ", timeOut);
        if (getPin(FD_Track0)) {
            Serial.printf("reached track 0\n");
            headPosition[index] = 0;
            return 0;
        }
        stepIn();
    }
    headPosition[index] = -1;
    return -1;
}
//gets the data rate of the floppy
//configures timer to count sys clocks till rising edge of data pin
//then store data in statistics table and figure out what the base clock is
//the base clock is the first significant spike with a multiple on a spike
int getDatarate(int driveIndex) {
    //collect timing statistics
    std::array<int, 1024> statisticBuffer;
    for (int i = 0; i < statisticBuffer.size(); i++) statisticBuffer[i] = 0;
    datarateDetector_program_init(pio0, FDPins[FD_ReadData].pinIndex);
    uint64_t timeout = time_us_64() + (1000 * 1000); //1 second timeout
    long counter = 0;
    long overFlow = 0;
    while (true) {
        uint32_t val;
        if (datarateDetector_pull(pio0, &val, timeout)) break;
        counter++;
        if (val >= 0 && val < statisticBuffer.size()) {
            statisticBuffer[val]++;
        } else if (val >= statisticBuffer.size()) {
            overFlow++;
        }
    }
    datarateDetector_program_exit(pio0);
    Serial.printf("got %ld samples with %ld overflows\n", counter, overFlow);
    Serial.printf("CSV Data:\n");
    for (int value : statisticBuffer) {
        Serial.printf("%d,", value);
    }
    Serial.printf("\nCSV over.\n");
    //calculate average and threshhold
    long average = 0;
    for (int value : statisticBuffer) average += value;
    average /= statisticBuffer.size();
    int threshold = (float) average * sqrtf(2);
    Serial.printf("timing avg %ld, threshold is %d\n", average, threshold);
    //get timing areas
    struct Area {
        int start;
        int end;
        int center;
    };
    std::vector<Area> timingAreas;
    bool inArea = false;
    for (int i = 1; i < (statisticBuffer.size() - 1); i++) {
        int prevSample = statisticBuffer[i-1];
        int sample = statisticBuffer[i];
        int nextSample = statisticBuffer[i+1];

        if (sample >= threshold || (prevSample >= threshold && nextSample >= threshold)) {
            //if the sample exceeds the threshold or if the sample before and after it does then this is a timing area
            if (!inArea) {
                timingAreas.push_back({i, -1, -1});
                inArea = true;
            }
        } else if (inArea) {
            Area lastArea = timingAreas.back();
            lastArea.end = i - 1;
            lastArea.center = ((lastArea.end - lastArea.start) / 2) + lastArea.start;
            timingAreas.back() = lastArea;
            inArea = false;
        }
    }
    if (inArea) Serial.printf("statistics data ended to soon\n");
    Serial.printf("got %d areas\n", timingAreas.size());
    for (Area area : timingAreas) {
        int dataRate = SYS_CLK_HZ / (area.center * 2);
        Serial.printf("start: %d, end: %d, center: %d, rate: %d\n", area.start, area.end, area.center, dataRate);
    }



    return 0;
}


int initFloppy() {
    initState();
    if (selectDrive(driveIndex) != 0) return -1;
    if (startMotor(driveIndex) != 0) return -1;
    if (seek0(driveIndex) != 0) return -1;
    delay(1000);
    stepTo(trackCount - 1, driveIndex);
    stepTo(0, driveIndex);

    getDatarate(driveIndex);

    stopMotor(0);
    selectDrive(0);

    return 0;
}