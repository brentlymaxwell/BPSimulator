/*
Group 2 - Branch Predictor Simulator
*/

#include "stdafx.h"
#include <stdio.h>
#include <Winsock2.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <exception>
#include <cmath>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Prototypes **************************************************************
class BranchPredictor {
protected:
    uint32_t memLimit;
    uint32_t predictionTableAddrBits;
    uint32_t pcMask;
    virtual void trainPredictor(uint32_t pc, bool outcome) = 0;
    virtual bool makePrediction(uint32_t pc) = 0;
public:
    BranchPredictor(uint32_t memLimit);
    void runPredictor(uint32_t memLimit, istream & trace, ostream & output);
    bool ReadTrace(istream& trace_stream, uint32_t* pc, bool* outcome);
    virtual ostream& displayResult(ostream& output, uint32_t num_branch, uint32_t num_miss_predicts);
};

class Gshare : public BranchPredictor {
private:
    uint32_t globalHistReg;
    vector<uint32_t> predictionTable;
    bool makePrediction(uint32_t pc);
    void trainPredictor(uint32_t pc, bool outcome);
public:
    Gshare(uint32_t memLimit);
};

class Bimodal : public BranchPredictor {
private:
    vector<uint32_t> predictionTable;
    bool makePrediction(uint32_t pc);
    void trainPredictor(uint32_t pc, bool outcome);
public:
    Bimodal(uint32_t memLimit);
};

class Tournament : public BranchPredictor {
private:
    uint32_t globalHistReg;
    vector<uint32_t> localHistTable;
    vector<uint32_t> localHistPredictionTable;
    vector<uint32_t> globalHistPredictionTable;
    vector<uint32_t> predictorChoiceTable;
    void trainPredictor(uint32_t pc, bool outcome);
    bool makePrediction(uint32_t pc);
public:
    Tournament(uint32_t memLimit);
};

// Definitions *************************************************************

// BranchPredictor -------------------------------------------------------------
BranchPredictor::BranchPredictor(uint32_t memLimit) {
    this->memLimit = memLimit;
    predictionTableAddrBits = (int)(log(memLimit) / log(2));
    pcMask = (1 << (int)(log(memLimit) / log(2))) - 1;
}

// Run branch predictor simulator main routine.
void BranchPredictor::runPredictor(uint32_t mem_limit, istream& trace, ostream& output) {
    // Read each branch from the trace
    uint32_t pc = 0;
    bool outcome = false;
    uint32_t numBranches = 0;
    uint32_t numMispredicts = 0;

    while (ReadTrace(trace, &pc, &outcome)) {
        pc = ntohl(pc);
        numBranches++;

        // Make a prediction and compare with actual outcome
        if (makePrediction(pc) != outcome) numMispredicts++;

        // Train the predictor
        trainPredictor(pc, outcome);
    }
    displayResult(cout, numBranches, numMispredicts);
}

// Helper functions for reading trace file.
bool BranchPredictor::ReadTrace(istream& trace_stream, uint32_t* pc, bool* outcome) {
    if (trace_stream.good()) {
        trace_stream.read((char*)pc, sizeof(uint32_t));

        char outcome_int = 0;
        if (trace_stream.good()) {
            trace_stream.read(&outcome_int, sizeof(outcome_int));
            if (outcome_int == 0) {
                *outcome = false;
            }
            else {
                *outcome = true;
            }
            return true;
        }
        return false;
    }
    return false;
}

// Print results of prediction.
ostream& BranchPredictor::displayResult(ostream& output, uint32_t numBranches,
    uint32_t numMispredicts) {
    output << "Branches in Trace: " << numBranches << endl;
    output << "Misprediction Rate: " << numMispredicts << "/" << numBranches
        << setprecision(4) << " = " << ((double)numMispredicts / (double)numBranches) * 100
        << "%" << endl;
    return output;
}

// Gshare ------------------------------------------------------------------
Gshare::Gshare(uint32_t memLimit) : BranchPredictor(memLimit) {
    this->globalHistReg = 0;
    this->predictionTable.resize(memLimit, 0);
}

// Return a branch prediction.
bool Gshare::makePrediction(uint32_t pc) {
    uint32_t index = (pc & this->pcMask) ^ (this->globalHistReg & this->pcMask);
    return this->predictionTable[index] >= 2;
}

// Train behavior of predictor.
void Gshare::trainPredictor(uint32_t pc, bool outcome) {
    uint32_t index = (pc & this->pcMask) ^ (this->globalHistReg & this->pcMask);
    if (outcome == true && this->predictionTable[index] < 3) {
        this->predictionTable[index]++;
    }
    if (outcome == false && this->predictionTable[index] > 0) {
        this->predictionTable[index]--;
    }
    this->globalHistReg = (this->globalHistReg << 1) | outcome;
}

// Bimodal -----------------------------------------------------------------
Bimodal::Bimodal(uint32_t memLimit) : BranchPredictor(memLimit) {
    this->predictionTable.resize(memLimit, 0);
}

// Return a branch prediction.
bool Bimodal::makePrediction(uint32_t pc) {
    uint32_t index = pc & this->pcMask;
    return this->predictionTable[index] >= 2;
}

// Train behavior of predictor.
void Bimodal::trainPredictor(uint32_t pc, bool outcome) {
    uint32_t index = pc & this->pcMask;
    if (outcome == true && this->predictionTable[index] < 3) {
        this->predictionTable[index]++;
    }
    if (outcome == false && this->predictionTable[index] > 0) {
        this->predictionTable[index]--;
    }
}

// Tournament --------------------------------------------------------------
Tournament::Tournament(uint32_t memLimit) : BranchPredictor(memLimit) {
    this->globalHistReg = 0;
    this->globalHistPredictionTable.resize(memLimit, 0);
    this->localHistPredictionTable.resize(memLimit, 0);
    this->localHistTable.resize(memLimit, 0);
    this->predictorChoiceTable.resize(memLimit, 2);
}

// Return a branch prediction.
bool Tournament::makePrediction(uint32_t pc) {
    uint32_t localIndex = (this->localHistTable[pc & this->pcMask] ^ pc) & this->pcMask;
    bool localPrediction = (this->localHistPredictionTable[localIndex] >= 2);
    uint32_t globalIndex = (pc & this->pcMask) ^ (this->globalHistReg & this->pcMask);
    bool globalPredicton = (this->globalHistPredictionTable[globalIndex] >= 2);
    if (this->predictorChoiceTable[pc & this->pcMask] >= 2) {
        return globalPredicton;
    }
    else {
        return localPrediction;
    }
}

// Train behavior of predictor.
void Tournament::trainPredictor(uint32_t pc, bool outcome) {
    uint32_t localIndex = (this->localHistTable[pc & this->pcMask] ^ pc) & this->pcMask;
    bool localPrediction = (this->localHistPredictionTable[localIndex] >= 2);
    uint32_t globalIndex = (pc & this->pcMask) ^ (this->globalHistReg & this->pcMask);
    bool globalPredicton = (this->globalHistPredictionTable[globalIndex] >= 2);
    uint32_t index = pc & this->pcMask;
    if (localPrediction != outcome && globalPredicton == outcome && this->predictorChoiceTable[index] < 3) {
        this->predictorChoiceTable[index]++;
    }
    if (localPrediction == outcome && globalPredicton != outcome && this->predictorChoiceTable[index] > 0) {
        this->predictorChoiceTable[index]--;
    }
    if (outcome == true && this->localHistPredictionTable[localIndex] < 3) {
        this->localHistPredictionTable[localIndex]++;
    }
    if (outcome == false && this->localHistPredictionTable[localIndex] > 0) {
        this->localHistPredictionTable[localIndex]--;
    }
    this->localHistTable[pc & this->pcMask] = (this->localHistTable[pc & this->pcMask] << 1) | outcome;
    if (outcome == true && this->globalHistPredictionTable[globalIndex] < 3) {
        this->globalHistPredictionTable[globalIndex]++;
    }
    if (outcome == false && this->globalHistPredictionTable[globalIndex] > 0) {
        this->globalHistPredictionTable[globalIndex]--;
    }
    this->globalHistReg = (this->globalHistReg << 1) | outcome;
}

// Main ********************************************************************
int main(int argc, char** argv)
{
    uint32_t memLimit = 32;
    string file_name = "D:\\Users\\Brently\\OneDrive\\Programming\\Workspace\\BPSimulator\\Traces\\gzip.trace";
    string predictor = "Gshare";

    /*
    if (argc == 3) {
        file_name = argv[2];
        predictor = argv[1];
    }
    else if (argc == 2) {
        predictor = argv[1];
    }
    else {
        cout << "Usage:" << endl;
        cout << argv[0] << " [predictor type] [trace file name]" << endl;
        return 1;
    }
    */

    istream* trace_input = nullptr;
    ifstream trace_file;
    if (!file_name.empty()) {
        // Open trace file as binary file
        trace_file.open(file_name, ios::in | ios::binary);

        uint32_t num_instructions = 0;
        if (trace_file.good()) {  // consume the number of instruction field
            trace_file.read((char*)&num_instructions, sizeof(num_instructions));
            num_instructions = ntohl(num_instructions);
        }
        else {
            cerr << "Unable to open file" << endl;
            return 1;
        }

        trace_input = &trace_file;
    }
    else {
        // read from cin
        trace_input = &cin;
    }

    BranchPredictor* bp = nullptr;

    if (predictor == "Gshare") {
        bp = new Gshare(memLimit);
    }
    else if (predictor == "Bimodel") {
        bp = new Bimodal(memLimit);
    }
    else if (predictor == "Tournament") {
        bp = new Tournament(memLimit);
    }
    else {
        cout << "No predictor " << endl;
    }

    // Used cout to print to console
    // We can use ofstream to write the result to a file, if desired.
    bp->runPredictor(memLimit, *trace_input, cout);

    delete bp;

    return 0;
}
