/*
 * Copyright (c) 2016, 2017 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "arm_compute/core/CL/CLKernelLibrary.h"

#include "arm_compute/core/Error.h"
#include "arm_compute/core/Utils.h"

#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

using namespace arm_compute;

Program::Program()
    : _context(), _device(), _is_binary(false), _name(), _source(), _binary()
{
}

Program::Program(cl::Context context, std::string name, std::string source)
    : _context(std::move(context)), _device(), _is_binary(false), _name(std::move(name)), _source(std::move(source)), _binary()
{
}

Program::Program(cl::Context context, cl::Device device, std::string name, std::vector<unsigned char> binary)
    : _context(std::move(context)), _device(std::move(device)), _is_binary(true), _name(std::move(name)), _source(), _binary(std::move(binary))
{
}

Program::operator cl::Program() const
{
    if(_is_binary)
    {
        return cl::Program(_context, { _device }, { _binary });
    }
    else
    {
        return cl::Program(_context, _source, false);
    }
}

bool Program::build(const cl::Program &program, const std::string &build_options)
{
    try
    {
        return program.build(build_options.c_str()) == CL_SUCCESS;
    }
    catch(const cl::Error &e)
    {
        cl_int     err        = CL_SUCCESS;
        const auto build_info = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&err);

        for(auto &pair : build_info)
        {
            std::cerr << pair.second << std::endl;
        }

        return false;
    }
}

cl::Program Program::build(const std::string &build_options) const
{
    cl::Program cl_program = static_cast<cl::Program>(*this);
    build(cl_program, build_options);
    return cl_program;
}

Kernel::Kernel()
    : _name(), _kernel()
{
}

Kernel::Kernel(std::string name, const cl::Program &program)
    : _name(std::move(name)),
      _kernel(cl::Kernel(program, _name.c_str()))
{
}

const std::map<std::string, std::string> CLKernelLibrary::_kernel_program_map =
{
    { "absdiff", "absdiff.cl" },
    { "accumulate", "accumulate.cl" },
    { "accumulate_squared", "accumulate.cl" },
    { "accumulate_weighted", "accumulate.cl" },
    { "activation_layer", "activation_layer.cl" },
    { "arithmetic_add", "arithmetic_op.cl" },
    { "arithmetic_sub", "arithmetic_op.cl" },
    { "bitwise_or", "bitwise_op.cl" },
    { "bitwise_and", "bitwise_op.cl" },
    { "bitwise_xor", "bitwise_op.cl" },
    { "bitwise_not", "bitwise_op.cl" },
    { "channel_combine_NV", "channel_combine.cl" },
    { "channel_combine_RGB888", "channel_combine.cl" },
    { "channel_combine_RGBA8888", "channel_combine.cl" },
    { "channel_combine_UYVY422", "channel_combine.cl" },
    { "channel_combine_YUYV422", "channel_combine.cl" },
    { "channel_extract_NV12", "channel_extract.cl" },
    { "channel_extract_NV21", "channel_extract.cl" },
    { "channel_extract_RGB888", "channel_extract.cl" },
    { "channel_extract_RGBA8888", "channel_extract.cl" },
    { "channel_extract_UYVY422", "channel_extract.cl" },
    { "channel_extract_YUYV422", "channel_extract.cl" },
    { "combine_gradients_L1", "canny.cl" },
    { "combine_gradients_L2", "canny.cl" },
    { "convolution_rectangle", "convolution_rectangle.cl" },
    { "col2im", "convolution_layer.cl" },
    { "convolution3x3_static", "convolution3x3.cl" },
    { "convolution5x5_static", "convolution5x5.cl" },
    { "convolution7x7_static", "convolution7x7.cl" },
    { "convolution9x9_static", "convolution9x9.cl" },
    { "convolution_separable1x5_static", "convolution5x5.cl" },
    { "convolution_separable5x1_static", "convolution5x5.cl" },
    { "convolution_separable1x7_static", "convolution7x7.cl" },
    { "convolution_separable7x1_static", "convolution7x7.cl" },
    { "convolution_separable1x9_static", "convolution9x9.cl" },
    { "convolution_separable9x1_static", "convolution9x9.cl" },
    { "convert_depth_down", "depth_convert.cl" },
    { "convert_depth_up", "depth_convert.cl" },
    { "copy_plane", "channel_extract.cl" },
    { "copy_planes_3p", "channel_combine.cl" },
    { "copy_to_keypoint", "fast_corners.cl" },
    { "derivative", "derivative.cl" },
    { "dilate", "dilate.cl" },
    { "erode", "erode.cl" },
    { "fast_corners", "fast_corners.cl" },
    { "fill_image_borders_constant", "fill_border.cl" },
    { "fill_image_borders_replicate", "fill_border.cl" },
    { "finalize", "optical_flow_pyramid_lk.cl" },
    { "gaussian1x5_sub_x", "gaussian_pyramid.cl" },
    { "gaussian5x1_sub_y", "gaussian_pyramid.cl" },
    { "gemm_accumulate_biases_f16", "gemm.cl" },
    { "gemm_accumulate_biases_f32", "gemm.cl" },
    { "gemm_interleave4x4_8bit", "gemm.cl" },
    { "gemm_interleave4x4_16bit", "gemm.cl" },
    { "gemm_interleave4x4_32bit", "gemm.cl" },
    { "gemm_ma_f16", "gemm.cl" },
    { "gemm_ma_f32", "gemm.cl" },
    { "gemm_mm_u8", "gemm.cl" },
    { "gemm_mm_f16", "gemm.cl" },
    { "gemm_mm_f32", "gemm.cl" },
    { "gemm_vm_f16", "gemm.cl" },
    { "gemm_vm_f32", "gemm.cl" },
    { "gemm_transpose1x16_u8", "gemm.cl" },
    { "gemm_transpose1x8_f16", "gemm.cl" },
    { "gemm_transpose1x4_f32", "gemm.cl" },
    { "harris_score_3x3", "harris_corners.cl" },
    { "harris_score_5x5", "harris_corners.cl" },
    { "harris_score_7x7", "harris_corners.cl" },
    { "hist_border_kernel", "histogram.cl" },
    { "hist_border_kernel_fixed", "histogram.cl" },
    { "hist_local_kernel", "histogram.cl" },
    { "hist_local_kernel_fixed", "histogram.cl" },
    { "hysteresis", "canny.cl" },
    { "im2col_generic", "convolution_layer.cl" },
    { "im2col_reduced", "convolution_layer.cl" },
    { "init_level", "optical_flow_pyramid_lk.cl" },
    { "init_level_max", "optical_flow_pyramid_lk.cl" },
    { "init_level_max_initial_estimate", "optical_flow_pyramid_lk.cl" },
    { "integral_horizontal", "integral_image.cl" },
    { "integral_vertical", "integral_image.cl" },
    { "IYUV_to_NV12_bt709", "color_convert.cl" },
    { "IYUV_to_RGB888_bt709", "color_convert.cl" },
    { "IYUV_to_RGBA8888_bt709", "color_convert.cl" },
    { "IYUV_to_YUV444_bt709", "color_convert.cl" },
    { "lktracker_stage0", "optical_flow_pyramid_lk.cl" },
    { "lktracker_stage1", "optical_flow_pyramid_lk.cl" },
    { "magnitude_phase", "magnitude_phase.cl" },
    { "mean_stddev_accumulate", "mean_stddev.cl" },
    { "minmax", "minmaxloc.cl" },
    { "minmax_border", "minmaxloc.cl" },
    { "minmaxloc", "minmaxloc.cl" },
    { "non_linear_filter_box3x3", "non_linear_filter3x3.cl" },
    { "non_linear_filter_cross3x3", "non_linear_filter3x3.cl" },
    { "non_linear_filter_disk3x3", "non_linear_filter3x3.cl" },
    { "non_linear_filter_box5x5", "non_linear_filter5x5.cl" },
    { "non_linear_filter_cross5x5", "non_linear_filter5x5.cl" },
    { "non_linear_filter_disk5x5", "non_linear_filter5x5.cl" },
    { "non_max_suppression", "nonmax.cl" },
    { "normalization_layer_cross_map", "normalization_layer.cl" },
    { "normalization_layer_in_map", "normalization_layer.cl" },
    { "NV12_to_IYUV_bt709", "color_convert.cl" },
    { "NV12_to_RGB888_bt709", "color_convert.cl" },
    { "NV12_to_RGBA8888_bt709", "color_convert.cl" },
    { "NV12_to_YUV444_bt709", "color_convert.cl" },
    { "NV21_to_IYUV_bt709", "color_convert.cl" },
    { "NV21_to_RGB888_bt709", "color_convert.cl" },
    { "NV21_to_RGBA8888_bt709", "color_convert.cl" },
    { "NV21_to_YUV444_bt709", "color_convert.cl" },
    { "pixelwise_mul_float", "pixelwise_mul_float.cl" },
    { "pixelwise_mul_int", "pixelwise_mul_int.cl" },
    { "pooling_layer_2", "pooling_layer.cl" },
    { "pooling_layer_3", "pooling_layer.cl" },
    { "remap_nearest_neighbour", "remap.cl" },
    { "remap_bilinear", "remap.cl" },
    { "reshape_to_columns", "convolution_layer.cl" },
    { "RGB888_to_IYUV_bt709", "color_convert.cl" },
    { "RGB888_to_NV12_bt709", "color_convert.cl" },
    { "RGB888_to_RGBA8888_bt709", "color_convert.cl" },
    { "RGB888_to_YUV444_bt709", "color_convert.cl" },
    { "RGBA8888_to_IYUV_bt709", "color_convert.cl" },
    { "RGBA8888_to_NV12_bt709", "color_convert.cl" },
    { "RGBA8888_to_RGB888_bt709", "color_convert.cl" },
    { "RGBA8888_to_YUV444_bt709", "color_convert.cl" },
    { "scale_nearest_neighbour", "scale.cl" },
    { "scale_bilinear", "scale.cl" },
    { "scharr3x3", "scharr_filter.cl" },
    { "sobel3x3", "sobel_filter.cl" },
    { "sobel_separable5x1", "sobel_filter.cl" },
    { "sobel_separable1x5", "sobel_filter.cl" },
    { "sobel_separable7x1", "sobel_filter.cl" },
    { "sobel_separable1x7", "sobel_filter.cl" },
    { "softmax_layer_max", "softmax_layer.cl" },
    { "softmax_layer_shift_exp_sum", "softmax_layer.cl" },
    { "softmax_layer_norm", "softmax_layer.cl" },
    { "suppress_non_maximum", "canny.cl" },
    { "tablelookup_U8", "tablelookup.cl" },
    { "tablelookup_S16", "tablelookup.cl" },
    { "threshold_binary", "threshold.cl" },
    { "threshold_range", "threshold.cl" },
    { "transpose", "transpose.cl" },
    { "UYVY422_to_IYUV_bt709", "color_convert.cl" },
    { "UYVY422_to_NV12_bt709", "color_convert.cl" },
    { "UYVY422_to_RGB888_bt709", "color_convert.cl" },
    { "UYVY422_to_RGBA8888_bt709", "color_convert.cl" },
    { "warp_affine_nearest_neighbour", "warp_affine.cl" },
    { "warp_affine_bilinear", "warp_affine.cl" },
    { "warp_perspective_nearest_neighbour", "warp_perspective.cl" },
    { "warp_perspective_bilinear", "warp_perspective.cl" },
    { "YUYV422_to_IYUV_bt709", "color_convert.cl" },
    { "YUYV422_to_NV12_bt709", "color_convert.cl" },
    { "YUYV422_to_RGB888_bt709", "color_convert.cl" },
    { "YUYV422_to_RGBA8888_bt709", "color_convert.cl" },
};

const std::map<std::string, std::string> CLKernelLibrary::_program_source_map =
{
#ifdef EMBEDDED_KERNELS
    {
        "absdiff.cl",
#include "./cl_kernels/absdiff.clembed"
    },
    {
        "accumulate.cl",
#include "./cl_kernels/accumulate.clembed"
    },
    {
        "activation_layer.cl",
#include "./cl_kernels/activation_layer.clembed"
    },
    {
        "arithmetic_op.cl",
#include "./cl_kernels/arithmetic_op.clembed"
    },
    {
        "bitwise_op.cl",
#include "./cl_kernels/bitwise_op.clembed"
    },
    {
        "canny.cl",
#include "./cl_kernels/canny.clembed"
    },
    {
        "channel_combine.cl",
#include "./cl_kernels/channel_combine.clembed"
    },
    {
        "channel_extract.cl",
#include "./cl_kernels/channel_extract.clembed"
    },
    {
        "color_convert.cl",
#include "./cl_kernels/color_convert.clembed"
    },
    {
        "convolution3x3.cl",
#include "./cl_kernels/convolution3x3.clembed"
    },
    {
        "convolution5x5.cl",
#include "./cl_kernels/convolution5x5.clembed"
    },
    {
        "convolution7x7.cl",
#include "./cl_kernels/convolution7x7.clembed"
    },
    {
        "convolution9x9.cl",
#include "./cl_kernels/convolution9x9.clembed"
    },
    {
        "convolution_layer.cl",
#include "./cl_kernels/convolution_layer.clembed"
    },
    {
        "convolution_rectangle.cl",
#include "./cl_kernels/convolution_rectangle.clembed"
    },
    {
        "depth_convert.cl",
#include "./cl_kernels/depth_convert.clembed"
    },
    {
        "derivative.cl",
#include "./cl_kernels/derivative.clembed"
    },
    {
        "dilate.cl",
#include "./cl_kernels/dilate.clembed"
    },
    {
        "erode.cl",
#include "./cl_kernels/erode.clembed"
    },
    {
        "fast_corners.cl",
#include "./cl_kernels/fast_corners.clembed"
    },
    {
        "fill_border.cl",
#include "./cl_kernels/fill_border.clembed"
    },
    {
        "gaussian_pyramid.cl",
#include "./cl_kernels/gaussian_pyramid.clembed"
    },
    {
        "gemm.cl",
#include "./cl_kernels/gemm.clembed"
    },
    {
        "harris_corners.cl",
#include "./cl_kernels/harris_corners.clembed"
    },
    {
        "helpers.h",
#include "./cl_kernels/helpers.hembed"
    },
    {
        "histogram.cl",
#include "./cl_kernels/histogram.clembed"
    },
    {
        "integral_image.cl",
#include "./cl_kernels/integral_image.clembed"
    },
    {
        "magnitude_phase.cl",
#include "./cl_kernels/magnitude_phase.clembed"
    },
    {
        "mean_stddev.cl",
#include "./cl_kernels/mean_stddev.clembed"
    },
    {
        "minmaxloc.cl",
#include "./cl_kernels/minmaxloc.clembed"
    },
    {
        "non_linear_filter3x3.cl",
#include "./cl_kernels/non_linear_filter3x3.clembed"
    },
    {
        "non_linear_filter5x5.cl",
#include "./cl_kernels/non_linear_filter5x5.clembed"
    },
    {
        "non_linear_filter_helpers.h",
#include "./cl_kernels/non_linear_filter_helpers.hembed"
    },
    {
        "nonmax.cl",
#include "./cl_kernels/nonmax.clembed"
    },
    {
        "normalization_layer.cl",
#include "./cl_kernels/normalization_layer.clembed"
    },
    {
        "optical_flow_pyramid_lk.cl",
#include "./cl_kernels/optical_flow_pyramid_lk.clembed"
    },
    {
        "pixelwise_mul_float.cl",
#include "./cl_kernels/pixelwise_mul_float.clembed"
    },
    {
        "pixelwise_mul_int.cl",
#include "./cl_kernels/pixelwise_mul_int.clembed"
    },
    {
        "pooling_layer.cl",
#include "./cl_kernels/pooling_layer.clembed"
    },
    {
        "remap.cl",
#include "./cl_kernels/remap.clembed"
    },
    {
        "scale.cl",
#include "./cl_kernels/scale.clembed"
    },
    {
        "scharr_filter.cl",
#include "./cl_kernels/scharr_filter.clembed"
    },
    {
        "sobel_filter.cl",
#include "./cl_kernels/sobel_filter.clembed"
    },
    {
        "softmax_layer.cl",
#include "./cl_kernels/softmax_layer.clembed"
    },
    {
        "tablelookup.cl",
#include "./cl_kernels/tablelookup.clembed"
    },
    {
        "threshold.cl",
#include "./cl_kernels/threshold.clembed"
    },
    {
        "transpose.cl",
#include "./cl_kernels/transpose.clembed"
    },
    {
        "types.h",
#include "./cl_kernels/types.hembed"
    },
    {
        "warp_affine.cl",
#include "./cl_kernels/warp_affine.clembed"
    },
    {
        "warp_helpers.h",
#include "./cl_kernels/warp_helpers.hembed"
    },
    {
        "warp_perspective.cl",
#include "./cl_kernels/warp_perspective.clembed"
    }
#endif
};

CLKernelLibrary::CLKernelLibrary()
    : _context(), _device(), _kernel_path("."), _programs_map(), _built_programs_map()
{
}

CLKernelLibrary &CLKernelLibrary::get()
{
    static CLKernelLibrary _kernel_library;
    return _kernel_library;
}

Kernel CLKernelLibrary::create_kernel(const std::string &kernel_name, const StringSet &build_options_set) const
{
    // Find which program contains the kernel
    auto kernel_program_it = _kernel_program_map.find(kernel_name);

    if(_kernel_program_map.end() == kernel_program_it)
    {
        ARM_COMPUTE_ERROR("Kernel %s not found in the CLKernelLibrary", kernel_name.c_str());
    }

    // Check if the program has been built before with same build options.
    const std::string program_name       = kernel_program_it->second;
    const std::string build_options      = stringify_set(build_options_set);
    const std::string built_program_name = program_name + "_" + build_options;
    auto              built_program_it   = _built_programs_map.find(built_program_name);

    cl::Program cl_program;

    if(_built_programs_map.end() != built_program_it)
    {
        // If program has been built, retrieve to create kernel from it
        cl_program = built_program_it->second;
    }
    else
    {
        // Get program
        Program program = load_program(program_name);

        // Build program
        cl_program = program.build(build_options);

        // Add built program to internal map
        _built_programs_map.emplace(built_program_name, cl_program);
    }

    // Create and return kernel
    return Kernel(kernel_name, cl_program);
}

const Program &CLKernelLibrary::load_program(const std::string &program_name) const
{
    const auto program_it = _programs_map.find(program_name);

    if(program_it != _programs_map.end())
    {
        return program_it->second;
    }

    Program program;

#ifdef EMBEDDED_KERNELS
    const auto program_source_it = _program_source_map.find(program_name);

    if(_program_source_map.end() == program_source_it)
    {
        ARM_COMPUTE_ERROR("Embedded program for %s does not exist.", program_name.c_str());
    }

    program = Program(_context, program_name, program_source_it->second);
#else
    // Check for binary
    std::string source_name = _kernel_path + program_name;
    std::string binary_name = source_name + "bin";

    if(std::ifstream(binary_name).is_open())
    {
        const std::string program_binary = read_file(binary_name, true);
        program                          = Program(_context, _device, program_name, std::vector<unsigned char>(program_binary.begin(), program_binary.end()));
    }
    else if(std::ifstream(source_name).is_open())
    {
        program = Program(_context, program_name, read_file(source_name, false));
    }
    else
    {
        ARM_COMPUTE_ERROR("Kernel file %s does not exist.", source_name.c_str());
    }
#endif

    // Insert program to program map
    const auto new_program = _programs_map.emplace(program_name, std::move(program));

    return new_program.first->second;
}

std::string CLKernelLibrary::stringify_set(const StringSet &s) const
{
    std::string concat_set = "-cl-arm-non-uniform-work-group-size ";

#ifndef EMBEDDED_KERNELS
    concat_set += "-I" + _kernel_path + " ";
#endif /* EMBEDDED_KERNELS */

    // Concatenate set
    for(const auto &el : s)
    {
        concat_set += " " + el;
    }

    return concat_set;
}
