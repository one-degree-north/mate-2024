#include <GLFW/glfw3.h>
#include <gst/gstelement.h>

#include "pi.h"

#ifndef MATE_CAMERA_H
#define MATE_CAMERA_H



class CameraStream {
private:
    Pi &pi;
    GstElement *pipeline, *sink;
    GLuint video_texture{};
    int video_width = 0;
    int video_height = 0;

    void PollCameraStream();
public:
    CameraStream(Pi &pi, const std::string& server_address, const std::string& device = "/dev/video0");
    ~CameraStream();

    GstElement* GetPipelineElement(const std::string& elementName);
    void FlushPipeline();
    void ShowCameraStream();
};


#endif //MATE_CAMERA_H
