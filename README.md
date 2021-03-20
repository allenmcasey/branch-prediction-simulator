## Branch Prediction Simulator ##

This repository simulates the dynamic branch prediction capabilities of a Pentium M processor. The simulator leverages a 4-way set associative cache as a global predictor, a bimodal table of 2-bit counters, and a global branch history register to make accurate branch predictions. <br/><br/>

### Running the predictor ###

To compile the predictor, type ``make`` under the ``src`` directory. You will see that this generates a binary file named ``predict``. Then, type the following command under the main repository directory: ``csh run traces``. 

You will see several test files being executed against the predictor, and after all files are finished, the resulting prediction accuracy averages will be printed in terms of MPKI (missed predictions per thousand instructions). The MPKI displayed should be close to 9.6.
