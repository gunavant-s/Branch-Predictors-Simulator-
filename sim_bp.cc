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
class bimodal{ //n =0
    public:
        int prediction_count;
        int misprediction_count;
        uint32_t m2;
        uint32_t PC;
        unsigned long int index;
        char prediction;
        bool misprediction;
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

        void update(uint32_t PC, char outcome,bool update_prediction_table = true){
            index = extract_index(PC,m2);
            prediction = (prediction_table[index].count >= 2)?'t':'n';
            // printf("PC: %x, Index: %x, index: %d\r\n", PC, index, index);
            misprediction = (prediction!= outcome);
            // printf("%x %u %u\n",PC,index,prediction_table[index].count);
            if(misprediction){
                misprediction_count++;
            } 
            if(update_prediction_table){
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
            }
            // printf("%x %u %u\n",PC,index,prediction_table[index].count);
        }
        int results_from_predictor(){
            return prediction_table[index].count;
        }

        char get_prediction(){
            return prediction;
        }

        void printBranchPredictor() { 
            printf("FINAL BIMODAL CONTENTS\n");
            for (int i = 0;i<pow(2,m2);i++) {
                cout << prediction_table[i].index << "	" << static_cast<int>(prediction_table[i].count) << endl;
            }
        }
    };

class gshare{
    public:
        int n;
        int prediction_count;
        int misprediction_count;
        uint32_t m1;
        uint32_t PC;
        unsigned long int index;
        unsigned int mask;
        char prediction;
        vector <blocks> prediction_table;
        bool misprediction;
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

        void update(uint32_t PC, char outcome, bool update_prediction_table = true, bool update_global_branch_history = true){
            uint32_t lower_bits_PC = extract_index(PC,m1-n);//
            uint32_t needed_bits   = extract_index(PC,m1);
            uint32_t upper_bits    = (needed_bits >> (m1-n))& ((1<<n) -1);
            // PC = PC >> 2;
            // uint32_t upper_bits_PC = (PC >> (m1 - n)) & (1<<n -1);
            // printf("%d\n",index);
            index = (((global_branch_history ^ upper_bits) << (m1 - n)) | (lower_bits_PC));
            // printf("%d\n",index);
            // printf("upper : %x,lower : %x  outcome: %c\n",upper_bits, lower_bits_PC,outcome);
            // printf("%d\t    %d\n",index,prediction_table[index].count);
            // index = (index << (m1-n))|lower_bits_PC;
            // printf("%d",global_branch_history);
            int msb_of_gbh;
            prediction = (prediction_table[index].count >= 2)?'t':'n'; // infinite loop here
            // printf("PC: %x, index: %d\n", PC, index);
            misprediction = (prediction!= outcome);
            msb_of_gbh = (outcome == 't')?1:0;
            if(misprediction){
                misprediction_count++;
            } 
            if(update_prediction_table){
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
            }
            // printf("PC: %x, index: %d\n\n", PC, index);
            if(update_global_branch_history){
                global_branch_history = (right_shift_gbh(global_branch_history,msb_of_gbh))&mask;
            }
            // printf("%d\n\n",global_branch_history);
        }
        void printBranchPredictor() {
            printf("FINAL GSHARE CONTENTS \n");
            for (int i = 0;i<pow(2,m1);i++) {
                cout << prediction_table[i].index << "  " << static_cast<int>(prediction_table[i].count) << endl;
            }
        }

        int results_from_predictor(){
            return prediction_table[index].count;
        }
        char get_prediction(){
            return prediction;
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

class hybrid{
    public:
        int n;
        int prediction_count;
        int misprediction_count;
        bool prediction;
        uint32_t m1;
        int count;
        int debug_index, debug_count;
        int k;
        uint32_t m2; // bimodal
        uint32_t PC;
        unsigned long int index;
        unsigned int mask;
        bool gshare_prediction_result;
        bool bimodal_prediction_result;
        bimodal* bimodel_sim  = nullptr;
        gshare*  gshare_sim  = nullptr;
        vector <blocks> chooser_counter;
        hybrid (int k, uint32_t m1, uint32_t n, uint32_t m2){
            this-> k = k;
            this-> m1 = m1;
            this-> m2 = m2;
            this-> n  = n;
            count = 0;
            prediction_count = 0;
            misprediction_count =0 ;
            prediction = false;
            mask = (1U << k) - 1;
            chooser_counter.resize(static_cast<uint32_t>(pow(2,k)));
            for(unsigned long int i = 0;i<(pow(2,k));i++){
                chooser_counter[i].count = 1;
                chooser_counter[i].index = i;
            }
            bimodel_sim = new bimodal(m2);
            gshare_sim = new gshare(m1,n);
        }
        
        void update(uint32_t PC, char outcome){
            bool gshare_misprediction, bimodal_misprediction;
            bimodel_sim->update(PC,outcome,false);
            gshare_sim->update(PC,outcome,false,false);

            bool bimodal_prediction= (bimodel_sim->prediction == outcome); // true/n
            bool gshare_prediction = (gshare_sim->prediction == outcome); // true/n
            index = extract_index(PC,k);
            // printf("%x %d\n",PC,index);
            // printf("=%d	%x %c\n",count,PC,outcome);
            // printf("	GP: %d	%d\n",gshare_sim->index, gshare_sim->results_from_predictor());
            // printf("	BP: %d	%d\n",bimodel_sim->index, bimodel_sim->results_from_predictor());
            count++;
            if(chooser_counter[index].count >= 2){ //gshare
                prediction = gshare_prediction;
                gshare_sim->update(PC,outcome,true,true);
                debug_index = gshare_sim->index;
                debug_count = gshare_sim->results_from_predictor();
            }
            else{
                prediction = bimodal_prediction;
                bimodel_sim->update(PC,outcome);
                gshare_sim->update(PC,outcome,false,true);
                debug_index = bimodel_sim->index;
                debug_count = bimodel_sim->results_from_predictor();
            }
            if(!prediction){
                misprediction_count++;
            } 


            // printf("	CP: %d  %d\n",index, chooser_counter[index].count);
            // printf("	BU: %d  %d\n",debug_index, debug_count);

            if(gshare_prediction!= bimodal_prediction){
                if(gshare_prediction){
                    chooser_counter[index].count = (chooser_counter[index].count == 3)? 3 : (chooser_counter[index].count + 1);
                    // printf("	CU: %d  %d\n",index, chooser_counter[index].count);
                }
                else{
                    chooser_counter[index].count = (chooser_counter[index].count == 0)? 0 : (chooser_counter[index].count - 1);
                    // printf("	CU: %d  %d\n",index, chooser_counter[index].count);
                }
            }
            
        } 

        void printBranchPredictor() { 
            printf("FINAL CHOOSER CONTENTS\n");
            for (int i = 0;i<pow(2,k);i++) {
                cout << chooser_counter[i].index << "  " << static_cast<int>(chooser_counter[i].count) << endl;
            }
            gshare_sim->printBranchPredictor();
            bimodel_sim->printBranchPredictor();
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
    hybrid* sim3  = nullptr;
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
        printf("COMMAND\n  %s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
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
        printf("COMMAND\n  %s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);
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
        sim3 = new hybrid(params.K,params.M1,params.N,params.M2);
        printf("COMMAND\n  %s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);

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
    while(fscanf(FP, "%lx %s", &addr, str) != EOF )//&& trace < 10000)
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
        else if(sim3!= nullptr){
            sim3->update(addr,outcome);
            sim3->prediction_count++;
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
    printf("OUTPUT\n");
    
    if(sim1!= nullptr){
        printf("number of predictions:    %d\n",sim1->prediction_count);
        printf("number of mispredictions: %d\n",sim1->misprediction_count);
        double mispredictionRate = static_cast<double>(sim1->misprediction_count) / sim1->prediction_count * 100;
        // cout << fixed << setprecision(2); // Set precision for percentage
        printf("misprediction rate:       %.2f%%\n", mispredictionRate);
        sim1->printBranchPredictor();
    }
    else if(sim2!=nullptr){
        printf("number of predictions:    %d\n",sim2->prediction_count);
        printf("number of mispredictions: %d\n",sim2->misprediction_count);
        double mispredictionRate = static_cast<double>(sim2->misprediction_count) / sim2->prediction_count * 100;
        // cout << fixed << setprecision(2); // Set precision for percentage
        printf("misprediction rate:       %.2f%%\n", mispredictionRate);
        sim2->printBranchPredictor();
    }
    else{
        printf("number of predictions:    %d\n",sim3->prediction_count);
        printf("number of mispredictions: %d\n",sim3->misprediction_count);
        double mispredictionRate = static_cast<double>(sim3->misprediction_count) / sim3->prediction_count * 100;
        // cout << fixed << setprecision(2); // Set precision for percentage
        printf("misprediction rate:       %.2f%%\n", mispredictionRate);
        sim3->printBranchPredictor();
    }
    
    return 0;
}
