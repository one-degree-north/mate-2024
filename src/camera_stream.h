#include <GLFW/glfw3.h>
#include <gst/gstelement.h>

#include "pi.h"

#ifndef MATE_CAMERA_H
#define MATE_CAMERA_H



class CameraStream {
private:
    GstElement *pipeline, *sink;
    GLuint video_texture;
    int video_width = 0;
    int video_height = 0;

    void PollCameraStream();
public:
    CameraStream(Pi &pi, std::string server_address, std::string device = "/dev/video0");
    ~CameraStream();

    GstElement* GetPipelineElement(std::string elementName);
    void FlushPipeline();
    void ShowCameraStream();
};


#endif //MATE_CAMERA_H
