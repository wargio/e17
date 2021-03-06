#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/**
 * NB: Events that receive a 'serial' instead of timestamp
 * 
 * input_device_attach (for pointer image)
 * input_device_button_event (button press/release)
 * input_device_key_press
 * input_device_pointer_enter
 * input_device_pointer_leave
 * input_device_keyboard_enter
 * input_device_keyboard_leave
 * input_device_touch_down
 * input_device_touch_up
 * 
 **/

#include "ecore_wl_private.h"

/* FIXME: This gives BTN_LEFT/RIGHT/MIDDLE for linux systems ... 
 *        What about other OSs ?? */
#ifdef __linux__
# include <linux/input.h>
#else
# define BTN_LEFT 0x110
# define BTN_RIGHT 0x111
# define BTN_MIDDLE 0x112
# define BTN_SIDE 0x113
# define BTN_EXTRA 0x114
# define BTN_FORWARD 0x115
# define BTN_BACK 0x116
#endif

/* Required for keysym names - in the future available in libxkbcommon */
//#include <X11/keysym.h>

#define MOD_SHIFT_MASK 0x01
#define MOD_ALT_MASK 0x02
#define MOD_CONTROL_MASK 0x04

/* local function prototypes */
static void _ecore_wl_input_cb_motion(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int timestamp, wl_fixed_t sx, wl_fixed_t sy);
static void _ecore_wl_input_cb_button(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, unsigned int timestamp, unsigned int button, unsigned int state);
static void _ecore_wl_input_cb_axis(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int timestamp, unsigned int axis, int value);
static void _ecore_wl_input_cb_key(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, unsigned int timestamp, unsigned int key, unsigned int state);
static void _ecore_wl_input_cb_pointer_enter(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy);
static void _ecore_wl_input_cb_pointer_leave(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, struct wl_surface *surface);
static void _ecore_wl_input_cb_keyboard_enter(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, struct wl_surface *surface, struct wl_array *keys __UNUSED__);
static void _ecore_wl_input_cb_keyboard_leave(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, struct wl_surface *surface);
static void _ecore_wl_input_cb_touch_down(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, unsigned int timestamp, struct wl_surface *surface __UNUSED__, int id __UNUSED__, wl_fixed_t x, wl_fixed_t y);
static void _ecore_wl_input_cb_touch_up(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, unsigned int timestamp, int id __UNUSED__);
static void _ecore_wl_input_cb_touch_motion(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int timestamp, int id __UNUSED__, wl_fixed_t x, wl_fixed_t y);
static void _ecore_wl_input_cb_touch_frame(void *data __UNUSED__, struct wl_input_device *input_device __UNUSED__);
static void _ecore_wl_input_cb_touch_cancel(void *data __UNUSED__, struct wl_input_device *input_device __UNUSED__);
static void _ecore_wl_input_cb_data_offer(void *data, struct wl_data_device *data_device, unsigned int id);
static void _ecore_wl_input_cb_data_enter(void *data, struct wl_data_device *data_device, unsigned int timestamp, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *offer);
static void _ecore_wl_input_cb_data_leave(void *data, struct wl_data_device *data_device);
static void _ecore_wl_input_cb_data_motion(void *data, struct wl_data_device *data_device, unsigned int timestamp, wl_fixed_t x, wl_fixed_t y);
static void _ecore_wl_input_cb_data_drop(void *data, struct wl_data_device *data_device);
static void _ecore_wl_input_cb_data_selection(void *data, struct wl_data_device *data_device, struct wl_data_offer *offer);

static void _ecore_wl_input_mouse_move_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_mouse_in_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_mouse_out_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_focus_in_send(Ecore_Wl_Input *input __UNUSED__, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_focus_out_send(Ecore_Wl_Input *input __UNUSED__, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_mouse_down_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_mouse_up_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp);
static void _ecore_wl_input_mouse_wheel_send(Ecore_Wl_Input *input, unsigned int axis, int value, unsigned int timestamp);

/* static int _ecore_wl_input_keysym_to_string(unsigned int symbol, char *buffer, int len); */

/* wayland interfaces */
static const struct wl_input_device_listener _ecore_wl_input_listener = 
{
   _ecore_wl_input_cb_motion,
   _ecore_wl_input_cb_button,
   _ecore_wl_input_cb_axis,
   _ecore_wl_input_cb_key,
   _ecore_wl_input_cb_pointer_enter,
   _ecore_wl_input_cb_pointer_leave,
   _ecore_wl_input_cb_keyboard_enter,
   _ecore_wl_input_cb_keyboard_leave,
   _ecore_wl_input_cb_touch_down,
   _ecore_wl_input_cb_touch_up,
   _ecore_wl_input_cb_touch_motion,
   _ecore_wl_input_cb_touch_frame,
   _ecore_wl_input_cb_touch_cancel
};

static const struct wl_data_device_listener _ecore_wl_data_listener = 
{
   _ecore_wl_input_cb_data_offer,
   _ecore_wl_input_cb_data_enter,
   _ecore_wl_input_cb_data_leave,
   _ecore_wl_input_cb_data_motion,
   _ecore_wl_input_cb_data_drop,
   _ecore_wl_input_cb_data_selection
};

/* local variables */
static int _pointer_x, _pointer_y;

EAPI void 
ecore_wl_input_grab(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int button)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   input->grab = win;
   input->grab_button = button;
}

EAPI void 
ecore_wl_input_ungrab(Ecore_Wl_Input *input)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   input->grab = NULL;
   input->grab_button = 0;
}

void 
_ecore_wl_input_add(Ecore_Wl_Display *ewd, unsigned int id)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = malloc(sizeof(Ecore_Wl_Input)))) return;

   memset(input, 0, sizeof(Ecore_Wl_Input));

   input->display = ewd;
   input->pointer_focus = NULL;
   input->keyboard_focus = NULL;

   input->input_device = 
     wl_display_bind(ewd->wl.display, id, &wl_input_device_interface);
   wl_list_insert(ewd->inputs.prev, &input->link);
   wl_input_device_add_listener(input->input_device, 
                                &_ecore_wl_input_listener, input);
   wl_input_device_set_user_data(input->input_device, input);

   input->data_device = 
     wl_data_device_manager_get_data_device(ewd->wl.data_device_manager, 
                                            input->input_device);
   wl_data_device_add_listener(input->data_device, 
                               &_ecore_wl_data_listener, input);
   ewd->input = input;
}

void 
_ecore_wl_input_del(Ecore_Wl_Input *input)
{
   if (!input) return;

   if (input->drag_source) _ecore_wl_dnd_del(input->drag_source);
   input->drag_source = NULL;

   if (input->selection_source) _ecore_wl_dnd_del(input->selection_source);
   input->selection_source = NULL;

   if (input->data_device) wl_data_device_destroy(input->data_device);
   if (input->input_device) wl_input_device_destroy(input->input_device);
   wl_list_remove(&input->link);
   free(input);
}

void 
_ecore_wl_input_pointer_xy_get(int *x, int *y)
{
   if (x) *x = _pointer_x;
   if (y) *y = _pointer_y;
}

/* local functions */
static void 
_ecore_wl_input_cb_motion(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int timestamp, wl_fixed_t sx, wl_fixed_t sy)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   _pointer_x = sx;
   _pointer_y = sy;

   input->sx = wl_fixed_to_int(sx);
   input->sy = wl_fixed_to_int(sy);

   input->timestamp = timestamp;

   /* TODO: FIXME: NB: Weston window code has set pointer image here also */
   if (input->pointer_focus)
     _ecore_wl_input_mouse_move_send(input, input->pointer_focus, timestamp);
}

static void 
_ecore_wl_input_cb_button(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, unsigned int timestamp, unsigned int button, unsigned int state)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   input->timestamp = timestamp;
   input->display->serial = serial;

//   _ecore_wl_input_mouse_move_send(input, timestamp);

   if (state)
     {
        if ((input->pointer_focus) && (!input->grab) && (state))
          ecore_wl_input_grab(input, input->pointer_focus, button);

        input->button = button;
        _ecore_wl_input_mouse_down_send(input, input->pointer_focus, 
                                        timestamp);
     }
   else
     {
        _ecore_wl_input_mouse_up_send(input, input->pointer_focus, 
                                      timestamp);
        input->button = 0;

        if ((input->grab) && (input->grab_button == button) && (!state))
          ecore_wl_input_ungrab(input);
     }

//   _ecore_wl_input_mouse_move_send(input, timestamp);
}

static void 
_ecore_wl_input_cb_axis(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int timestamp, unsigned int axis, int value)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;
   _ecore_wl_input_mouse_wheel_send(input, axis, value, timestamp);
}

/*
 * _ecore_wl_input_keysym_to_string: Translate a symbol to its printable form 
 * 
 * @symbol: the symbol to translate
 * @buffer: the buffer where to put the translated string
 * @len: size of the buffer
 *
 * Translates @symbol into a printable representation in @buffer, if possible.
 *
 * Return value: The number of bytes of the translated string, 0 if the
 *               symbol can't be printed
 *
 * Note: The code is derived from libX11's src/KeyBind.c
 *       Copyright 1985, 1987, 1998  The Open Group
 *
 */
/* static int  */
/* _ecore_wl_input_keysym_to_string(unsigned int symbol, char *buffer, int len) */
/* { */
/*   unsigned long high_bytes; */
/*   unsigned char c; */

/*    high_bytes = symbol >> 8; */
/*    if (!(len && */
/*          ((high_bytes == 0) || */
/*              ((high_bytes == 0xFF) && */
/*                  (((symbol >= XK_BackSpace) && */
/*                    (symbol <= XK_Clear)) || */
/*                      (symbol == XK_Return) || */
/*                      (symbol == XK_Escape) || */
/*                      (symbol == XK_KP_Space) || */
/*                      (symbol == XK_KP_Tab) || */
/*                      (symbol == XK_KP_Enter) || */
/*                      ((symbol >= XK_KP_Multiply) && */
/*                          (symbol <= XK_KP_9)) || */
/*                      (symbol == XK_KP_Equal) || */
/*                      (symbol == XK_Delete)))))) */
/*      return 0; */

/*    if (symbol == XK_KP_Space) */
/*    else if (high_bytes == 0xFF) */
/*      c = symbol & 0x7F; */
/*    else */
/*      c = symbol & 0xFF; */

/*    buffer[0] = c; */
/*    return 1; */
/* } */

static void 
_ecore_wl_input_cb_key(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, unsigned int timestamp, unsigned int keycode, unsigned int state)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win;
   unsigned int code, num;
   const xkb_keysym_t *syms;
   xkb_keysym_t sym;
   xkb_mod_mask_t mask;
   char string[8], key[8], keyname[8];
   Ecore_Event_Key *e;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;
   input->display->serial = serial;
   code = keycode + 8;

   win = input->keyboard_focus;
   if ((!win) || (win->keyboard_device != input)) return;

   num = xkb_key_get_syms(input->display->xkb.state, code, &syms);
   xkb_state_update_key(input->display->xkb.state, code, 
                        (state ? XKB_KEY_DOWN : XKB_KEY_UP));
   mask = xkb_state_serialize_mods(input->display->xkb.state, 
                                   (XKB_STATE_DEPRESSED | XKB_STATE_LATCHED));
   input->modifiers = 0;
   if (mask & input->display->xkb.control_mask)
     input->modifiers |= MOD_CONTROL_MASK;
   if (mask & input->display->xkb.alt_mask)
     input->modifiers |= MOD_ALT_MASK;
   if (mask & input->display->xkb.shift_mask)
     input->modifiers |= MOD_SHIFT_MASK;

   if (num == 1) sym = syms[0];
   else sym = XKB_KEY_NoSymbol;

   memset(key, 0, sizeof(key));
   memset(keyname, 0, sizeof(keyname));
   memset(string, 0, sizeof(string));

   /* TODO: Switch over to the libxkbcommon API when it is available */
   /* if (!_ecore_wl_input_keysym_to_string(sym, string, sizeof(string))) */
   /*   string[0] = '\0'; */

   xkb_keysym_get_name(sym, key, sizeof(key));
   xkb_keysym_get_name(sym, keyname, sizeof(keyname));
   xkb_keysym_get_name(sym, string, sizeof(string));

   e = malloc(sizeof(Ecore_Event_Key) + strlen(keyname) + strlen(key) +
              strlen(string) + 3);

   e->keyname = (char *)(e + 1);
   e->key = e->keyname + strlen(keyname) + 1;
   e->string = strlen(string) ? e->key + strlen(key) + 1 : NULL;
   e->compose = e->string;

   strcpy((char *)e->keyname, keyname);

   strcpy((char *)e->key, key);
   if (strlen (string))
     strcpy((char *)e->string, string);

   e->window = win->id;
   e->event_window = win->id;
   e->timestamp = timestamp;

   /* The Ecore_Event_Modifiers don't quite match the X mask bits */
   e->modifiers = input->modifiers;

   if (state)
     ecore_event_add(ECORE_EVENT_KEY_DOWN, e, NULL, NULL);
   else
     ecore_event_add(ECORE_EVENT_KEY_UP, e, NULL, NULL);

   /* if (state) */
   /*   input->modifiers |= _ecore_wl_disp->xkb->map->modmap[keycode]; */
   /* else */
   /*   input->modifiers &= ~_ecore_wl_disp->xkb->map->modmap[keycode]; */
}

static void 
_ecore_wl_input_cb_pointer_enter(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   if (!input->timestamp)
     {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        input->timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
     }

   input->sx = wl_fixed_to_int(sx);
   input->sy = wl_fixed_to_int(sy);
   input->display->serial = serial;
   input->pointer_enter_serial = serial;

   if (!(win = wl_surface_get_user_data(surface))) return;

   win->pointer_device = input;
   input->pointer_focus = win;

   _ecore_wl_input_mouse_move_send(input, win, input->timestamp);
   _ecore_wl_input_mouse_in_send(input, win, input->timestamp);

   /* NB: This whole 'if' below is a major HACK due to wayland's stupidness 
    * of not sending a mouse_up (or any notification at all for that matter) 
    * when a move or resize grab is finished */
   if (input->grab)
     {
        /* NB: This COULD mean a move has finished, or it could mean that 
         * a 'drag' is being done to a different surface */

        if ((input->grab == win) && (win->moving))
          {
             /* NB: 'Fake' a mouse_up for move finished */
             win->moving = EINA_FALSE;
             _ecore_wl_input_mouse_up_send(input, win, input->timestamp);

             input->button = 0;

             if ((input->grab) && (input->grab_button == BTN_LEFT))
               ecore_wl_input_ungrab(input);
          }
        else if ((input->grab == win) && (win->resizing))
          {
             /* NB: 'Fake' a mouse_up for resize finished */
             win->resizing = EINA_FALSE;
             _ecore_wl_input_mouse_up_send(input, win, input->timestamp);

             input->button = 0;

             if ((input->grab) && (input->grab_button == BTN_LEFT))
               ecore_wl_input_ungrab(input);
          }
        /* FIXME: Test d-n-d and potentially add needed case here */
     }
}

static void 
_ecore_wl_input_cb_pointer_leave(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, struct wl_surface *surface)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   input->display->serial = serial;

   if (!surface) return;
   if (!(win = wl_surface_get_user_data(surface))) return;

   win->pointer_device = NULL;
   input->pointer_focus = NULL;

   _ecore_wl_input_mouse_move_send(input, win, input->timestamp);
   _ecore_wl_input_mouse_out_send(input, win, input->timestamp);

   if (input->grab)
     {
        /* move or resize started */

        /* printf("Pointer Leave WITH a Grab\n"); */
     }
}

static void 
_ecore_wl_input_cb_keyboard_enter(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, struct wl_surface *surface, struct wl_array *keys __UNUSED__)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   if (!input->timestamp)
     {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        input->timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
     }

   input->display->serial = serial;

   if (!(win = wl_surface_get_user_data(surface))) return;

   win->keyboard_device = input;
   input->keyboard_focus = win;

   /* FIXME: NB: This may need to be 'serial' */
   _ecore_wl_input_focus_in_send(input, win, input->timestamp);
}

static void 
_ecore_wl_input_cb_keyboard_leave(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, struct wl_surface *surface)
{
   Ecore_Wl_Input *input;
   Ecore_Wl_Window *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   if (!input->timestamp)
     {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        input->timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
     }

   input->display->serial = serial;

   if (!surface) return;
   if (!(win = wl_surface_get_user_data(surface))) return;

   win->keyboard_device = NULL;
   input->keyboard_focus = NULL;

   _ecore_wl_input_focus_out_send(input, win, input->timestamp);
}

static void 
_ecore_wl_input_cb_touch_down(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, unsigned int timestamp, struct wl_surface *surface __UNUSED__, int id __UNUSED__, wl_fixed_t x, wl_fixed_t y)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   /* FIXME: NB: Not sure yet if input->timestamp should be set here. 
    * This needs to be tested with an actual touch device */
   /* input->timestamp = timestamp; */
   input->display->serial = serial;
   input->button = 0;
   input->sx = wl_fixed_to_int(x);
   input->sy = wl_fixed_to_int(y);
   _ecore_wl_input_mouse_down_send(input, input->pointer_focus, timestamp);
}

static void 
_ecore_wl_input_cb_touch_up(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int serial, unsigned int timestamp, int id __UNUSED__)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   /* FIXME: NB: Not sure yet if input->timestamp should be set here. 
    * This needs to be tested with an actual touch device */
   /* input->timestamp = timestamp; */
   input->button = 0;
   input->display->serial = serial;
   _ecore_wl_input_mouse_up_send(input, input->pointer_focus, timestamp);
}

static void 
_ecore_wl_input_cb_touch_motion(void *data, struct wl_input_device *input_device __UNUSED__, unsigned int timestamp, int id __UNUSED__, wl_fixed_t x, wl_fixed_t y)
{
   Ecore_Wl_Input *input;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(input = data)) return;

   /* FIXME: NB: Not sure yet if input->timestamp should be set here. 
    * This needs to be tested with an actual touch device */
   /* input->timestamp = timestamp; */
   input->sx = wl_fixed_to_int(x);
   input->sy = wl_fixed_to_int(y);

   _ecore_wl_input_mouse_move_send(input, input->pointer_focus, timestamp);
}

static void 
_ecore_wl_input_cb_touch_frame(void *data __UNUSED__, struct wl_input_device *input_device __UNUSED__)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

static void 
_ecore_wl_input_cb_touch_cancel(void *data __UNUSED__, struct wl_input_device *input_device __UNUSED__)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

static void 
_ecore_wl_input_cb_data_offer(void *data, struct wl_data_device *data_device, unsigned int id)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_add(data, data_device, id);
}

static void 
_ecore_wl_input_cb_data_enter(void *data, struct wl_data_device *data_device, unsigned int timestamp, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *offer)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_enter(data, data_device, timestamp, surface, x, y, offer);
}

static void 
_ecore_wl_input_cb_data_leave(void *data, struct wl_data_device *data_device)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_leave(data, data_device);
}

static void 
_ecore_wl_input_cb_data_motion(void *data, struct wl_data_device *data_device, unsigned int timestamp, wl_fixed_t x, wl_fixed_t y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_motion(data, data_device, timestamp, x, y);
}

static void 
_ecore_wl_input_cb_data_drop(void *data, struct wl_data_device *data_device)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_drop(data, data_device);
}

static void 
_ecore_wl_input_cb_data_selection(void *data, struct wl_data_device *data_device, struct wl_data_offer *offer)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _ecore_wl_dnd_selection(data, data_device, offer);
}

static void 
_ecore_wl_input_mouse_move_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Event_Mouse_Move *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = malloc(sizeof(Ecore_Event_Mouse_Move)))) return;

   ev->timestamp = timestamp;
   ev->x = input->sx;
   ev->y = input->sy;
   /* ev->root.x = input->sx; */
   /* ev->root.y = input->sy; */
   ev->modifiers = input->modifiers;
   ev->multi.device = 0;
   ev->multi.radius = 1;
   ev->multi.radius_x = 1;
   ev->multi.radius_y = 1;
   ev->multi.pressure = 1.0;
   ev->multi.angle = 0.0;
   ev->multi.x = input->sx;
   ev->multi.y = input->sy;

   ev->window = win->id;
   ev->event_window = win->id;

   ecore_event_add(ECORE_EVENT_MOUSE_MOVE, ev, NULL, NULL);
}

static void 
_ecore_wl_input_mouse_in_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Wl_Event_Mouse_In *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Mouse_In)))) return;

   ev->x = input->sx;
   ev->y = input->sy;
   /* ev->root.x = input->sx; */
   /* ev->root.y = input->sy; */
   ev->modifiers = input->modifiers;
   ev->timestamp = timestamp;

   ev->window = win->id;
   ev->event_window = win->id;

   ecore_event_add(ECORE_WL_EVENT_MOUSE_IN, ev, NULL, NULL);
}

static void 
_ecore_wl_input_mouse_out_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Wl_Event_Mouse_Out *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Mouse_Out)))) return;

   ev->x = input->sx;
   ev->y = input->sy;
   /* ev->root.x = input->sx; */
   /* ev->root.y = input->sy; */
   ev->modifiers = input->modifiers;
   ev->timestamp = timestamp;

   ev->window = win->id;
   ev->event_window = win->id;

   ecore_event_add(ECORE_WL_EVENT_MOUSE_OUT, ev, NULL, NULL);
}

static void 
_ecore_wl_input_focus_in_send(Ecore_Wl_Input *input __UNUSED__, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Wl_Event_Focus_In *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Focus_In)))) return;
   ev->timestamp = timestamp;
   ev->win = win->id;
   ecore_event_add(ECORE_WL_EVENT_FOCUS_IN, ev, NULL, NULL);
}

static void 
_ecore_wl_input_focus_out_send(Ecore_Wl_Input *input __UNUSED__, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Wl_Event_Focus_Out *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Focus_Out)))) return;
   ev->timestamp = timestamp;
   ev->win = win->id;
   ecore_event_add(ECORE_WL_EVENT_FOCUS_OUT, ev, NULL, NULL);
}

static void 
_ecore_wl_input_mouse_down_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Event_Mouse_Button *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = malloc(sizeof(Ecore_Event_Mouse_Button)))) return;

   if (input->button == BTN_LEFT)
     ev->buttons = 1;
   else if (input->button == BTN_MIDDLE)
     ev->buttons = 2;
   else if (input->button == BTN_RIGHT)
     ev->buttons = 3;
   else
     ev->buttons = input->button;

   ev->timestamp = timestamp;
   ev->x = input->sx;
   ev->y = input->sy;
   /* ev->root.x = input->sx; */
   /* ev->root.y = input->sy; */
   printf("Input Modifiers: %d\n", input->modifiers);
   ev->modifiers = input->modifiers;

   /* FIXME: Need to get these from wayland somehow */
   ev->double_click = 0;
   ev->triple_click = 0;

   ev->multi.device = 0;
   ev->multi.radius = 1;
   ev->multi.radius_x = 1;
   ev->multi.radius_y = 1;
   ev->multi.pressure = 1.0;
   ev->multi.angle = 0.0;
   ev->multi.x = input->sx;
   ev->multi.y = input->sy;

   ev->window = win->id;
   ev->event_window = win->id;

   ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, ev, NULL, NULL);
}

static void 
_ecore_wl_input_mouse_up_send(Ecore_Wl_Input *input, Ecore_Wl_Window *win, unsigned int timestamp)
{
   Ecore_Event_Mouse_Button *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = malloc(sizeof(Ecore_Event_Mouse_Button)))) return;

   if (input->button == BTN_LEFT)
     ev->buttons = 1;
   else if (input->button == BTN_MIDDLE)
     ev->buttons = 2;
   else if (input->button == BTN_RIGHT)
     ev->buttons = 3;
   else
     ev->buttons = input->button;

   ev->timestamp = timestamp;
   ev->x = input->sx;
   ev->y = input->sy;
   /* ev->root.x = input->sx; */
   /* ev->root.y = input->sy; */
   ev->modifiers = input->modifiers;

   /* FIXME: Need to get these from wayland somehow */
   ev->double_click = 0;
   ev->triple_click = 0;

   ev->multi.device = 0;
   ev->multi.radius = 1;
   ev->multi.radius_x = 1;
   ev->multi.radius_y = 1;
   ev->multi.pressure = 1.0;
   ev->multi.angle = 0.0;
   ev->multi.x = input->sx;
   ev->multi.y = input->sy;

   ev->window = win->id;
   ev->event_window = win->id;

   ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_UP, ev, NULL, NULL);
}

static void 
_ecore_wl_input_mouse_wheel_send(Ecore_Wl_Input *input, unsigned int axis, int value, unsigned int timestamp)
{
   Ecore_Event_Mouse_Wheel *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ev = malloc(sizeof(Ecore_Event_Mouse_Wheel)))) return;

   ev->timestamp = timestamp;
   ev->modifiers = input->modifiers;
   ev->x = input->sx;
   ev->y = input->sy;
   /* ev->root.x = input->sx; */
   /* ev->root.y = input->sy; */

   if (axis == WL_INPUT_DEVICE_AXIS_VERTICAL_SCROLL)
     {
        ev->direction = 0;
        ev->z = -value;
     }
   else if (axis == WL_INPUT_DEVICE_AXIS_HORIZONTAL_SCROLL)
     {
        ev->direction = 1;
        ev->z = -value;
     }

   if (input->grab)
     {
        ev->window = input->grab->id;
        ev->event_window = input->grab->id;
     }
   else if (input->pointer_focus)
     {
        ev->window = input->pointer_focus->id;
        ev->event_window = input->pointer_focus->id;
     }

   ecore_event_add(ECORE_EVENT_MOUSE_WHEEL, ev, NULL, NULL);
}
