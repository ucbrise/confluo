#ifndef CONFLUO_UNIV_SKETCH_LOG_H
#define CONFLUO_UNIV_SKETCH_LOG_H

#include "container/monolog/monolog_exp2.h"
#include "container/sketch/confluo_universal_sketch.h"
#include "container/sketch/universal_sketch.h"

namespace confluo {

/** A log for triggers */
typedef monolog::monolog_exp2<sketch::confluo_universal_sketch<> *> univ_sketch_log;

}

#endif //CONFLUO_UNIV_SKETCH_LOG_H
