//
// Created by Xun Li on 2/14/19.
//

#ifndef SPATIALWEIGHTS_WEIGHTSIO_H
#define SPATIALWEIGHTS_WEIGHTSIO_H

class AbstractWeights; // forward declaration

/**
 * WeightsIO is a abstract class defined for high-level generalization of
 * reading/writing weights file of different formats, including:
 * GAL, GWT, KWT, SWM, MAT, H5
 *
 */
class WeightsIO {
public:
    WeightsIO();

    WeightsIO(const char *file_path);

    virtual ~WeightsIO();

    enum WEIGHTS_TYPE {GAL, GWT, KWT, SWM, MAT, H5};

    bool Save(const char *file_path);

    AbstractWeights Read(const char *file_path);

protected:
    const char* file_path;

    WEIGHTS_TYPE weights_type;

    WEIGHTS_TYPE GetWeightsType(const char *path);
};



#endif //SPATIALWEIGHTS_WEIGHTSIO_H
