// some gtkmm references ...
// papers.harvie.cz/unsorted/programming-with_gtkmm.pdf
// https://developer.gnome.org/gtkmm-tutorial/stable/

#include "render-ui.h"
#include <gtkmm/application.h>


int main(int argc, char* argv[])
{
    auto app = Gtk::Application::create(argc, argv);

    RenderUI render_ui;

    // Shows the window and returns when it is closed.
    return app->run(render_ui);
}
