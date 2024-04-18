//
// Created by Sidharth Maheshwari on 17/4/24.
//

#include <GLFW/glfw3.h>

#include <imgui.h>

#include <gst/gst.h>
#include <gst/app/app.h>

#include <iostream>

#include "camera_stream.h"

CameraStream::CameraStream(Pi &pi, const std::string& server_address, const std::string& device) : pi(pi) {
    gst_init(nullptr, nullptr);

    if (pi.Shell("run", "gst-launch-1.0 videotestsrc ! jpegenc ! rtpjpegpay ! udpsink host=192.168.1.1 port=6970 &> /dev/null &") < 0) {
        std::cerr << "Failed to start camera stream" << std::endl;
        exit(1);
    }

    this->pipeline = gst_parse_launch(
"udpsrc port=6970 ! application/x-rtp,encoding-name=JPEG ! rtpjpegdepay ! jpegparse ! jpegdec ! tee name=t "
                "t. ! queue leaky=downstream ! videoconvert ! video/x-raw,format=RGB ! appsink name=gui-sink drop=true sync=false "
                "t. ! queue leaky=downstream ! valve name=image-valve drop=true ! videocrop name=image-crop bottom=0 ! videorate skip-to-first=true max-closing-segment-duplication-duration=0 ! capsfilter caps-change-mode=immediate name=image-rate-filter caps=video/x-raw,framerate=1/1 ! jpegenc ! multifilesink name=image-sink location=images/image%d.jpeg async=false",
    nullptr);

    if (!this->pipeline) {
        std::cerr << "Failed to create pipeline" << std::endl;
        exit(1);
    }

    this->sink = this->GetPipelineElement("gui-sink");

    gst_element_set_state(this->pipeline, GST_STATE_PLAYING);

    glGenTextures(1, &this->video_texture);
    glBindTexture(GL_TEXTURE_2D, this->video_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

CameraStream::~CameraStream() {
    gst_element_set_state(this->pipeline, GST_STATE_NULL);
    pi.Shell("run", "killall gst-launch-1.0");

    gst_object_unref(this->pipeline);
    gst_object_unref(this->sink);
}

GstElement* CameraStream::GetPipelineElement(const std::string& elementName) {
    return gst_bin_get_by_name(GST_BIN(this->pipeline), elementName.c_str());
}

void CameraStream::ShowCameraStream() {
    this->PollCameraStream();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Camera Stream");

    ImVec2 windowSize = ImGui::GetWindowSize();
    windowSize.y -= ImGui::GetFrameHeight();

    ImVec2 imageSize((float) video_width, (float) video_height);

    if (windowSize.x / windowSize.y > imageSize.x / imageSize.y) {
        imageSize.x = windowSize.y * imageSize.x / imageSize.y;
        imageSize.y = windowSize.y;
        ImGui::SetCursorPosX((windowSize.x - imageSize.x) / 2);
    } else {
        imageSize.y = windowSize.x * imageSize.y / imageSize.x;
        imageSize.x = windowSize.x;
        ImGui::SetCursorPosY(ImGui::GetFrameHeight() + (windowSize.y - imageSize.y) / 2);
    }

    ImGui::Image((void*)(intptr_t) this->video_texture, imageSize, ImVec2(0, 0), ImVec2(1, 1));

    ImGui::End();
    ImGui::PopStyleVar();
}

void CameraStream::PollCameraStream() {
    GstSample* videoSample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),  GST_NSECOND);
    if (videoSample) {
        if (this->video_width == 0 || this->video_height == 0) {
            GstCaps *caps = gst_sample_get_caps(videoSample);
            GstStructure *structure = gst_caps_get_structure(caps, 0);

            gst_structure_get_int(structure, "width", &this->video_width);
            gst_structure_get_int(structure, "height", &this->video_height);
        }

        GstBuffer* videoBuffer = gst_sample_get_buffer(videoSample);
        GstMapInfo map;

        gst_buffer_map (videoBuffer, &map, GST_MAP_READ);

        glBindTexture(GL_TEXTURE_2D, this->video_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->video_width, this->video_height, 0, GL_RGB, GL_UNSIGNED_BYTE, map.data);
        gst_buffer_unmap(videoBuffer, &map);
        gst_sample_unref(videoSample);
    }
}

void CameraStream::FlushPipeline() {
    gst_element_send_event(pipeline, gst_event_new_flush_start());
    gst_element_send_event(pipeline, gst_event_new_flush_stop(true));
}
