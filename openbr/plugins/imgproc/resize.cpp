/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2012 The MITRE Corporation                                      *
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

#include <opencv2/imgproc/imgproc.hpp>

#include <openbr/plugins/openbr_internal.h>

using namespace cv;

namespace br
{

/*!
 * \ingroup transforms
 * \brief Resize the template
 * \author Josh Klontz \cite jklontz
 * \br_property enum method Resize method. Good options are: [Area should be used for shrinking an image, Cubic for slow but accurate enlargment, Bilin for fast enlargement]
 * \br_property bool preserveAspect If true, the image will be sized per specification, but a border will be applied to preserve aspect ratio.
 */
class ResizeTransform : public UntrainableTransform
{
    Q_OBJECT
    Q_ENUMS(Method)

public:
    /*!< */
    enum Method { Near = INTER_NEAREST,
                  Area = INTER_AREA,
                  Bilin = INTER_LINEAR,
                  Cubic = INTER_CUBIC,
                  Lanczo = INTER_LANCZOS4};

private:
    Q_PROPERTY(int rows READ get_rows WRITE set_rows RESET reset_rows STORED false)
    Q_PROPERTY(int columns READ get_columns WRITE set_columns RESET reset_columns STORED false)
    Q_PROPERTY(Method method READ get_method WRITE set_method RESET reset_method STORED false)
    Q_PROPERTY(bool preserveAspect READ get_preserveAspect WRITE set_preserveAspect RESET reset_preserveAspect STORED false)
    Q_PROPERTY(bool pad READ get_pad WRITE set_pad RESET reset_pad STORED false)
    BR_PROPERTY(int, rows, -1)
    BR_PROPERTY(int, columns, -1)
    BR_PROPERTY(Method, method, Bilin)
    BR_PROPERTY(bool, preserveAspect, false)
    BR_PROPERTY(bool, pad, true)

    void project(const Template &src, Template &dst) const
    {
        if ((rows == -1) && (columns == -1)) {
            dst = src;
            return;
        }

        if (!preserveAspect) {
            resize(src, dst, Size((columns == -1) ? src.m().cols*rows/src.m().rows : columns, rows), 0, 0, method);
            const float rowScaleFactor = (float)rows/src.m().rows;
            const float colScaleFactor = (columns == -1) ? rowScaleFactor : (float)columns/src.m().cols;
            QList<QPointF> points = src.file.points();
            for (int i=0; i<points.size(); i++)
                points[i] = QPointF(points[i].x() * colScaleFactor,points[i].y() * rowScaleFactor);
            dst.file.setPoints(points);
        } else if (!pad) {
            const int size = std::max(rows, columns);
            float ratio = (float) src.m().rows / src.m().cols;
            if (src.m().rows > src.m().cols)
                resize(src, dst, Size(size/ratio, size), 0, 0, method);
            else
                resize(src, dst, Size(size, size*ratio), 0, 0, method);
        } else {
            float inRatio = (float) src.m().rows / src.m().cols;
            float outRatio = (float) rows / columns;
            dst = Mat::zeros(rows, columns, src.m().type());
            if (inRatio < outRatio) {
                int columnOffset = (src.m().cols - (src.m().cols / outRatio * inRatio)) /2;
                Mat buffer;
                src.m().copyTo(buffer);
                //Rect (col_start, r_start, c_width, r_wdith)
                buffer(Rect(columnOffset,0, src.m().cols - columnOffset*2,src.m().rows)).copyTo(buffer);
                resize(buffer,dst.m(),Size(columns, rows), 0, 0, method);
            } else if (inRatio > outRatio) {
                int rowOffset = (src.m().rows - (src.m().rows * outRatio / inRatio)) /2;
                Mat buffer;
                src.m().copyTo(buffer);
                buffer(Rect(0,rowOffset, src.m().cols,src.m().rows - rowOffset*2)).copyTo(buffer);
                resize(buffer,dst.m(),Size(columns, rows), 0, 0, method);
            } else {
                resize(src.m(),dst.m(),Size(columns, rows), 0, 0, method);
            }

        }
    }
};

BR_REGISTER(Transform, ResizeTransform)

} // namespace br

#include "imgproc/resize.moc"
