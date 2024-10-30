#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"
#include <iostream>
#include <cstdint> //uint32_t
#include <vector>
#include <math.h>

using namespace std;
/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim bimodal 6 gcc_trace.txt
    argc = 4
    argv[0] = "sim"
    argv[1] = "bimodal"
    argv[2] = "6"
    ... and so on
*/
unsigned long int extract_index(uint32_t value, int m) {
    int startBit = 2;
    int endBit = m + 1;
    //a mask to isolate the bits from startBit to endBit
    uint32_t mask = (1U << (m)) - 1; // Mask for (endBit - startBit + 1) bits
    return (value >> startBit) & mask; // Shift right by startBit and apply the mask
}


struct blocks{
    int count;
    unsigned long int index;
};

class bimodal{ //n =0
    public:
        int prediction_count;
        int misprediction_count;
        uint32_t m;
        uint32_t PC;
        unsigned long int index;
        vector <blocks> branch_predicter;
        // uint16_
        bimodal(uint32_t m){
            this->m = m;
            misprediction_count = 0;
            prediction_count = 0;
            branch_predicter.resize(static_cast<uint32_t>(pow(2,m)));
            for(unsigned long int i = 0;i<(pow(2,m));i++){ //each vector id
                branch_predicter[i].count = 2; // (“weakly taken”) 
                branch_predicter[i].index = i; // 0 to 2^m
            }
            // this->PC = PC;
            // If predicted taken then make it strongly taken -> increase counter
            // Else decrease counter
        }

        void update(uint32_t PC, char outcome){
            index = extract_index(PC,m);
            char prediction = (branch_predicter[index].count >= 2)?'t':'n';
            // printf("PC: %x, Index: %x, index: %d\r\n", PC, index, index);
            bool misprediction = (prediction!= outcome);
            // printf("%x %u %u\n",PC,index,branch_predicter[index].count);
            if(misprediction){
                misprediction_count++;
            } 
            if(branch_predicter[index].count >= 2){ // predicted taken
                if(outcome == 't'){
                    branch_predicter[index].count = (branch_predicter[index].count == 3)? 3 : (branch_predicter[index].count + 1);
                }
                else{ //mispredict
                    branch_predicter[index].count = branch_predicter[index].count - 1;
                }
            }
            else{ // predicted not taken
                if(outcome == 't'){ // but taken -> mispredict
                    branch_predicter[index].count = branch_predicter[index].count + 1;
                }
                else{ // correct prediction
                     branch_predicter[index].count = (branch_predicter[index].count == 0)? 0 : (branch_predicter[index].count - 1);
                }
            }
            // printf("%x %u %u\n",PC,index,branch_predicter[index].count);
        }

        void printBranchPredictor() { //chatgpt
            printf("FINAL BIMODAL CONTENTS \n");
            for (int i = 0;i<pow(2,m);i++) {
                cout << branch_predicter[i].index << "\t" << static_cast<int>(branch_predicter[i].count) << endl;
            }
        }
    };


int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file
    
    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.bp_name  = argv[1];
    // printf("%s\n",params.bp_name);
    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if(strcmp(params.bp_name, "bimodal") == 0)              // Bimodal
    {
        if(argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M2       = strtoul(argv[2], NULL, 10);
        trace_file      = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
    }
    else if(strcmp(params.bp_name, "gshare") == 0)          // Gshare
    {
        if(argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M1       = strtoul(argv[2], NULL, 10);
        params.N        = strtoul(argv[3], NULL, 10);
        trace_file      = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);
    }
    else if(strcmp(params.bp_name, "hybrid") == 0)          // Hybrid
    {
        if(argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.K        = strtoul(argv[2], NULL, 10);
        params.M1       = strtoul(argv[3], NULL, 10);
        params.N        = strtoul(argv[4], NULL, 10);
        params.M2       = strtoul(argv[5], NULL, 10);
        trace_file      = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);

    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }
    
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    char str[2];
    bimodal sim(params.M2);

    while(fscanf(FP, "%lx %s", &addr, str) != EOF)
    {
        
        outcome = str[0];
        sim.update(addr,outcome);
        sim.prediction_count++;
        
        // if (outcome == 't')
        //     printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
        // // t -> taken   :   n -> nottaken
        // else if (outcome == 'n')
        //     printf("%lx %s\n", addr, "n");          // Print and test if file is read correctly
        /*************************************
            Add branch predictor code here
        **************************************/
    }
    

    printf("number of predictions:     %d\n",sim.prediction_count);
    printf("number of mispredictions:  %d\n",sim.misprediction_count);
    double mispredictionRate = static_cast<double>(sim.misprediction_count) / sim.prediction_count * 100;
    // cout << fixed << setprecision(2); // Set precision for percentage
    cout << "Misprediction rate:       " << mispredictionRate << "%" << endl;
    sim.printBranchPredictor();
    
    return 0;
}
