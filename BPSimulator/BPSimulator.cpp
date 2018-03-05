/*
Group 2 - Branch Predictor Simulator
*/

#include "stdafx.h"
#include <stdio.h>
#include <Winsock2.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <random>
#include <list>
#include <vector>
#include <exception>

using namespace std;

// Prototypes **************************************************************
class BPSimulator {
protected:
    uint32_t numBranches;
    uint32_t numMispredict;
    uint32_t memLimit;
    uint32_t histRegister;
    vector<uint8_t> histArray;

    void predictHandler();
    bool makePrediction(uint32_t pc) { return true; }
public:
    BPSimulator(uint32_t memLimit);
    uint32_t getMispredictRate();
};

class Gshare : public BPSimulator {
private:
    bool makePrediction(uint32_t pc);
public:
    Gshare(uint32_t memLimit);
};

class Bimodal : public BPSimulator {
private:
    bool makePrediction(uint32_t pc);
public:
    Bimodal();
};

class Tournament : public BPSimulator {
private:
    bool makePrediction(uint32_t pc);
public:
    Tournament();
};

bool readTrace(FILE* stream, uint32_t* pc, bool* outcome);

// Definitions *************************************************************
const char* FILE_NAME = "C:\\Users\\Brently\\OneDrive\\Programming\\Workspace\\BPSimulator\\Traces\\test.txt";

// BPSimulator -------------------------------------------------------------
BPSimulator::BPSimulator(uint32_t memLimit) : histArray(memLimit, 0) {
    this->memLimit = memLimit;
    numBranches = 0;
    numMispredict = 0;
    histRegister = 0;
}

// Trains the histRegister and builds the histArray.
void BPSimulator::predictHandler() {
    uint32_t pc = 0;
    uint32_t numInstructions = 0;
    bool outcome = false;
    FILE* stream;

    // Open trace file.
    try {
        fopen_s(&stream, FILE_NAME, "r");
    }
    catch (exception e) {
        cout << "ERROR: Cannot open " << FILE_NAME << endl;
    }

    // Get number of instructions from trace file.
    if (fread(&numInstructions, sizeof(uint32_t), 1, stream) != 1) {
        printf("Could not read input file\n");
        return;
    }
    numInstructions = ntohl(numInstructions);
    // Read trace file and compare outcomes with predictions.
    while (readTrace(stream, &pc, &outcome)) {
        pc = ntohl(pc);
        numBranches++;

        if (makePrediction(pc) != outcome)
            numMispredict++;

        // Train the predictor.
    }
}


// Return misprediction rate.
uint32_t BPSimulator::getMispredictRate() {
    return numMispredict / numBranches;
}

// Gshare ------------------------------------------------------------------
Gshare::Gshare(uint32_t memLimit) : BPSimulator(memLimit) {
    predictHandler();
}

// Return a branch prediction.
bool Gshare::makePrediction(uint32_t pc) {
    return true;
}

// Bimodal -----------------------------------------------------------------
Bimodal::Bimodal() : BPSimulator(memLimit) {
    predictHandler();
}


// Return a branch prediction.
bool Bimodal::makePrediction(uint32_t pc) {
    return true;
}

// Tournament --------------------------------------------------------------
Tournament::Tournament() : BPSimulator(memLimit) {
    predictHandler();
}

// Return a branch prediction.
bool Tournament::makePrediction(uint32_t pc) {
    return true;
}


// General scope definitions ------------------------------------------------
bool readTrace(FILE* stream, uint32_t* pc, bool* outcome)
{
    // Read PC.
    if (fread(pc, sizeof(uint32_t), 1, stream) != 1)
        return false;

    // Read branch instruction.
    uint32_t outcome_int;
    if (fread(&outcome_int, sizeof(uint32_t), 1, stream) != 1)
        return false;
    if (outcome_int == 0)
        *outcome = false;
    else
        *outcome = true;

    return true;
}



// Main ********************************************************************
int main()
{
    uint32_t memLimit = 32;

    Gshare gshare(memLimit);
    return 0;
}

