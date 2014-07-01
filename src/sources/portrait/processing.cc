#include "portrait/processing.hh"

#include <utility>

#include "portrait/exception.hh"
#include "portrait/algorithm.hh"
#include "portrait/facedetect.hh"

namespace portrait {

cv::Mat PortraitProcessAll(
    cv::Mat&& photo,
    const int face_resize_to,
    const cv::Size& portrait_size,
    const int VerticalOffset,
    const cv::Vec3b& back_color)
{
    const SemiData semi = PortraitProcessSemi(std::move(photo), face_resize_to);
    return PortraitMix(semi, portrait_size, VerticalOffset, back_color);
}

struct SemiDataImpl
{
public:
    cv::Mat image; //CV_8UC3 R,G,B
    cv::Mat raw; //CV_8UC4 R,G,B,A
    cv::Rect face_area;
}; //struct SemiDataImpl

SemiData::SemiData() throw()
    : _data(nullptr)
{ }

SemiData::SemiData(void* data) throw()
    : _data(data)
{ }

SemiData::SemiData(SemiData&& another) throw()
    : _data(nullptr)
{
    Swap(another);
}

SemiData::~SemiData() throw()
{
    delete (SemiDataImpl*)_data;
}

SemiData& SemiData::operator=(SemiData&& another) throw()
{
    Swap(another);
    return *this;
}

void SemiData::Swap(SemiData& another) throw()
{
    std::swap(_data, another._data);
}

void* SemiData::Get() const throw()
{
    return _data;
}

cv::Mat SemiData::GetImage() const
{
    return ((SemiDataImpl*)_data)->image;
}
cv::Mat SemiData::GetImageWithLines() const
{
    cv::Mat tmp;
    ((SemiDataImpl*)_data)->image.copyTo(tmp);
    DrawGrabCutLines(tmp, ((SemiDataImpl*)_data)->face_area);
    return tmp;
}

SemiData PortraitProcessSemi(
    cv::Mat&& photo, //CV_8UC3
    const int face_resize_to)
{
    SemiData semi(new SemiDataImpl());
    SemiDataImpl& data = *(SemiDataImpl*)semi.Get();
    data.image = photo;
    photo = cv::Mat();

    data.face_area = DetectSingleFace(data.image);
    data.face_area = TryCutPortrait(data.image, data.face_area, 0.6, 0.6, 0.4);
    data.face_area = ResizeFace(data.image, data.face_area,
                                cv::Size(face_resize_to, face_resize_to));
    data.raw = GetFrontBackMask(data.image, data.face_area);

    return semi;
}

cv::Mat PortraitMix(
    const SemiData& semi,
    const cv::Size& portrait_size,
    const int VerticalOffset,
    const cv::Vec3b& back_color)
{
    SemiDataImpl& data = *(SemiDataImpl*)semi.Get();
    //裁剪
    cv::Point face_center = CenterOf(data.face_area);
    cv::Rect cut_area(face_center.x - portrait_size.width / 2,
                      face_center.y - portrait_size.height / 2
                          + VerticalOffset,
                      portrait_size.width,
                      portrait_size.height);
    if (!Inside(cut_area, data.image))
        throw Error(OutOfRange);

    //混合
    cv::Mat mix = Mix(data.image(cut_area), data.raw(cut_area), back_color);

    return mix;
}


}
