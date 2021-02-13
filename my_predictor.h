// my_predictor.h
// This file contains a sample gshare_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.

class gshare_update : public branch_update {
public:
	unsigned int index;
};

class gshare_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	15
#define TABLE_BITS	15
	gshare_update u;
	branch_info bi;
	unsigned int history;
	unsigned char tab[1<<TABLE_BITS];

	gshare_predictor (void) : history(0) { 
		memset (tab, 0, sizeof (tab));
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
			history <<= 1;				// shift table left 1
			history |= taken;			// add 1 or 0 to history (T/NT)
			history &= (1<<HISTORY_LENGTH)-1;	// mask for only 15 bits (chops off MSB)
		}
	}
};

//
// Pentium M hybrid branch predictors
// This class implements a simple hybrid branch predictor based on the Pentium M branch outcome prediction units. 
// Instead of implementing the complete Pentium M branch outcome predictors, the class below implements a hybrid 
// predictor that combines a bimodal predictor and a global predictor. 
class pm_update : public branch_update {
public:
        unsigned int index;
};

class pm_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	15
#define TABLE_BITS	12

	pm_update u;
	unsigned int history;
	unsigned char bimodalTable[1<<TABLE_BITS];

        pm_predictor (void) : history(0)  {

		// fill bimodal table with zeros
		memset (bimodalTable, 0, sizeof (bimodalTable));

		// TODO: potentially fill global predictor cache as well
        }

        branch_update *predict (branch_info & b) {
			
		// predict branch outcome
            	u.direction_prediction (true);
			
		// predict branch target address
            	u.target_prediction (0);
            
		return &u;
        }

        void update (branch_update *u, bool taken, unsigned int target) {

		if (bi.br_flags & BR_CONDITIONAL) {

			// update bimodal predictor if needed
			unsigned char *c = &tab[((pm_update*)u)->index];
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}

			// update GBH
			history <<= 1;				// shift table left 1
			history |= taken;			// add 1 or 0 to history (T/NT)
			history &= (1<<HISTORY_LENGTH)-1;	// mask for only 15 bits (chops off MSB)
		}		
        }
};
//
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


