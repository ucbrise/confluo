#ifndef DIALOG_CONFIGURATION_PARAMS_H_
#define DIALOG_CONFIGURATION_PARAMS_H_

namespace dialog {

class configuration_params {
 public:
  // Thread configuration parameters
  static int MAX_CONCURRENCY;

  // Index configuration parameters
  static double DEFAULT_BLOCK_SIZE;
};

}

#endif /* DIALOG_CONFIGURATION_PARAMS_H_ */
