#pragma once

#include "lumi.h"

#ifndef DL_EXPORT
#error "You must define DL_EXPORT before including this header"
#endif

extern "C" {
    DL_EXPORT void SetupDetector(int i_device, int input_width, int input_height, const char *model_path)
    {
        lumi::api::SetupDetector(i_device, input_width, input_height, model_path);
    }

    DL_EXPORT int InferDetector(uint8_t *image_data)
    {
        return lumi::api::InferDetector(image_data);
    }

    DL_EXPORT const uint8_t *GetDetections()
    {
        return lumi::api::GetDetections();
    }

	DL_EXPORT int FindDevices()
    {
        return lumi::api::FindDevices();
	}

    DL_EXPORT const char *GetDeviceName(int i_device)
    {
        return lumi::api::GetDeviceName(i_device);
    }

    DL_EXPORT const char *GetDeviceFullName(int i_device)
    {
        return lumi::api::GetDeviceFullName(i_device);
    }

    DL_EXPORT const char *GetDeviceDescription(int i_device)
    {
        return lumi::api::GetDeviceDescription(i_device);
    }

    DL_EXPORT void DisposeDetector()
    {
        return lumi::api::DisposeDetector();
    }
}