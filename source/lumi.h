#pragma once

#include <cstdlib>

// clang-format off
#include "openvino/openvino.hpp"
// clang-format on

namespace lumi {

struct Detection {
    Detection(float x, float y, float w, float h, float conf, int32_t label)
      : x(x), y(y), w(w), h(h), conf(conf), label(label) {}

    float   x;
    float   y;
    float   w;
    float   h;
    float   conf;
    int32_t label;
};


class Detector {
  public:
    Detector(int image_width, int image_height, std::string model_path, std::string device_name = "CPU");

    int Infer(uint8_t *image_data);

    std::vector<Detection> *GetDetections()
    {
        return &m_detections;
    }

  private:
    static constexpr float kSmallConfidenceThreshold = 0.002f;

    ov::Core                                          m_core;
    std::unique_ptr<ov::preprocess::PrePostProcessor> m_pre_post_processor;
    ov::CompiledModel                                 m_compiled_model;
    ov::Shape                                         m_input_shape;
    std::vector<Detection>                            m_detections;
};


namespace api {

void            DisposeDetector();
void            SetupDetector(int i_device, int input_width, int input_height, const char *model_path);
int             InferDetector(uint8_t *image_data);
const uint8_t * GetDetections();
int             FindDevices();
const char *    GetDeviceName(int i_device);
const char *    GetDeviceFullName(int i_device);
const char *    GetDeviceDescription(int i_device);

}  // namespace api

}  // namespace lumi