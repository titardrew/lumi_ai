#include <cstdlib>
#include <iomanip>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <iostream>

// clang-format off
#include "openvino/openvino.hpp"
// clang-format on

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


void printInputAndOutputsInfo(const ov::Model& network) {
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

/**
 * @brief Print IE Parameters
 * @param reference on IE Parameter
 * @return void
 */
void print_any_value(const ov::Any& value)
{
    if (value.empty()) {
        std::cout << "EMPTY VALUE" << "\n";
    } else {
        std::string stringValue = value.as<std::string>();
        std::cout << (stringValue.empty() ? "\"\"" : stringValue) << "\n";
    }
}

void Devices()
{
        // -------- Get OpenVINO runtime version --------
        std::cout << ov::get_openvino_version() << "\n";

        ov::Core core;
        std::vector<std::string> availableDevices = core.get_available_devices();

        std::cout << "Available devices: " << "\n";
        for (auto&& device : availableDevices) {
            std::cout << device << "\n";

            // Query supported properties and print all of them
            std::cout << "\tSUPPORTED_PROPERTIES: " << "\n";
            auto supported_properties = core.get_property(device, ov::supported_properties);
            for (auto&& property : supported_properties) {
                if (property != ov::supported_properties.name()) {
                    std::cout << "\t\t" << (property.is_mutable() ? "Mutable: " : "Immutable: ") << property << " : "
                               << std::flush;
                    print_any_value(core.get_property(device, property));
                }
            }

            std::cout << "\n";
        }
}

int main()
{
    Devices();

    // -------- Step 1. Initialize OpenVINO Runtime Core --------
    ov::Core core;

    // -------- Step 2. Read a model --------
    std::string model_path = "../end2end.onnx";
    std::cout << "Loading model files: " << model_path << "\n";
    std::shared_ptr<ov::Model> model = core.read_model(model_path);
    printInputAndOutputsInfo(*model);

    // -------- Step 3. Load an input image --------
    // Loading an image
    const char *image_path = "../v1_0008_prep.png";
    int32_t image_width = 0;
    int32_t image_height = 0;
    int32_t image_depth = 0;
    uint8_t *image_data = stbi_load(image_path, &image_width, &image_height, &image_depth, 3);
    if (!image_data) {
        fprintf(stderr, "Can't load image: %s \n", image_path);
        exit(1);
    }
    if (image_depth != 3) {
        fprintf(stderr, "Only 3-channel images are supported. Found: %d.\n", image_depth);
        exit(1);
    }
    
    // Back to OpenVINO world.
    ov::element::Type input_type = ov::element::u8;
    ov::Shape input_shape = {1, (uint32_t)image_height, (uint32_t)image_width, 3};
    ov::Tensor input_tensor = ov::Tensor(input_type, input_shape, image_data);
    const ov::Layout tensor_layout{"NHWC"};
    
    // -------- Step 4. Configure preprocessing --------

    ov::preprocess::PrePostProcessor ppp(model);

    // 1) Set input tensor information:
    // - input() provides information about a single model input
    // - reuse precision and shape from already available `input_tensor`
    // - layout of data is 'NHWC'
    ppp.input().tensor().set_shape(input_shape).set_element_type(input_type).set_layout(tensor_layout);
    // 2) Adding explicit preprocessing steps:
    // - convert layout to 'NCHW' (from 'NHWC' specified above at tensor layout)
    // - apply linear resize from tensor spatial dims to model spatial dims
    // @aty: not necessary for yolo, we expect a resized image.
    ppp.input().preprocess()
        .resize(ov::preprocess::ResizeAlgorithm::RESIZE_LINEAR)
        .convert_element_type(ov::element::f32)
        .scale(255.0f);
    // 4) Here we suppose model has 'NCHW' layout for input
    ppp.input().model().set_layout("NCHW");
    // 5) Set output tensor information:
    // - precision of tensor is supposed to be 'f32'
    // ppp.output().tensor().set_element_type(ov::element::f32);

    // 6) Apply preprocessing modifying the original 'model'
    model = ppp.build();

    // -------- Step 5. Loading a model to the device --------
    ov::CompiledModel compiled_model = core.compile_model(model, "CPU");

    // -------- Step 6. Create an infer request --------
    ov::InferRequest infer_request = compiled_model.create_infer_request();
    // -----------------------------------------------------------------------------------------------------

    // -------- Step 7. Prepare input --------
    infer_request.set_input_tensor(input_tensor);

    // -------- Step 8. Do inference synchronously --------
    infer_request.infer();

    // -------- Step 9. Process output
    const ov::Tensor& dets_tensor = infer_request.get_output_tensor(0);
    const ov::Tensor& labels_tensor = infer_request.get_output_tensor(1);

    ov::Shape dets_shape = dets_tensor.get_shape();
    int32_t num_detections = dets_tensor.get_shape()[1];
    const float *dets = dets_tensor.data<const float>();
    const int64_t *labels = labels_tensor.data<const int64_t>();

    for (int i_det = 0; i_det < num_detections; ++i_det) {
        float det_x    = dets[i_det * 5 + 0];
        float det_y    = dets[i_det * 5 + 1];
        float det_w    = dets[i_det * 5 + 2];
        float det_h    = dets[i_det * 5 + 3];
        float det_conf = dets[i_det * 5 + 4];

        printf("Det #%d: bbox=[%.3f %.3f %.3f %.3f] conf=%.3f label=%lld\n", i_det, det_x, det_y, det_w, det_h, det_conf, labels[i_det]);
    }

}
