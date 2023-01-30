#include "lumi.h"

#include <cstdlib>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <iostream>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


namespace lumi {

static void PrintInputAndOutputsInfo(const ov::Model& network) {
    std::cout << "model name: " << network.get_friendly_name() << "\n";

    const std::vector<ov::Output<const ov::Node>> inputs = network.inputs();
    for (const ov::Output<const ov::Node> input : inputs) {
            std::cout << "    inputs" << std::endl;

            const std::string name = input.get_names().empty() ? "NONE" : input.get_any_name();
            std::cout << "        input name: " << name << std::endl;

            const ov::element::Type type = input.get_element_type();
            std::cout << "        input type: " << type << std::endl;

            const ov::Shape shape = input.get_shape();
            std::cout << "        input shape: " << shape << std::endl;
        }

    const std::vector<ov::Output<const ov::Node>> outputs = network.outputs();
    for (const ov::Output<const ov::Node> output : outputs) {
            std::cout << "    outputs" << std::endl;

            const std::string name = output.get_names().empty() ? "NONE" : output.get_any_name();
            std::cout << "        output name: " << name << std::endl;

            const ov::element::Type type = output.get_element_type();
            std::cout << "        output type: " << type << std::endl;

            const ov::PartialShape shape = output.get_partial_shape();
            std::cout << "        output shape: " << shape << std::endl;
        }
}


Detector::Detector(int image_width, int image_height, std::string model_path, std::string device_name)
{
    m_input_shape = {1, (uint32_t)image_height, (uint32_t)image_width, 3};

    std::shared_ptr<ov::Model> model = m_core.read_model(model_path);
    // PrintInputAndOutputsInfo(*model);
    
    m_pre_post_processor = std::make_unique<ov::preprocess::PrePostProcessor>(model);

    // Set input tensor information
    const ov::Layout tensor_layout{"NHWC"};
    m_pre_post_processor->input().tensor()
        .set_shape(m_input_shape)
        .set_element_type(ov::element::u8)
        .set_layout(tensor_layout);

    // Adding explicit preprocessing steps
    // NOTE(@aty): not necessary for yolo, we expect a resized image.
    m_pre_post_processor->input().preprocess()
        .resize(ov::preprocess::ResizeAlgorithm::RESIZE_LINEAR)
        .convert_element_type(ov::element::f32)
        .scale(255.0f);

    // Here we suppose model has 'NCHW' layout for input
    m_pre_post_processor->input().model().set_layout("NCHW");

    // Apply preprocessing modifying the original 'model'
    model = m_pre_post_processor->build();

    // Loading a model to the device
    m_compiled_model = m_core.compile_model(model, device_name);
}


int Detector::Infer(uint8_t *image_data)
{
    // int ret = stbi_write_bmp("/Users/andrii.tytarenko/Desktop/out.bmp", m_input_shape[2], m_input_shape[1], 3, image_data);

    ov::Tensor m_input_tensor = ov::Tensor(ov::element::u8, m_input_shape, image_data);
    
    ov::InferRequest infer_request = m_compiled_model.create_infer_request();

    infer_request.set_input_tensor(m_input_tensor);
    infer_request.infer();

    const ov::Tensor& dets_tensor   = infer_request.get_output_tensor(0);
    const ov::Tensor& labels_tensor = infer_request.get_output_tensor(1);

    int32_t        num_detections = dets_tensor.get_shape()[1];
    const float   *dets           = dets_tensor.data<const float>();
    const int64_t *labels         = labels_tensor.data<const int64_t>();

    m_detections.clear();
    for (int i_det = 0; i_det < num_detections; ++i_det) {
        float conf = dets[i_det * 5 + 4];

        if (conf > kSmallConfidenceThreshold) {
            float input_height = static_cast<float>(m_input_shape[1]);
            float input_width  = static_cast<float>(m_input_shape[2]);

            // Doing emplace instead of a designated initialization because of MVCC.
            // It considers this a C++20 feature. Thanks, MVCC.
            m_detections.emplace_back(
                /* x     */ dets[i_det * 5 + 0] / input_width,
                /* y     */ dets[i_det * 5 + 1] / input_height,
                /* w     */ (dets[i_det * 5 + 2] - dets[i_det * 5 + 0]) / input_width,
                /* h     */ (dets[i_det * 5 + 3] - dets[i_det * 5 + 1]) / input_height,
                /* conf  */ conf,
                /* label */ (int32_t)labels[i_det]
            );
        }
    }
    return m_detections.size();
}

namespace api {

struct OVDevice {
    std::string name;
    std::string full_name;
    std::string description;
};

std::unique_ptr<Detector> g__detector;
std::vector<OVDevice>     g__available_devices;

void DisposeDetector()
{
    g__available_devices.clear();
    g__detector.reset();
}

void SetupDetector(int i_device, int input_width, int input_height, const char *model_path)
{
    if (g__available_devices.empty()) {
        FindDevices();
    }

    if (i_device >= g__available_devices.size()) {
        throw std::invalid_argument("i_device > number of available devices.");
    }

    std::string device_name = g__available_devices[i_device].name;
    g__detector = std::make_unique<Detector>(input_width, input_height, model_path, device_name);
}

int InferDetector(uint8_t *image_data)
{
    int num_detections = g__detector->Infer(image_data);
    return num_detections;
}

const uint8_t *GetDetections()
{
    std::vector<Detection> *dets = g__detector->GetDetections();
    return reinterpret_cast<const uint8_t *>(dets->data());
}

int FindDevices()
{
    ov::Core core;
    g__available_devices.clear();

    for (auto&& device_name : core.get_available_devices()) {
	// TODO(aty): GNA devices are not supported. They crash an app. But why?
	// According to logs, GNA supports only integer operations. Investigate
	// later.
    // UPDATE(aty): GNA - is a cool low-power Intel neural accelerator.
    // This sounds awesome, but we won't need that for now. This is not
    // an embedded GPU.

	if (device_name.find("GNA") != std::string::npos) continue;
        auto device_full_name = core.get_property(device_name, ov::device::full_name);

        std::stringstream device_description;
        device_description << device_name << "\n";

        // Query supported properties and print all of them
        device_description << "\tSUPPORTED_PROPERTIES: " << "\n";
        auto supported_properties = core.get_property(device_name, ov::supported_properties);
        for (auto&& property : supported_properties) {
            if (property != ov::supported_properties.name()) {
                device_description << "\t\t" << (property.is_mutable() ? "Mutable: " : "Immutable: ") << property << " : ";
                auto value = core.get_property(device_name, property);

                if (value.empty()) {
                    device_description << "EMPTY VALUE" << "\n";
                } else {
                    std::string str_val = value.as<std::string>();
                    device_description << (str_val.empty() ? "\"\"" : str_val) << "\n";
                }
            }
        }
        device_description << "\n";

        OVDevice d;
        d.name = device_name;
        d.full_name = device_full_name;
        d.description = device_description.str();

        g__available_devices.push_back(d);
    }

    // Reverse the order of the list
    std::reverse(g__available_devices.begin(), g__available_devices.end());

    // Configure the cache directory for GPU compute devices
    // core.set_config({ {CONFIG_KEY(CACHE_DIR), "cache"} }, "GPU");

    return g__available_devices.size();
}

const char *GetDeviceName(int i_device)
{
    return g__available_devices[i_device].name.c_str();
}

const char *GetDeviceFullName(int i_device)
{
    return g__available_devices[i_device].full_name.c_str();
}

const char *GetDeviceDescription(int i_device)
{
    return g__available_devices[i_device].description.c_str();
}

}  // namespace api
}  // namespace lumi