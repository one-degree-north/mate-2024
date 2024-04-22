#include <GLFW/glfw3.h>
#include <gst/gstelement.h>

#include "pi.h"

#ifndef MATE_CAMERA_STREAM_H
#define MATE_CAMERA_STREAM_H

class CameraStream {
public:
    CameraStream(Pi &pi, const std::string& server_address, const std::string& device = "/dev/video0");
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

    void PollCameraStream();
};


#endif // MATE_CAMERA_STREAM_H
