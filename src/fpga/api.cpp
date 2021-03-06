/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "api.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <cstring>
#include "bias_scale.h"
#include "filter.h"
#include "image.h"

#define FPGA_TEST_MODE

namespace paddle_mobile {
namespace fpga {

static int fd = -1;
static const char *device_path = "/dev/fpgadrv0";

static inline int do_ioctl(int req, const void *arg) {
#ifdef PADDLE_MOBILE_OS_LINUX
  return ioctl(req, (unsigned int64_t)arg);
#else
  return -1;
#endif
}

int open_device() {
  if (fd == -1) {
    fd = open(device_path, O_RDWR);
  }
  return fd;
}

// memory management;
void *fpga_malloc(size_t size) {
#ifdef PADDLE_MOBILE_OS_LINUX
  return reinterpret_cast<void *>(
      mmap64(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
#else
  return malloc(size);
#endif
}

void fpga_free(void *ptr) {
#ifdef PADDLE_MOBILE_OS_LINUX
  munmap(ptr, 0);
#else
  free(ptr);
#endif
}

void fpga_copy(void *dest, const void *src, size_t num) {
  memcpy(dest, src, num);
}

int ComputeFpgaConv(const struct WrapperConvArgs &args) {
#ifdef FPGA_TEST_MODE
/*DLOG << "   relu_enabled:" << args.relu_enabled
     << "   sb_address:" << args.sb_address
     << "   filter_address:" << args.filter_address
     << "   filter_num:" << args.filter_num
     << "   group_num:" << args.group_num;
DLOG << "   image_address:" << args.image.address
     << "   image_scale_address:" << args.image.scale_address
     << "   image_channels:" << args.image.channels
     << "   image_height:" << args.image.height
     << "   image_width:" << args.image.width
     << "   pad_height:" << args.image.pad_height
     << "   pad_width:" << args.image.pad_width;
DLOG << "   kernel_height:" << args.kernel.height
     << "   kernel_width:" << args.kernel.width
     << "   stride_h:" << args.kernel.stride_h
     << "   stride_w:" << args.kernel.stride_w;
DLOG << "   out_address:" << args.output.address
     << "   out_scale_address:" << args.output.scale_address;*/
#endif
  int split_num = args.split_num;
  for (int i = 0; i < split_num; i++) {
    do_ioctl(IOCTL_CONFIG_CONV, &args.conv_args[i]);
  }

  if (split_num > 1) {
    ComputeFPGAConcat(args.concat_arg);
  }
}

int ComputeFpgaPool(const struct PoolingArgs &args) {
#ifdef FPGA_TEST_MODE
  DLOG << "   image_address:" << args.image.address
       << "   image_scale_address:" << args.image.scale_address
       << "   image_channels:" << args.image.channels
       << "   image_height:" << args.image.height
       << "   image_width:" << args.image.width
       << "   pad_height:" << args.image.pad_height
       << "   pad_width:" << args.image.pad_width;
  DLOG << "   kernel_height:" << args.kernel.height
       << "   kernel_width:" << args.kernel.width
       << "   stride_h:" << args.kernel.stride_h
       << "   stride_w:" << args.kernel.stride_w;
  DLOG << "   out_address:" << args.output.address
       << "   out_scale_address:" << args.output.scale_address;
#endif

  return do_ioctl(IOCTL_CONFIG_POOLING, &args);
}

int ComputeFpgaEWAdd(const struct EWAddArgs &args) {
#ifdef FPGA_TEST_MODE
  DLOG << "   relu_enabled:" << args.relu_enabled << "   const0:" << args.const0
       << "   const1:" << args.const1;
  DLOG << "   image0_address:" << args.image0.address
       << "   image0_scale_address:" << args.image0.scale_address
       << "   image0_channels:" << args.image0.channels
       << "   image0_height:" << args.image0.height
       << "   image0_width:" << args.image0.width
       << "   pad0_height:" << args.image0.pad_height
       << "   pad0_width:" << args.image0.pad_width;
  DLOG << "   image1_address:" << args.image1.address
       << "   image1_scale_address:" << args.image1.scale_address
       << "   image1_channels:" << args.image1.channels
       << "   image1_height:" << args.image1.height
       << "   image1_width:" << args.image1.width
       << "   pad1_height:" << args.image1.pad_height
       << "   pad_width:" << args.image1.pad_width;
  DLOG << "   out_address:" << args.output.address
       << "   out_scale_address:" << args.output.scale_address;
#endif

  return do_ioctl(IOCTL_CONFIG_EW, &args);
}
int PerformBypass(const struct BypassArgs &args) {
#ifdef FPGA_TEST_MODE
  DLOG << "   layout_type:" << args.layout_type
       << "   convert_type:" << args.convert_type;
  DLOG << "   image_address:" << args.image.address
       << "   image_scale_address:" << args.image.scale_address
       << "   image_channels:" << args.image.channels
       << "   image_height:" << args.image.height
       << "   image_width:" << args.image.width
       << "   pad_height:" << args.image.pad_height
       << "   pad_width:" << args.image.pad_width;
  DLOG << "   out_address:" << args.output.address
       << "   out_scale_address:" << args.output.scale_address;
#endif

  return do_ioctl(IOCTL_CONFIG_BYPASS, &args);
}

int ComputeFPGAConcat(const struct ConcatArgs &args) {
  image::concat_images(args.images_in, args.scales_in, args.image_out,
                       args.scale_out, args.image_num, args.channel_num,
                       args.height, args.width);
  return 0;
}

void format_image(framework::Tensor *image_tensor) {
  auto dims = image_tensor->dims();
  auto channel = dims[1], height = dims[2], width = dims[3];
  auto data_ptr = image_tensor->mutable_data<float>();
  size_t memory_size = channel * height * width * sizeof(float);
  float *new_data = (float *)fpga_malloc(memory_size);
  fpga_copy(new_data, data_ptr, memory_size);
  image::format_image(&new_data, channel, height, width);
  image_tensor->reset_data_ptr(new_data);
}

void format_ofm(framework::Tensor *ofm_tensor) {
  auto dims = ofm_tensor->dims();
  auto channel = dims[1], height = dims[2], width = dims[3];
  size_t memory_size =
      height * align_to_x(channel * width, IMAGE_ALIGNMENT) * sizeof(half);
  ofm_tensor->reset_data_ptr(fpga_malloc(memory_size));
}

float filter_find_max(framework::Tensor *filter_tensor) {
  auto filter_ptr = filter_tensor->data<float>();
  return filter::find_max(filter_ptr, filter_tensor->numel());
}

int get_plit_num(framework::Tensor *filter_tensor) {
  auto dims = filter_tensor->dims();
  auto chw = dims[1] * dims[2] * dims[3];
  auto num = dims[0];
  int div_capacity = filter::calc_division_capacity(chw);
  return filter::calc_split_num(num, div_capacity);
}

int get_element_num_per_div(framework::Tensor *filter_tensor, int group_num) {
  auto dims = filter_tensor->dims();
  auto chw = dims[1] * dims[2] * dims[3];
  auto num = dims[0];
  int div_capacity = filter::calc_division_capacity(chw);
  return filter::calc_num_per_div(num, group_num, div_capacity);
}

int get_aligned_filter_element_num(int chw) {
  return align_to_x(chw, FILTER_ELEMENT_ALIGNMENT);
}

int get_aligned_filter_num(int num) {
  return align_to_x(num, FILTER_NUM_ALIGNMENT);
}

void format_filter(framework::Tensor *filter_tensor, float max_value,
                   int group_num) {
  auto dims = filter_tensor->dims();
  auto num = dims[0], channel = dims[1], height = dims[2], width = dims[3];
  auto data_ptr = filter_tensor->mutable_data<float>();
  size_t memory_size = num * channel * height * width * sizeof(float);
  auto new_data = (float *)fpga_malloc(memory_size);
  fpga_copy(new_data, data_ptr, memory_size);
  filter::format_filter(&new_data, num, channel, height, width, group_num,
                        max_value);
  filter_tensor->reset_data_ptr(new_data);
}

void format_bias_scale_array(float **bias_scale_array,
                             int element_num_per_division, int num) {
  bias_scale::format_bias_scale_array(bias_scale_array,
                                      element_num_per_division, num);
}

void format_concat_output(framework::Tensor *out, int height, int width,
                          int image_num, uint32_t *channel_num) {
  int sum_channel = 0, sum_cw = 0;
  for (int i = 0; i < image_num; i++) {
    sum_channel += channel_num[i];
  }

  sum_cw = align_to_x(width * sum_channel, IMAGE_ALIGNMENT);
  auto data_ptr = fpga_malloc(height * sum_cw * sizeof(half));
  auto ddim = framework::make_ddim({-1, sum_channel, height, width});
  out->Resize(ddim);
  out->reset_data_ptr(data_ptr);
}

}  // namespace fpga
}  // namespace paddle_mobile
