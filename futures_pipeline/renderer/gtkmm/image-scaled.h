#pragma once

#include <gtkmm.h>

class ImageScaled : public Gtk::ScrolledWindow
{

public:

    ImageScaled();
    ~ImageScaled();

    void SetAspectRatio(int width, int height);

    int GetImageWidth();
    int GetImageHeight();

protected :

    void on_size_allocate(Gtk::Allocation& allocation) override;

private :

    void Rescale(int view_width, int view_height);

    int view_width_, view_height_;

    float image_aspect_ratio_;

    int image_width_, image_height_;
};
