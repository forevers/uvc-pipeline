#include "image-scaled.h"

#include <iostream>

using namespace std;


ImageScaled::ImageScaled() :
    view_width_{0},
    view_height_{0},
    image_aspect_ratio_{1.0f},
    image_width_{1},
    image_height_{1}
{
    /* only show scrollbars when necessary */
    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    set_has_window(true);
}


ImageScaled::~ImageScaled()
{
}


void ImageScaled::on_size_allocate(Gtk::Allocation& allocation)
{
    int width = allocation.get_width();
    int height = allocation.get_height();

    if (width != view_width_ || height != view_height_) {

        view_width_ = width;
        view_height_ = height;
        cout << "view size : " << width << " x " << height << endl;
        Rescale(view_width_, view_height_);
    }

    Gtk::ScrolledWindow::on_size_allocate(allocation);
}


void ImageScaled::Rescale(int view_width, int view_height)
{
    int test_height = view_width / image_aspect_ratio_;
    if (test_height < view_height) {
        image_width_ = view_width;
        image_height_ = test_height;
    } else {
        image_width_ = view_height * image_aspect_ratio_;
        image_height_ = view_height;
    }

    cout << "Rescale () image size : " << image_width_ << " x " << image_height_ << endl;
}


void ImageScaled::SetAspectRatio(int width, int height)
{
    cout << "SetAspectRatio : " << width << " x " << height << endl;
    image_aspect_ratio_ = (float)width / height;
    Rescale(view_width_, view_height_);
}

int ImageScaled::GetImageWidth()
{
    return image_width_;
}


int ImageScaled::GetImageHeight()
{
    return image_height_;
}
