#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include <iostream>
#include <unistd.h> 
#include <complex>
#include <vector>
#include <algorithm>

Display* the_display = NULL;
Window the_main_window;
GC the_main_gc;
XColor the_colors[32];
bool the_touch_is_idle = true;
size_t the_drawing_size = 1;
enum WorkingMode {
    Draw,
    Line,
    Dot,
} the_working_mode;

#define XA_TOUCH ((Atom) XA_LAST_PREDEFINED + 1)
typedef enum {
    TOUCH_UP,
    TOUCH_DOWN,
    TOUCH_DELTA
} touch_phase_t;

template <typename T>
class Point : public std::complex<T>
{
public:
    Point<T>(T x = static_cast<T>(0), T y = static_cast<T>(0))
        :std::complex<T>(x, y)
    {};
public:
    T x() const { return std::complex<T>::real(); };
    T y() const { return std::complex<T>::imag(); };
    void x(const T& x) { std::complex<T>::real(x); };
    void y(const T& y) { std::complex<T>::imag(y); };
};

template <typename T>
class Shift : public std::complex<T>
{
public:
    Shift<T>(T dx = static_cast<T>(0), T dy = static_cast<T>(0))
        : std::complex<T>(dx, dy)
    {};
    Shift<T>(const std::complex<T>& complex)
        : std::complex<T>(complex)
    {};
public:
    T dx() const { return std::complex<T>::real(); };
    T dy() const { return std::complex<T>::imag(); };
    void dx(const T& dx) { return std::complex<T>::real(dx); };
    void dy(const T& dy) { return std::complex<T>::imag(dy); };
}; 


class Brush
{
public:
    size_t _id;
    std::vector< Point<double> > _loci;
    size_t _step;

public:
    Brush(size_t id, const Point<double>& location)
        : _id(id)
        , _loci(1, location)
        , _step(0)
    {};

public:
    size_t id() const { return _id; };
    void draw(Window win, GC gc, bool from_scratch)
    {
        if (!from_scratch && _step >= _loci.size())
            return;

        XWindowAttributes win_attributes = { 0 };
        XGetWindowAttributes(the_display, win, &win_attributes);
        std::vector<XPoint> xpoints;
        typedef std::vector< Point<double> >::const_iterator const_iterator; 
        const_iterator first = (!from_scratch && _step > 0) ? _loci.begin() + _step - 1 : _loci.begin();
        for (const_iterator it = first; it != _loci.end(); ++it) {
            XPoint xpoint = { it->x() * win_attributes.width, it->y() * win_attributes.height };
            xpoints.push_back(xpoint);
        }
        _step = from_scratch ? 0 : _loci.size();

        if (xpoints.empty())
            return; 
        XSetForeground(the_display, gc, the_colors[_id % 32].pixel);
        if (the_working_mode == Dot) {
            XDrawPoints(the_display, win, gc, &xpoints[0], xpoints.size(), CoordModeOrigin);
            std::vector<XArc> xarcs;
            for (size_t i = 0; i != xpoints.size(); ++i) {
                XArc xarc = { xpoints[i].x, xpoints[i].y, the_drawing_size, the_drawing_size, 0, 360 * 64 };
                xarcs.push_back(xarc);
            }
            XFillArcs(the_display, win, gc, &xarcs[0], xarcs.size());
        }
        else {
            XSetLineAttributes(the_display, gc, the_drawing_size, LineSolid, CapRound, JoinRound);
            XDrawLines(the_display, win, gc, &xpoints[0], xpoints.size(), CoordModeOrigin);
        }
    };
    
public:
    Brush& operator += (const Brush& other)
    {
        _loci.insert(_loci.end(), other._loci.begin(), other._loci.end());
        return *this;
    };
};

bool operator == (const Brush& left, const Brush& right)
{
    return left.id() == right.id();
};

bool operator != (const Brush& left, const Brush& right)
{
    return left.id() != right.id();
};

typedef struct xinput2_spec {
    Bool is_enabled;
    int opcode;
    int event_base;
    int error_base;
    bool is_touch_supported;
} xinput2_spec_t;

xinput2_spec_t the_xinput2_spec = { 0 };

std::vector<Brush> the_drawing_brushes;
std::vector<Brush> the_finished_brushes;

static void send_touch_event(size_t id, touch_phase_t phase, long x, long y)
{
    XClientMessageEvent xevent; 
    xevent.type = ClientMessage; 
    xevent.message_type = XA_TOUCH; 
    xevent.format = 32;
    xevent.data.l[0] = id; 
    xevent.data.l[1] = phase; 
    xevent.data.l[2] = x; 
    xevent.data.l[3] = y; 
    XSendEvent(the_display, the_main_window, False, 0, (XEvent*)&xevent);
}

static bool on_touch(XClientMessageEvent* xevent)
{
    int screen = DefaultScreen(the_display);
    const int width = DisplayWidth(the_display, screen);
    const int height = DisplayHeight(the_display, screen); 
    size_t id = xevent->data.l[0];
    double x = xevent->data.l[2] / static_cast<double>(width);
    double y = xevent->data.l[3] / static_cast<double>(height);
    Brush brush(id, Point<double>(x, y));
    touch_phase_t phase = static_cast<touch_phase_t>(xevent->data.l[1]);
    if (phase == TOUCH_UP) {
        std::vector<Brush>::iterator it = std::find(the_drawing_brushes.begin(), the_drawing_brushes.end(), brush);
        if (it != the_drawing_brushes.end()) {
            the_finished_brushes.push_back(*it);
            the_drawing_brushes.erase(it);
        }
    }
    else if (phase == TOUCH_DELTA) {
        for (std::vector<Brush>::iterator it = the_drawing_brushes.begin(); it != the_drawing_brushes.end(); ++it) {
            if (*it == brush) {
                *it += brush;
                break;
            }
        }
    }
    else if (phase == TOUCH_DOWN) {
        if (the_drawing_brushes.empty() && the_working_mode != Draw) {
            std::vector<Brush> empty;
            std::swap(the_finished_brushes, empty);
            XClearArea(the_display, the_main_window, 0, 0, 0, 0, true);
        }
        the_drawing_brushes.push_back(brush);
    }
    the_touch_is_idle = the_drawing_brushes.empty();
    XEvent event = { 0 };
    event.type = Expose;
    event.xexpose.window = the_main_window;
    XSendEvent(the_display, the_main_window, False, ExposureMask, &event);
    XFlush(the_display);
    return true;
}

static void on_map_notify(XEvent* event)
{
    if (!the_xinput2_spec.is_touch_supported) {
        std::cout << "xinput is not support for touch, use touchwin-sdk instead of" << std::endl;
    }
}

static void on_expose(XEvent* event)
{
    if (event->xexpose.count > 0)
        return;

    bool from_scratch = !event->xexpose.send_event;
    if (from_scratch) {
        for (std::vector<Brush>::iterator it = the_finished_brushes.begin(); it != the_finished_brushes.end(); ++it) {
            it->draw(event->xexpose.window, the_main_gc, from_scratch);
        }
    }
    for (std::vector<Brush>::iterator it = the_drawing_brushes.begin(); it != the_drawing_brushes.end(); ++it) {
        it->draw(event->xexpose.window, the_main_gc, from_scratch);
    }
}

static bool on_key_release(XEvent* event)
{
    int key = XLookupKeysym(&event->xkey, 0);
    if (key == XK_Escape) {
        return false;
    }
    else if (key == XK_d) {
        the_working_mode = Draw;
    }
    else if (key == XK_l) {
        the_working_mode = Line;
    }
    else if (key == XK_p) {
        the_working_mode = Dot;
    }
    else if (key == XK_h) {
        ++the_drawing_size;
    }
    else if (key == XK_t) {
        if (the_drawing_size > 1)
            --the_drawing_size;
    }
    else if (key == XK_space) {
        std::vector<Brush> empty[2];
        std::swap(the_drawing_brushes, empty[0]);
        std::swap(the_finished_brushes, empty[1]);
        XClearArea(the_display, the_main_window, 0, 0, 0, 0, true);
    }
    return true;
}

static bool on_xi_event(XIDeviceEvent* xievent)
{
    const int id = xievent->detail;
    const int x = xievent->event_x;
    const int y = xievent->event_y;
    touch_phase_t phase;

    switch (xievent->evtype) {
    case XI_TouchBegin:
        phase = TOUCH_DOWN;
        break;
    case XI_TouchUpdate:
        phase = TOUCH_DELTA;
        break;
    case XI_TouchEnd:
        phase = TOUCH_UP;
        break;
    default:
        return true;
    } 
    send_touch_event(id, phase, x, y);
    return true;
}

static bool on_generic_event(XGenericEventCookie* xcookie)
{
    if (xcookie->extension == the_xinput2_spec.opcode) {
        return on_xi_event((XIDeviceEvent*)xcookie->data);
    }
    return true;
}

static bool on_event(Window win, XEvent* event)
{ 
    XGenericEventCookie *cookie = &event->xcookie;
    if (XGetEventData(the_display, cookie) && cookie->type == GenericEvent) {
        return on_generic_event(cookie);
    }
    XFreeEventData(the_display, cookie); 

    switch (event->type) {
    case MapNotify:
        on_map_notify(event);
        break; 
    case Expose:
        on_expose(event);
        break;
    case ButtonPress:
        // fallthrough intended
    case ButtonRelease:
        break;
    case MotionNotify:
        break;
    case KeyPress:
        break;
    case KeyRelease:
        return on_key_release(event);
    case PropertyNotify:
        break;
    case EnterNotify:
    case LeaveNotify:
        break;
    case SelectionClear:
        break;
    case ClientMessage:
        if (reinterpret_cast<XClientMessageEvent*>(event)->message_type == XA_TOUCH)
            return on_touch(reinterpret_cast<XClientMessageEvent*>(event));
        else
            return false;
    default:
        break;
    } 
    return true;
};

static void set_fullscreen(Display* display, Window window)
{
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    XEvent event = { 0 };
    event.type = ClientMessage;
    event.xclient.window = the_main_window;
    event.xclient.message_type = wm_state;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = fullscreen;
    event.xclient.data.l[2] = 0;
    XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &event);
};

static void regiester_touch_window(Display* display, Window window)
{
    XIEventMask eventmask;
    unsigned char mask[1] = { 0 };
    eventmask.deviceid = 2;
    eventmask.mask_len = sizeof(mask);
    XISetMask(mask, XI_TouchBegin);
    XISetMask(mask, XI_TouchUpdate);
    XISetMask(mask, XI_TouchEnd);
    XISelectEvents(display, window, &eventmask, 1);


    if (the_xinput2_spec.is_enabled) {
        XIEventMask xieventmask;

        const int mask_len = XIMaskLen(XI_LASTEVENT);
        unsigned char* bitmask = (unsigned char*) calloc(mask_len, sizeof(unsigned char));

        xieventmask.deviceid = XIAllMasterDevices;
        xieventmask.mask = bitmask;
        xieventmask.mask_len = mask_len;

        XISetMask(bitmask, XI_TouchBegin);
        XISetMask(bitmask, XI_TouchEnd);
        XISetMask(bitmask, XI_TouchUpdate);

        XISelectEvents(display, window, &xieventmask, 1);
        free(bitmask);
    }
}

static void initialize_xinput(Display* display)
{
    the_xinput2_spec.is_enabled = XQueryExtension(display, "XInputExtension",
        &the_xinput2_spec.opcode, &the_xinput2_spec.event_base, &the_xinput2_spec.error_base);
    int ximajor = 2, ximinor = 2;
    the_xinput2_spec.is_touch_supported = (XIQueryVersion(display, &ximajor, &ximinor) == Success);
    std::cout << "with XInput 2 support: " << (the_xinput2_spec.is_enabled ? "YES" : "NO") << std::endl;
    std::cout << "with touch support: " << (the_xinput2_spec.is_touch_supported ? "YES" : "NO") << std::endl;
}

static void initialize_colors(Display* display)
{
    Colormap colormap = DefaultColormap(display, DefaultScreen(display));
    for (size_t i = 0; i < 32; ++i) {
        XColor color;
        color.red = rand() % 0xffff;
        color.green = rand() % 0xffff;
        color.blue = rand() % 0xffff;
        Status rc = XAllocColor(display, colormap, &color);
        the_colors[i] = color;
    }
}

static Window create_main_window(Display* display)
{
    Window root = DefaultRootWindow(the_display);
    int screen = DefaultScreen(the_display);
    int width = DisplayWidth(the_display, screen);
    int height = DisplayHeight(the_display, screen); 
    int black_pixel = BlackPixel(the_display, screen);
    int white_pixel = WhitePixel(the_display, screen); 
    return XCreateSimpleWindow(the_display, root,
        0, 0, width >> 1, height >> 1,
        3, black_pixel, white_pixel);
}

static void select_event(Display* display, Window window)
{
    long event_mask =
        KeymapStateMask | EnterWindowMask | LeaveWindowMask | PropertyChangeMask
        | PointerMotionMask | PointerMotionHintMask | Button1MotionMask
        | KeyPressMask | KeyReleaseMask
        | ButtonPressMask | StructureNotifyMask | ExposureMask;
    XSelectInput(display, window, event_mask);
}

int main(int argc, char* argv[])
{
    XInitThreads();

    the_display = XOpenDisplay(NULL); 
    initialize_xinput(the_display); 
    initialize_colors(the_display); 
    the_main_window = create_main_window(the_display); 
    select_event(the_display, the_main_window); 
    regiester_touch_window(the_display, the_main_window); 
    XMapWindow(the_display, the_main_window); 
    set_fullscreen(the_display, the_main_window); 
    the_main_gc = XCreateGC(the_display, the_main_window, 0, NULL); 

    XEvent event = { 0 };
    bool continued = true;
    while (continued) {
        if (XPending(the_display) <= 0) {
            /* take a longer sleep (10ms) to save cpus while there is no any touch,
             * or just give up this cpu slot for performance. */
            if (the_touch_is_idle)
                usleep(10000);
            else
                sched_yield();
            continue;
        } 
        continued = XNextEvent(the_display, &event) >= 0 && on_event(the_main_window, &event); 
    }

    XFreeGC(the_display, the_main_gc);
    XDestroyWindow(the_display, the_main_window);
    XCloseDisplay(the_display);

    return 0;
}

