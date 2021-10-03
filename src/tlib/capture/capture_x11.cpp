//
// Created by maomao on 03/10/2021.
//

#include "capture_x11.h"

#include <iostream>
//x11
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

//xrandr
#include <X11/extensions/Xrandr.h>

#define BPP    4

void tsc_capture_x11::init()
{
    // Initialise X11
    XInitThreads();
    display = XOpenDisplay(nullptr);
    root = XDefaultRootWindow( display );
    if(display == nullptr) {
        std::cout << "cannot open display\n";
        exit(0); }
};
/*
// Display *dpy;
    // dpy = XOpenDisplay("1");

    XRRScreenResources *screen;
    XRRCrtcInfo *crtc_info;
    // std::cout << "test \n";
    // //0 to get the first monitor
    screen = XRRGetScreenResources (display, root);
    // std::cout << screen->crtcs[2] << "\n";
    // can iterate thru this x and y is offsets
    // width and height
    crtc_info = XRRGetCrtcInfo (display, screen, screen->crtcs[1]);
    // std::cout << crtc_info->y << "\n";
    // std::cout << crtc_info->outputs << "\n";
    // std::cout << screen->modes->vSyncStart << "\n";
    // std::cout << screen->modes->vSyncEnd << "\n";

    // use Xrandr to get the screen list
    tsc_display display_info;

    display_info = {
        .index = 0,
        .width = (int) screen->modes->width,
        .height = (int) screen->modes->height,
        .fps = 60
    };

    x_offset = crtc_info->x;
    y_offset = crtc_info->y;

    display_info_vec.push_back(display_info);
    return display_info_vec;
    */

struct std::vector<tsc_display> tsc_capture_x11::get_displays()
{
    XRRScreenResources *screen;
    XRRCrtcInfo *crtc_info;
    screen = XRRGetScreenResources (display, root);
    std::cout << screen->modes->hSkew<< "\n";
    // Time	timestamp;
    // Time	configTimestamp;
    // int		ncrtc; shows up as 4
    // RRCrtc	*crtcs;
    // int		noutput; shows up as 8
    // RROutput	*outputs;
    // int		nmode;
    // XRRModeInfo	*modes;
        // modes
    // unsigned int	width;
    // unsigned int	height;
    // unsigned long	dotClock;
    // unsigned int	hSyncStart;
    // unsigned int	hSyncEnd;
    // unsigned int	hTotal;
    // unsigned int	hSkew;
    // unsigned int	vSyncStart;
    // unsigned int	vSyncEnd;
    // unsigned int	vTotal;
    // char		*name;
    // unsigned int	nameLength;
    // XRRModeFlags	modeFlags;
    for (int i=0; i < 2; i++){
        tsc_display display_info;
        crtc_info = XRRGetCrtcInfo (display, screen, screen->crtcs[i]);
        display_info = {
        .index = i,
        .width = (int) screen->modes->width,
        .height = (int) screen->modes->height,
        .fps = 60
        };
        //
        tsc_offset offset;
        offset.x_offset = crtc_info->x;
        offset.y_offset = crtc_info->y;
        offset_vec.push_back(offset);
        display_info_vec.push_back(display_info);
    }
    return display_info_vec;
    
};

struct tsc_frame_buffer *tsc_capture_x11::alloc_frame(int index)
{   tsc_frame_buffer *buf = new tsc_frame_buffer;
    shmimage *temp;
    std::cout << offset_vec[index].x_offset << "\n";
    std::cout << offset_vec[index].y_offset << "\n";
    temp = tsc_capture_x11::init_x11(display,display_info_vec[index].width,display_info_vec[index].height,offset_vec[index].x_offset,offset_vec[index].y_offset);
    buf->display = display_info_vec[index];
    // figure out how to deal with this later
    buf->buffer_id = 0;
    buf->pixmap = 0;
    buf->buffer = temp->shminfo.shmaddr;
    buf->buffer_length=0;
    // needed to update the ximage
    buf->context = temp;
    return buf;
    
};

void tsc_capture_x11::update_frame(struct tsc_frame_buffer *frame_buffer){
    get_frame((shmimage *) frame_buffer->context);
}

void tsc_capture_x11::initimage( struct shmimage * image )
{
    image->ximage = nullptr ;
    image->shminfo.shmaddr = (char *) -1 ;
}

void tsc_capture_x11::destroyimage( Display * dsp, struct shmimage * image )
{
    if( image->ximage )
    {
        XShmDetach( dsp, &image->shminfo ) ;
        XDestroyImage( image->ximage ) ;
        image->ximage = nullptr ;
    }

    if( image->shminfo.shmaddr != ( char * ) -1 )
    {
        shmdt( image->shminfo.shmaddr ) ;
        image->shminfo.shmaddr = ( char * ) -1 ;
    }
}

int tsc_capture_x11::init_xshm(Display * dsp, struct shmimage * image , int width, int height){
    // Create a shared memory area
    image->shminfo.shmid = shmget( IPC_PRIVATE, width * height * BPP, IPC_CREAT | 0600 ) ;
    if( image->shminfo.shmid == -1 )
    {
        return false ;
    }

    // Map the shared memory segment into the address space of this process
    image->shminfo.shmaddr = (char *) shmat( image->shminfo.shmid, nullptr, 0 ) ;
    if( image->shminfo.shmaddr == (char *) -1 )
    {
        return false ;
    }

    image->data = (unsigned int*) image->shminfo.shmaddr ;
    image->shminfo.readOnly = false ;

    // Mark the shared memory segment for removal
    // It will be removed even if this program crashes
    shmctl( image->shminfo.shmid, IPC_RMID, nullptr ) ;

    // Allocate the memory needed for the XImage structure
    image->ximage = XShmCreateImage( dsp, XDefaultVisual( dsp, XDefaultScreen( dsp ) ),
                                     DefaultDepth( dsp, XDefaultScreen( dsp ) ), ZPixmap, 0,
                                     &image->shminfo, 0, 0 ) ;
    if( !image->ximage )
    {
        destroyimage( dsp, image ) ;
        printf( "Error: could not allocate the XImage structure\n" ) ;
        return false ;
    }

    image->ximage->data = (char *)image->data ;
    image->ximage->width = width ;
    image->ximage->height = height ;

    // Ask the X server to attach the shared memory segment and sync
    XShmAttach( dsp, &image->shminfo ) ;
    XSync( dsp, false ) ;
    return true ;

}

tsc_capture_x11::shmimage *tsc_capture_x11::init_x11(Display *dsp, int dsp_width, int dsp_height,int xoffset,int yoffset){
    x_offset = xoffset;
    y_offset = yoffset;
    shmimage *src = new shmimage;

    // initalise the xshm image
    initimage(src);
    // init xshm for to capture
    if( !init_xshm( dsp, src, dsp_width, dsp_height ) )
    {
        std::cout << "Error: Unable to initialise XShm";
        XCloseDisplay( dsp ) ;
    }
    return src;
}

const void *tsc_capture_x11::get_frame(shmimage *src)
    {   
        XShmGetImage( display, XDefaultRootWindow( display ), src->ximage, x_offset, y_offset, AllPlanes ) ;
        return reinterpret_cast<void *>(src->ximage->data);
    }

void tsc_capture_x11::free_displays(struct tsc_display *display){
    free(display);
}

void tsc_capture_x11::free_frame(struct tsc_frame_buffer *frame_buffer){
    free(frame_buffer->buffer);
    free(frame_buffer->context);

}