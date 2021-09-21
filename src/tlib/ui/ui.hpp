#pragma once

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <stdio.h>
#include <stdexcept>
#include <iostream>
#include <GLFW/glfw3.h>

typedef enum {
    _tsc_SETUP      = 0,    // Setting up
    _tsc_CONNREQ    = 1,    // User requested connection
    _tsc_RUNNING    = 2     // Running
} _ui_enum;

typedef struct {
    _ui_enum state;
    bool enable_rdma;
    int scr_width;
    int scr_height;
    char *ifname_str;
    char *raw_conn_str;
    char *host_str;
    char *port_str;
    int res_x;
    int res_y;
    int fps;
} _ui_data;

class tsc_client_ui {

public:

    /* Initialise and render the UI. */
    void init(int width, int height);

    /* Loop once. */
    int refresh();

    /* Wait for the user to click connect */
    int wait_request();

    _ui_data get_config() {
        return ui_data;
    }

    /* Hide the cursor and allow for unlimited movement */
    void lock_cursor();

    /* Return cursor to normal operation */
    void unlock_cursor();
    

    bool cursor_state() {
        return cursor_is_locked;
    }

private:

    bool cursor_is_locked       = false;

    GLFWwindow *window;
    const char* glsl_version    = "#version 130";
    ImGuiIO io;

    const int ifname_max_len    = 20;
    const int host_max_len      = 256;
    const int port_max_len      = 6;

    _ui_data ui_data;

    void set_default_values(int width, int height);

    void init_window(int width, int height);

    void init_imgui();

    void init_fonts();

    void refresh_ui();

    void cleanup();

};
