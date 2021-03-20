// my_predictor.h
// This file contains a sample gshare_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.

class gshare_update : public branch_update {
public:
    unsigned int index;
};

class gshare_predictor : public branch_predictor {
public:

#define HISTORY_LENGTH    15
#define TABLE_BITS    15

    gshare_update u;
    branch_info bi;
    unsigned int history;
    unsigned char tab[1<<TABLE_BITS];

    gshare_predictor (void) : history(0) {
        memset (tab, 0, sizeof(tab));
    }

    branch_update *predict (branch_info & b) {
        bi = b;
        if (b.br_flags & BR_CONDITIONAL) {

            // (history) XOR (PC address)
            u.index = (history << (TABLE_BITS - HISTORY_LENGTH)) ^ (b.address & ((1<<TABLE_BITS)-1));
            u.direction_prediction (tab[u.index] >> 1);
        } else {
            u.direction_prediction (true);
        }
        u.target_prediction (0);
        return &u;
    }

    void update (branch_update *u, bool taken, unsigned int target) {
        if (bi.br_flags & BR_CONDITIONAL) {

            // update 2-bit counter FSM if needed
            unsigned char *c = &tab[((gshare_update*)u)->index];
            if (taken) {
                if (*c < 3) (*c)++;
            } else {
                if (*c > 0) (*c)--;
            }
            
            // update history
            history <<= 1;                        // shift table left 1
            history |= taken;                    // add 1 or 0 to history (T/NT)
            history &= (1<<HISTORY_LENGTH)-1;    // mask for only 15 bits (chops off MSB)
        }
    }
};


// Pentium M hybrid branch predictors
// This class implements a simple hybrid branch predictor based on the Pentium M branch outcome prediction units.
// Instead of implementing the complete Pentium M branch outcome predictors, the class below implements a hybrid
// predictor that combines a bimodal predictor and a global predictor.
class pm_update : public branch_update {
public:
    unsigned int bimodalIndex;
    unsigned int globalIndex;
    unsigned char globalTag;
    unsigned char globalLRUTable;
    bool globalHit = false;
};


class pm_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH         15
#define BM_TABLE_BITS          12

    pm_update u;
    branch_info bi;
    unsigned int history;
    unsigned char bimodalTable[1 << BM_TABLE_BITS];
    unsigned int globalPredictor[512][4];

    pm_predictor(void) : history(0)  {

        // fill bimodal table and global predictor with values
        memset(bimodalTable, 0, sizeof (bimodalTable));

        for (int i = 0; i < 512; i++) {
            for (int j = 0; j < 4; j++) {
                globalPredictor[i][j] = 15 - j;
            }
        }
    }

    branch_update* predict(branch_info & b) {

        bi = b;
        
        if (b.br_flags & BR_CONDITIONAL) {

            // find bimodal and global indices/tag
            unsigned int hash = ((((b.address >> 13) & ((1 << 6) - 1)) ^ (history & ((1 << 6) - 1))) << 9)
                                | (((b.address >> 4) & ((1 << 9) - 1)) ^ ((history >> 6) & ((1 << 9) - 1)));
            u.globalIndex = (hash >> 6);
            u.globalTag = (hash & ((1 << 6) - 1));

            // loop through 4 tables
            for (int i = 0; i < 4; i++) {

                // if tag matches, use 2bC for prediction
                if (u.globalTag == (globalPredictor[u.globalIndex][i] >> 4)) {
                    u.globalHit = true;
                    u.globalLRUTable = i;
                    u.direction_prediction((globalPredictor[u.globalIndex][i] >> 3) & 1);
                    u.target_prediction(0);
                    return &u;
                }
                else if ((globalPredictor[u.globalIndex][i] & ((1 << 2) - 1)) == 3) {
                    u.globalLRUTable = i;
                }
            }

            // no global predictor hit, so use bimodal for prediction
            u.bimodalIndex = (b.address & ((1 << BM_TABLE_BITS) - 1));
            u.direction_prediction(bimodalTable[u.bimodalIndex] >> 1);
        }
        else {
            u.direction_prediction(true);
        }

        u.target_prediction(0);
        return &u;
    }

    void update(branch_update *u, bool taken, unsigned int target) {

        if (bi.br_flags & BR_CONDITIONAL) {

            // update bimodal predictor if needed
            unsigned char *c = &bimodalTable[((pm_update*)u)->bimodalIndex];
            if (taken) {
                if (*c < 3) (*c)++;
            }
            else {
                if (*c > 0) (*c)--;
            }

            // tag insertion if no hit
            if (!((pm_update*)u)->globalHit) {
                int bit_mask = (63 << 4);
                globalPredictor[((pm_update*)u)->globalIndex][((pm_update*)u)->globalLRUTable]
                = ((globalPredictor[((pm_update*)u)->globalIndex][((pm_update*)u)->globalLRUTable] & (~bit_mask)) | (((pm_update*)u)->globalTag << 4));
            }

            // update LRU values
            for (int i = 0; i < 4; i++) {
                if (i == ((pm_update*)u)->globalLRUTable)
                    globalPredictor[((pm_update*)u)->globalIndex][i] = ((globalPredictor[((pm_update*)u)->globalIndex][i] >> 2) << 2);
                else {
                    int thisLRU = (globalPredictor[((pm_update*)u)->globalIndex][i] & ((1 << 2) - 1));
                    if (thisLRU < 3)
                        globalPredictor[((pm_update*)u)->globalIndex][i] += 1;
                }
            }

            // get current 2bC value
            int counterValue = ((globalPredictor[((pm_update*)u)->globalIndex][((pm_update*)u)->globalLRUTable] >> 2) & ((1 << 2) - 1));

            // update the value
            if (taken) {
                if (counterValue < 3) counterValue++;
            }
            else {
                if (counterValue > 0) counterValue--;
            }

            //place new 2bC value in table
            int bit_mask = (3 << 2);
            globalPredictor[((pm_update*)u)->globalIndex][((pm_update*)u)->globalLRUTable]
            = ((globalPredictor[((pm_update*)u)->globalIndex][((pm_update*)u)->globalLRUTable] & (~bit_mask)) | (counterValue << 2));
            
            // update GBH
            history <<= 1;                           // shift table left 1
            history |= taken;                        // add 1 or 0 to history (T/NT)
            history &= (1 << HISTORY_LENGTH) - 1;    // mask for only 15 bits (chops off MSB)
        }
    }
};


// Complete Pentium M branch predictors for extra credit
// This class implements the complete Pentium M branch prediction units.
// It implements both branch target prediction and branch outcome predicton.
class cpm_update : public branch_update {
public:
        unsigned int index;
};

class cpm_predictor : public branch_predictor {
public:
    cpm_update u;

    cpm_predictor (void) {
    }

    branch_update *predict (branch_info & b) {
        u.direction_prediction (true);
        u.target_prediction (0);
        return &u;
    }

    void update (branch_update *u, bool taken, unsigned int target) {
        
    }
};
