// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
#if __AVX__
#include "avx_activation.h"
#endif // __AVX__

#include "swish_x86.h"

#include <math.h>

namespace ncnn {

Swish_x86::Swish_x86()
{
    support_packing = true;
}

int Swish_x86::forward_inplace(Mat& bottom_top_blob, const Option& opt) const
{
    int w = bottom_top_blob.w;
    int h = bottom_top_blob.h;
    int channels = bottom_top_blob.c;
    int size = w * h;
    int elempack = bottom_top_blob.elempack;

#if __AVX__
    if (elempack == 8)
    {
        #pragma omp parallel for num_threads(opt.num_threads)
        for (int q = 0; q < channels; q++)
        {
            float* ptr = bottom_top_blob.channel(q);

            for (int i = 0; i < size; i++)
            {
                __m256 _p = _mm256_loadu_ps(ptr);
                _mm256_storeu_ps(ptr, _mm256_mul_ps(_p, sigmoid_avx(_p)));
                ptr += 8;
            }
        }

        return 0;
    }
#endif // __AVX__

    if (elempack == 4)
    {
        // TODO implement pack4
        Mat bottom_top_blob_unpacked;

        Option opt_pack = opt;
        opt_pack.blob_allocator = opt.workspace_allocator;
        convert_packing(bottom_top_blob, bottom_top_blob_unpacked, 1, opt_pack);

        bottom_top_blob = bottom_top_blob_unpacked;

        return forward_inplace(bottom_top_blob, opt);
    }

    #pragma omp parallel for num_threads(opt.num_threads)
    for (int q = 0; q < channels; q++)
    {
        float* ptr = bottom_top_blob.channel(q);

#if __AVX__
        int nn = size >> 3;
        int remain = size & 7;
#else
        int remain = size;
#endif // __AVX__

#if __AVX__
        for (; nn > 0; nn--)
        {
            __m256 _p = _mm256_loadu_ps(ptr);
            _mm256_storeu_ps(ptr, _mm256_mul_ps(_p, sigmoid_avx(_p)));
            ptr += 8;
        }
#endif // __AVX__
        for (; remain > 0; remain--)
        {
            *ptr = *ptr / (1.f + exp(-*ptr));
            ptr++;
        }
    }

    return 0;
}

} // namespace ncnn
