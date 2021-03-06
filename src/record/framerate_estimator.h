/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_RECORD_FRAMERATE_ESTIMATOR_H
#define VCS_RECORD_FRAMERATE_ESTIMATOR_H

#include <QElapsedTimer>

struct framerate_estimator_s
{
    uint prevFrameCount = 0;
    double fps = 0;
    QElapsedTimer timer;

    void initialize(const uint frameCount);

    void update(const uint frameCount);

    double framerate(void);

};

#endif
