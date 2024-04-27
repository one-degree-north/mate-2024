#include <GLFW/glfw3.h>
#include <imgui.h>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <iostream>
#include "camera_stream.h"

CameraStream::CameraStream(Pi &pi, const std::string& server_address, const std::string& device) : pi_(pi) {
    gst_init(nullptr, nullptr);

    int exit_code = pi.Shell("run", "gst-launch-1.0 v4l2src device=/dev/video0 ! jpegenc ! rtpjpegpay ! udpsink host=192.168.1.1 port=6970 &> /dev/null &");
    if (exit_code < 0)
        throw std::runtime_error("Failed to start camera stream");

    std::string pipeline_description = "udpsrc port=6970 ! application/x-rtp,encoding-name=JPEG ! rtpjpegdepay ! jpegdec ! tee name=t ";

    for (int i = 0; i < num_devices; ++i) {
        pipeline_description += "t. ! queue leaky=downstream ! videoconvert ! video/x-raw,format=RGB ! jpegenc ! udpsink host=destination_ip_" + std::to_string(i) + " port=destination_port_" + std::to_string(i) + " ";
    }

    this->pipeline_ = gst_parse_launch(pipeline_description.c_str(), nullptr);

    if (!this->pipeline_) {
        std::cerr << "Failed to create pipeline_" << std::endl;
        exit(1);
    }

    this->sink_ = this->GetPipelineElement("gui-sink_");

    gst_element_set_state(this->pipeline_, GST_STATE_PLAYING);

    glGenTextures(1, &this->video_texture_);
    glBindTexture(GL_TEXTURE_2D, this->video_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

CameraStream::~CameraStream() {
    gst_element_set_state(this->pipeline_, GST_STATE_NULL);
    pi_.Shell("run", "killall gst-launch-1.0");

    gst_object_unref(this->pipeline_);
    gst_object_unref(this->sink_);
}

GstElement* CameraStream::GetPipelineElement(const std::string& elementName) {
    return gst_bin_get_by_name(GST_BIN(this->pipeline_), elementName.c_str());
}

void CameraStream::ShowCameraStream() {
    this->PollCameraStream();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Camera Stream");

    ImVec2 windowSize = ImGui::GetWindowSize();
    windowSize.y -= ImGui::GetFrameHeight();

    ImVec2 imageSize((float) video_width_, (float) video_height_);

    if (windowSize.x / windowSize.y > imageSize.x / imageSize.y) {
        imageSize.x = windowSize.y * imageSize.x / imageSize.y;
        imageSize.y = windowSize.y;
        ImGui::SetCursorPosX((windowSize.x - imageSize.x) / 2);
    } else {
        imageSize.y = windowSize.x * imageSize.y / imageSize.x;
        imageSize.x = windowSize.x;
        ImGui::SetCursorPosY(ImGui::GetFrameHeight() + (windowSize.y - imageSize.y) / 2);
    }

    ImGui::Image((void*)(intptr_t) this->video_texture_, imageSize, ImVec2(0, 0), ImVec2(1, 1));

    ImGui::End();
    ImGui::PopStyleVar();
}

void CameraStream::PollCameraStream() {
    GstSample* videoSample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink_), GST_NSECOND);
    if (videoSample) {
        if (this->video_width_ == 0 || this->video_height_ == 0) {
            GstCaps *caps = gst_sample_get_caps(videoSample);
            GstStructure *structure = gst_caps_get_structure(caps, 0);

            gst_structure_get_int(structure, "width", &this->video_width_);
            gst_structure_get_int(structure, "height", &this->video_height_);
        }

        GstBuffer* videoBuffer = gst_sample_get_buffer(videoSample);
        GstMapInfo map;

        gst_buffer_map (videoBuffer, &map, GST_MAP_READ);

        glBindTexture(GL_TEXTURE_2D, this->video_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->video_width_, this->video_height_, 0, GL_RGB, GL_UNSIGNED_BYTE, map.data);
        gst_buffer_unmap(videoBuffer, &map);
        gst_sample_unref(videoSample);
    }
}

void CameraStream::FlushPipeline() {
    gst_element_send_event(pipeline_, gst_event_new_flush_start());
    gst_element_send_event(pipeline_, gst_event_new_flush_stop(true));
}
