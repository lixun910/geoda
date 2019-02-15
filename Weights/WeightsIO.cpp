//
// Created by Xun Li on 2/14/19.
//

#include "AbstractWeights.h" // cross?
#include "WeightsIO.h"

WeightsIO::WeightsIO() {

}

WeightsIO::WeightsIO(const char *file_path) : file_path(file_path) {
    // rea and construct weights from a file
}

WeightsIO::~WeightsIO() {

}

WeightsIO::WEIGHTS_TYPE WeightsIO::GetWeightsType(const char *path) {
    // detect weights type according to the suffix of input file path

    return GAL;
}

bool WeightsIO::Save(const char *file_path) {
    if (file_path == 0) return false;

    WEIGHTS_TYPE w_type = GetWeightsType(file_path);
    return true;
}

AbstractWeights WeightsIO::Read(const char *file_path) {
    return AbstractWeights();
}

