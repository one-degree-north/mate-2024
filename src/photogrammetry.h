//
// Created by Sidharth Maheshwari on 18/4/24.
//

#include <gst/gstelement.h>

#include "camera_stream.h"

#ifndef MATE_PHOTOGRAMMETRY_H
#define MATE_PHOTOGRAMMETRY_H


class Photogrammetry {
private:
    CameraStream camera_stream;
    GstElement *image_valve_element, *image_crop_element, *image_rate_filter_element, *image_sink_element;
    bool is_recording = false;
    int recording_frame_rate = 1;
public:
    explicit Photogrammetry(CameraStream &camera);
    ~Photogrammetry();

    void ShowPhotogrammetryWindow();
};


#endif //MATE_PHOTOGRAMMETRY_H
