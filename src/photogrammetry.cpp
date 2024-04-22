#include "imgui.h"

#include <gst/gstvalue.h>

#include <filesystem>

#include "photogrammetry-swift.h"
#include "photogrammetry.h"

Photogrammetry::Photogrammetry(CameraStream &camera_stream) : camera_stream_(camera_stream) {
    this->image_valve_element_ = camera_stream.GetPipelineElement("image-valve"); // valve
    this->image_crop_element_ = camera_stream.GetPipelineElement("image-crop"); // videocrop
    this->image_rate_filter_element_ = camera_stream.GetPipelineElement("image-rate-filter"); // capsfilter
    this->image_sink_element_ = camera_stream.GetPipelineElement("image-sink_"); // multifilesink
}

Photogrammetry::~Photogrammetry() {
    gst_object_unref(this->image_valve_element_);
    gst_object_unref(this->image_crop_element_);
    gst_object_unref(this->image_rate_filter_element_);
    gst_object_unref(this->image_sink_element_);
}

void Photogrammetry::ShowPhotogrammetryWindow() {
    ImGui::Begin("Reconstruction");

    if (ImGui::SliderInt("Recording Frame Rate", &this->recording_frame_rate_, 1, 30)) {
        GstCaps* caps = gst_caps_new_simple("video/x-raw", "framerate", GST_TYPE_FRACTION, this->recording_frame_rate_, 1, nullptr);
        g_object_set(this->image_rate_filter_element_, "caps", caps, nullptr);
        gst_caps_unref(caps);
    }

    static int cropBottom = 0;
    if (ImGui::SliderInt("Bottom Crop Pixels", &cropBottom, 0, 500)) {
        g_object_set(this->image_crop_element_, "bottom", cropBottom, nullptr);
    }

    if (this->is_recording_) {
        if (ImGui::Button("Stop Recording")) {
            g_object_set(image_valve_element_, "drop", true, nullptr);
            this->is_recording_ = false;
        }
    } else {
        if (ImGui::Button("Start Recording")) {
            std::filesystem::remove_all("images");
            std::filesystem::create_directory("images");

            this->camera_stream_.FlushPipeline();

            g_object_set(image_sink_element_, "index", 0, nullptr);
            g_object_set(image_valve_element_, "drop", false, nullptr);

            this->is_recording_ = true;
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::ProgressBar((float) PhotogrammetrySwift::GetProgress(), ImVec2(0.0f, 0.0f));
    ImGui::SameLine();
    ImGui::Text("ETA: %0.2fs", PhotogrammetrySwift::GetETA());

    if (ImGui::Button("Start Photogrammetry Session")) {
        if (std::filesystem::is_empty("images")) {
            ImGui::OpenPopup("No Images to Process");
        } else {
            PhotogrammetrySwift::RunPhotogrammetrySession("images");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Model")) {
        std::system("open out.usdz");
    }
    ImGui::End();
}

