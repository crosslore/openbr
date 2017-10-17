/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2016 Li Li, Colin Heinzmann                                     *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <iostream>
using namespace std;
#include <unistd.h>

#include <opencv2/opencv.hpp>
using namespace cv;

#include <openbr/plugins/openbr_internal.h>

// definitions from the CUDA source file
namespace br { namespace cuda { namespace cvtfloat {
  void wrapper(void* src, void** dst, int rows, int cols);
}}}

namespace br
{

/*!
 * \ingroup transforms
 * \brief Converts 8-bit images currently on GPU into 32-bit floating point equivalent.
 * \author Colin Heinzmann \cite DepthDeluxe
 */
class CUDACvtFloatTransform : public UntrainableTransform
{
    Q_OBJECT

  public:
    void project(const Template &src, Template &dst) const
    {
      void* const* srcDataPtr = src.m().ptr<void*>();
      int rows = *((int*)srcDataPtr[1]);
      int cols = *((int*)srcDataPtr[2]);
      int type = *((int*)srcDataPtr[3]);

      // assume the image type is 256-monochrome
      // TODO(colin): real exception handling
      if (type != CV_8UC1) {
        cout << "ERR: Invalid memory format" << endl;
        return;
      }

      // build the destination mat
      Mat dstMat = Mat(src.m().rows, src.m().cols, src.m().type());
      void** dstDataPtr = dstMat.ptr<void*>();
      dstDataPtr[1] = srcDataPtr[1];
      dstDataPtr[2] = srcDataPtr[2];
      dstDataPtr[3] = srcDataPtr[3]; *((int*)dstDataPtr[3]) = CV_32FC1;

      cuda::cvtfloat::wrapper(srcDataPtr[0], &dstDataPtr[0], rows, cols);
      dst = dstMat;
    }
};

BR_REGISTER(Transform, CUDACvtFloatTransform)

} // namespace br

#include "cuda/cudacvtfloat.moc"
