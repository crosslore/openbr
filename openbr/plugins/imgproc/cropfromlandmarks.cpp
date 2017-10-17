#include <openbr/plugins/openbr_internal.h>
#include <openbr/core/opencvutils.h>

using namespace cv;

namespace br
{

/*!
 * \ingroup transforms
 * \brief Crops around the landmarks numbers provided.
 * \author Brendan Klare \cite bklare
 * \param padding Percentage of height and width to pad the image.
 */
class CropFromLandmarksTransform : public UntrainableTransform
{
    Q_OBJECT

    Q_PROPERTY(QList<int> indices READ get_indices WRITE set_indices RESET reset_indices STORED false)
    Q_PROPERTY(float paddingHorizontal READ get_paddingHorizontal WRITE set_paddingHorizontal RESET reset_paddingHorizontal STORED false)
    Q_PROPERTY(float paddingVertical READ get_paddingVertical WRITE set_paddingVertical RESET reset_paddingVertical STORED false)
    Q_PROPERTY(bool shiftPoints READ get_shiftPoints WRITE set_shiftPoints RESET reset_shiftPoints STORED false)
    BR_PROPERTY(QList<int>, indices, QList<int>())
    BR_PROPERTY(float, paddingHorizontal, .1)
    BR_PROPERTY(float, paddingVertical, .1)
    BR_PROPERTY(bool, shiftPoints, false)

    void project(const Template &src, Template &dst) const
    {
        QList<int> cropIndices = indices;
        if (cropIndices.isEmpty() && !src.file.points().isEmpty())
            for (int i=0; i<src.file.points().size(); i++)
                cropIndices.append(i);

        int minX = src.m().cols - 1,
            maxX = 1,
            minY = src.m().rows - 1,
            maxY = 1;

        for (int i = 0; i <cropIndices.size(); i++) {
            if (minX > src.file.points()[cropIndices[i]].x())
                minX = src.file.points()[cropIndices[i]].x();
            if (minY > src.file.points()[cropIndices[i]].y())
                minY = src.file.points()[cropIndices[i]].y();
            if (maxX < src.file.points()[cropIndices[i]].x())
                maxX = src.file.points()[cropIndices[i]].x();
            if (maxY < src.file.points()[cropIndices[i]].y())
                maxY = src.file.points()[cropIndices[i]].y();
        }

        int padW = qRound((maxX - minX) * (paddingHorizontal / 2));
        int padH = qRound((maxY - minY) * (paddingVertical / 2));

        QRectF rect(minX - padW, minY - padH, (maxX - minX + 1) + padW * 2, (maxY - minY + 1) + padH * 2);
        if (rect.x() < 0) rect.setX(0);
        if (rect.y() < 0) rect.setY(0);
        if (rect.x() + rect.width()  > src.m().cols) rect.setWidth (src.m().cols - rect.x());
        if (rect.y() + rect.height() > src.m().rows) rect.setHeight(src.m().rows - rect.y());

        if (shiftPoints) {
            QList<QPointF> points = src.file.points();
            for (int i=0; i<points.size(); i++)
                points[i] -= rect.topLeft();
            dst.file.setPoints(points);
        }

        dst = Mat(src, OpenCVUtils::toRect(rect));
    }
};

BR_REGISTER(Transform, CropFromLandmarksTransform)

} // namespace br

#include "imgproc/cropfromlandmarks.moc"
