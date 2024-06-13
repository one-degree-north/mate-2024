#include <GLFW/glfw3.h>
#include <gst/gstelement.h>
#include <string>
#include "pi.h"

#ifndef MATE_CAMERA_STREAM_H
#define MATE_CAMERA_STREAM_H

class CameraStream {
public:
    CameraStream(Pi &pi, int port);
    ~CameraStream();

    GstElement* GetPipelineElement(const std::string& elementName);
    void FlushPipeline();
    void ShowCameraStream();
private:
    Pi &pi_;
    GstElement *pipeline_, *sink_;
    GLuint video_texture_ = 0;
    int video_width_ = 0;
    int video_height_ = 0;
    int port_;
    const char* window_id_;

    void PollCameraStream();
};


#endif // MATE_CAMERA_STREAM_H
