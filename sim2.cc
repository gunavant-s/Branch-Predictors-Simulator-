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

unsigned long int extract_index(uint32_t value, int m2) {
    int startBit = 2;
    int endBit = m2 + 1;
    //a mask to isolate the bits from startBit to endBit
    uint32_t mask = (1U << (m2)) - 1; // Mask for (endBit - startBit + 1) bits
    return (value >> startBit) & mask; // Shift right by startBit and apply the mask
}

struct blocks{
    int count;
    unsigned long int index;
};

//int upper_bits_addr = PC >> mn_diff;

class gshare{
    public:
        int n;
        int prediction_count;
        int misprediction_count;
        uint32_t m1;
        uint32_t PC;
        unsigned long int index;
        unsigned int mask;
        vector <blocks> prediction_table;
        unsigned long int global_branch_history;
        gshare(uint32_t m1,int n){
            misprediction_count = 0;
            prediction_count = 0;
            mask = (1U << n) - 1;
            global_branch_history = 0;
            global_branch_history &= mask;
            this->m1 = m1;
            this->n = n;
            prediction_table.resize(static_cast<uint32_t>(pow(2,m1)));
            for(unsigned long int i = 0;i<(pow(2,m1));i++){ //each vector id
                prediction_table[i].count = 2; // (“weakly taken”) 
                prediction_table[i].index = i; // 0 to 2^m1
            }
        }

        void update(uint32_t PC, char outcome){
            uint32_t lower_bits_PC = extract_index(PC,m1-n);//
            uint32_t needed_bits   = extract_index(PC,m1);
            uint32_t upper_bits    = (needed_bits >> (m1-n))& ((1<<n) -1);
            // PC = PC >> 2;
            // uint32_t upper_bits_PC = (PC >> (m1 - n)) & (1<<n -1);
            // printf("%d\n",index);
            index = (((global_branch_history ^ upper_bits) << (m1 - n)) | (lower_bits_PC));

            // printf("upper : %x,lower : %x  outcome: %c\n",upper_bits, lower_bits_PC,outcome);
            // printf("%d\t    %d\n",index,prediction_table[index].count);
            // index = (index << (m1-n))|lower_bits_PC;
            // printf("%d",global_branch_history);
            int msb_of_gbh;
            char prediction = (prediction_table[index].count >= 2)?'t':'n'; // infinite loop here
            // printf("PC: %x, index: %d\n", PC, index);
            bool misprediction = (prediction!= outcome);
            msb_of_gbh = (outcome == 't')?1:0;
            if(misprediction){
                misprediction_count++;
            } 
            if(prediction_table[index].count >= 2){ // predicted taken
                if(outcome == 't'){
                    prediction_table[index].count = (prediction_table[index].count == 3)? 3 : (prediction_table[index].count + 1);
                }
                else{ //mispredict
                    prediction_table[index].count = prediction_table[index].count - 1;
                }
            }
            else{ // predicted not taken
                if(outcome == 't'){ // but taken -> mispredict
                    prediction_table[index].count = prediction_table[index].count + 1;
                }
                else{ // correct prediction
                     prediction_table[index].count = (prediction_table[index].count == 0)? 0 : (prediction_table[index].count - 1);
                }
            }
            // printf("PC: %x, index: %d\n\n", PC, index);

            global_branch_history = (right_shift_gbh(global_branch_history,msb_of_gbh))&mask;
            // printf("%d\n\n",global_branch_history);
        }
        void printBranchPredictor() {
            printf("FINAL GSHARE CONTENTS \n");
            for (int i = 0;i<pow(2,m1);i++) {
                cout << prediction_table[i].index << "  " << static_cast<int>(prediction_table[i].count) << endl;
            }
        }

    private:
        unsigned int right_shift_gbh(unsigned int global_branch_history, int outcome) {
            unsigned int mask = (1U << n) - 1;
            global_branch_history = global_branch_history >> 1;
            // if(outcome == 1){
                global_branch_history = global_branch_history | (outcome << (n-1));
            // }
            return global_branch_history;
        }

};

class bimodal{ //n =0
    public:
        int prediction_count;
        int misprediction_count;
        uint32_t m2;
        uint32_t PC;
        unsigned long int index;
        vector <blocks> prediction_table;
        // uint16_
        bimodal(uint32_t m2){
            this->m2 = m2;
            misprediction_count = 0;
            prediction_count = 0;
            prediction_table.resize(static_cast<uint32_t>(pow(2,m2)));
            for(unsigned long int i = 0;i<(pow(2,m2));i++){ //each vector id
                prediction_table[i].count = 2; // (“weakly taken”) 
                prediction_table[i].index = i; // 0 to 2^m2
            }
            // this->PC = PC;
            // If predicted taken then make it strongly taken -> increase counter
            // Else decrease counter
        }

        void update(uint32_t PC, char outcome){
            index = extract_index(PC,m2);
            char prediction = (prediction_table[index].count >= 2)?'t':'n';
            // printf("PC: %x, Index: %x, index: %d\r\n", PC, index, index);
            bool misprediction = (prediction!= outcome);
            // printf("%x %u %u\n",PC,index,prediction_table[index].count);
            if(misprediction){
                misprediction_count++;
            } 
            if(prediction_table[index].count >= 2){ // predicted taken
                if(outcome == 't'){
                    prediction_table[index].count = (prediction_table[index].count == 3)? 3 : (prediction_table[index].count + 1);
                }
                else{ //mispredict
                    prediction_table[index].count = prediction_table[index].count - 1;
                }
            }
            else{ // predicted not taken
                if(outcome == 't'){ // but taken -> mispredict
                    prediction_table[index].count = prediction_table[index].count + 1;
                }
                else{ // correct prediction
                     prediction_table[index].count = (prediction_table[index].count == 0)? 0 : (prediction_table[index].count - 1);
                }
            }
            // printf("%x %u %u\n",PC,index,prediction_table[index].count);
        }

        void printBranchPredictor() { //chatgpt
            printf("FINAL BIMODEL CONTENTS\n");
            for (int i = 0;i<pow(2,m2);i++) {
                cout << prediction_table[i].index << "  " << static_cast<int>(prediction_table[i].count) << endl;
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
    bimodal* sim1 = nullptr;
    gshare* sim2  = nullptr;
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
        // bimodal sim(params.M2);
        sim1 = new bimodal(params.M2);
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
        sim2             = new gshare(params.M1, params.N);
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
    
    // int trace = 0;
    while(fscanf(FP, "%lx %s", &addr, str) != EOF)
    {
        // trace++;
        outcome = str[0];
        if(sim1!= nullptr){
            sim1->update(addr,outcome);
            sim1->prediction_count++;
        }
        else if(sim2!= nullptr){
            sim2->update(addr,outcome);
            sim2->prediction_count++;
        }

        
        // if (outcome == 't')
        //     printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
        // // t -> taken   :   n -> nottaken
        // else if (outcome == 'n')
        //     printf("%lx %s\n", addr, "n");          // Print and test if file is read correctly
        /*************************************
            Add branch predictor code here
        **************************************/
    }
    
    if(sim1!= nullptr){
        printf("number of predictions:    %d\n",sim1->prediction_count);
        printf("number of mispredictions: %d\n",sim1->misprediction_count);
        double mispredictionRate = static_cast<double>(sim1->misprediction_count) / sim1->prediction_count * 100;
        // cout << fixed << setprecision(2); // Set precision for percentage
        printf("Misprediction rate:       %.2f%%\n", mispredictionRate);
        sim1->printBranchPredictor();
    }
    else if(sim2!=nullptr){
        printf("number of predictions:    %d\n",sim2->prediction_count);
        printf("number of mispredictions: %d\n",sim2->misprediction_count);
        double mispredictionRate = static_cast<double>(sim2->misprediction_count) / sim2->prediction_count * 100;
        // cout << fixed << setprecision(2); // Set precision for percentage
        printf("Misprediction rate:       %.2f%%\n", mispredictionRate);
        sim2->printBranchPredictor();
    }
    
    return 0;
}
