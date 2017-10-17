#include <opencv2/imgproc/imgproc.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>
#include <dlib/opencv.h>

#include "openbr/plugins/openbr_internal.h"

#include <QTemporaryFile>

using namespace std;
using namespace dlib;

namespace br
{

/*!
 * \ingroup transforms
 * \brief Wrapper to dlib's landmarker.
 * \author Scott Klum \cite sklum
 */
class DLibShapeResourceMaker : public ResourceMaker<shape_predictor>
{

private:
    shape_predictor *make() const
    {
        shape_predictor *sp = new shape_predictor();
        dlib::deserialize(qPrintable(Globals->sdkPath + "/share/openbr/models/dlib/shape_predictor_68_face_landmarks.dat")) >> *sp;
        return sp;
    }
};

class DLandmarkerTransform : public UntrainableTransform
{
    Q_OBJECT

private:
    Resource<shape_predictor> shapeResource;

    void init()
    {
        shapeResource.setResourceMaker(new DLibShapeResourceMaker());
        shapeResource.release(shapeResource.acquire()); // Pre-load one instance of the model
    }

    QPointF averagePoints(const QList<QPointF> &points, int rangeBegin, int rangeEnd) const
    {
        QPointF point;
        for (int i=rangeBegin; i<rangeEnd; i++)
            point += points[i];
        point /= (rangeEnd-rangeBegin);
        return point;
    }

    void setFacePoints(Template &dst) const
    {
        const QList<QPointF> points = dst.file.points();
        dst.file.set("RightEye", averagePoints(points, 36, 42));
        dst.file.set("LeftEye" , averagePoints(points, 42, 48));
        dst.file.set("Chin", points[8]);
    }

    void project(const Template &src, Template &dst) const
    {
        dst = src;

        shape_predictor *const sp = shapeResource.acquire();

        cv::Mat cvImage = src.m();
        if (cvImage.channels() == 3)
            cv::cvtColor(cvImage, cvImage, CV_BGR2GRAY);

        cv_image<unsigned char> cimg(cvImage);
        array2d<unsigned char> image;
        assign_image(image,cimg);

        rectangle r;
        if (src.file.rects().isEmpty()) { // If the image has no rects assume the whole image is a face
            r = rectangle(0, 0, cvImage.cols, cvImage.rows);
        } else { // Crop the image on the first rect
            const QRectF rect = src.file.rects().first();
            r = rectangle(rect.left(), rect.top(), rect.right(), rect.bottom());
        }

        full_object_detection shape = (*sp)(image, r);
        QList<QPointF> points;
        for (size_t i=0; i<shape.num_parts(); i++)
            points.append(QPointF(shape.part(i)(0), shape.part(i)(1)));
        dst.file.setPoints(points);
        setFacePoints(dst);

        shapeResource.release(sp);
    }
};

BR_REGISTER(Transform, DLandmarkerTransform)

/*!
 * \ingroup transforms
 * \brief Wrapper to dlib's trainable object detector.
 * \author Scott Klum \cite sklum
 */
class DObjectDetectorTransform : public Transform
{
    Q_OBJECT

    Q_PROPERTY(int winSize READ get_winSize WRITE set_winSize RESET reset_winSize STORED true)
    Q_PROPERTY(float C READ get_C WRITE set_C RESET reset_C STORED true)
    Q_PROPERTY(float epsilon READ get_epsilon WRITE set_epsilon RESET reset_epsilon STORED true)
    BR_PROPERTY(int, winSize, 80)
    BR_PROPERTY(float, C, 1)
    BR_PROPERTY(float, epsilon, .01)

private:
    typedef scan_fhog_pyramid<pyramid_down<6> > image_scanner_type;
    mutable object_detector<image_scanner_type> detector;
    mutable QMutex mutex;

    void train(const TemplateList &data)
    {
        dlib::array<array2d<unsigned char> > samples;
        std::vector<std::vector<rectangle> > boxes;

        foreach (const Template &t, data) {
            if (!t.file.rects().isEmpty()) {
                cv_image<unsigned char> cimg(t.m());

                array2d<unsigned char> image;
                assign_image(image,cimg);

                samples.push_back(image);

                std::vector<rectangle> b;
                foreach (const QRectF &r, t.file.rects())
                    b.push_back(rectangle(r.left(),r.top(),r.right(),r.bottom()));

                boxes.push_back(b);
            }
        }

        if (samples.size() == 0)
            qFatal("Training data has no bounding boxes.");

        image_scanner_type scanner;

        scanner.set_detection_window_size(winSize, winSize);
        structural_object_detection_trainer<image_scanner_type> trainer(scanner);
        trainer.set_num_threads(max(1,QThread::idealThreadCount()));
        trainer.set_c(C);
        trainer.set_epsilon(epsilon);

        if (Globals->verbose)
            trainer.be_verbose();

        detector = trainer.train(samples, boxes);
    }

    void project(const Template &src, Template &dst) const
   {
        dst = src;
        cv_image<unsigned char> cimg(src.m());
        array2d<unsigned char> image;
        assign_image(image,cimg);

        QMutexLocker locker(&mutex);
        std::vector<rectangle> dets = detector(image);
        locker.unlock();

        for (size_t i=0; i<dets.size(); i++)
            dst.file.appendRect(QRectF(QPointF(dets[i].left(),dets[i].top()),QPointF(dets[i].right(),dets[i].bottom())));
    }

    void store(QDataStream &stream) const
    {
        // Create local file
        QTemporaryFile tempFile;
        tempFile.open();
        tempFile.close();

        dlib::serialize(qPrintable(tempFile.fileName())) << detector;

        // Copy local file contents to stream
        tempFile.open();
        QByteArray data = tempFile.readAll();
        tempFile.close();
        stream << data;
    }

    void load(QDataStream &stream)
    {
        // Copy local file contents from stream
        QByteArray data;
        stream >> data;

        // Create local file
        QTemporaryFile tempFile(QDir::tempPath()+"/model");
        tempFile.open();
        tempFile.write(data);
        tempFile.close();

        // Load MLP from local file
        dlib::deserialize(qPrintable(tempFile.fileName())) >> detector;
    }
};

BR_REGISTER(Transform, DObjectDetectorTransform)

} // namespace br

#include "dlib.moc"
