#include <gst/gstelement.h>

#include "camera_stream.h"

#ifndef MATE_PHOTOGRAMMETRY_H
#define MATE_PHOTOGRAMMETRY_H


class Photogrammetry {
public:
    explicit Photogrammetry(CameraStream &camera);
    ~Photogrammetry();

    void ShowPhotogrammetryWindow();
private:
    CameraStream camera_stream_;
    GstElement *image_valve_element_, *image_crop_element_, *image_rate_filter_element_, *image_sink_element_;
    bool is_recording_ = false;
    int recording_frame_rate_ = 1;
};


#endif //MATE_PHOTOGRAMMETRY_H
