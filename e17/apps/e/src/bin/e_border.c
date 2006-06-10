/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

//#define INOUTDEBUG_MOUSE 1
//#define INOUTDEBUG_FOCUS 1

/* These are compatible with netwm */
#define RESIZE_TL   0
#define RESIZE_T    1
#define RESIZE_TR   2
#define RESIZE_R    3
#define RESIZE_BR   4
#define RESIZE_B    5
#define RESIZE_BL   6
#define RESIZE_L    7
#define MOVE        8
#define RESIZE_NONE 11

/* local subsystem functions */
static void _e_border_free(E_Border *bd);
static void _e_border_del(E_Border *bd);

/* FIXME: these likely belong in a separate icccm/client handler */
/* and the border needs to become a dumb object that just does what its */
/* told to do */
static int _e_border_cb_window_show_request(void *data, int ev_type, void *ev);
static int _e_border_cb_window_destroy(void *data, int ev_type, void *ev);
static int _e_border_cb_window_hide(void *data, int ev_type, void *ev);
static int _e_border_cb_window_reparent(void *data, int ev_type, void *ev);
static int _e_border_cb_window_configure_request(void *data, int ev_type, void *ev);
static int _e_border_cb_window_resize_request(void *data, int ev_type, void *ev);
static int _e_border_cb_window_gravity(void *data, int ev_type, void *ev);
static int _e_border_cb_window_stack_request(void *data, int ev_type, void *ev);
static int _e_border_cb_window_property(void *data, int ev_type, void *ev);
static int _e_border_cb_window_colormap(void *data, int ev_type, void *ev);
static int _e_border_cb_window_shape(void *data, int ev_type, void *ev);
static int _e_border_cb_window_focus_in(void *data, int ev_type, void *ev);
static int _e_border_cb_window_focus_out(void *data, int ev_type, void *ev);
static int _e_border_cb_client_message(void *data, int ev_type, void *ev);

static int _e_border_cb_window_state_request(void *data, int ev_type, void *ev);
static int _e_border_cb_window_move_resize_request(void *data, int ev_type, void *ev);
static int _e_border_cb_desktop_change(void *data, int ev_type, void *ev);
static int _e_border_cb_sync_alarm(void *data, int ev_type, void *ev);

static int  _e_border_cb_pointer_warp(void *data, int ev_type, void *ev);
static void _e_border_cb_signal_bind(void *data, Evas_Object *obj, const char *emission, const char *source);
static int  _e_border_cb_mouse_in(void *data, int type, void *event);
static int  _e_border_cb_mouse_out(void *data, int type, void *event);
static int  _e_border_cb_mouse_wheel(void *data, int type, void *event);
static int  _e_border_cb_mouse_down(void *data, int type, void *event);
static int  _e_border_cb_mouse_up(void *data, int type, void *event);
static int  _e_border_cb_mouse_move(void *data, int type, void *event);
static int  _e_border_cb_grab_replay(void *data, int type, void *event);

static void _e_border_eval(E_Border *bd);
static void _e_border_moveinfo_gather(E_Border *bd, const char *source);
static void _e_border_resize_handle(E_Border *bd);

static int  _e_border_shade_animator(void *data);

static void _e_border_event_border_add_free(void *data, void *ev);
static void _e_border_event_border_remove_free(void *data, void *ev);
static void _e_border_event_border_zone_set_free(void *data, void *ev);
static void _e_border_event_border_desk_set_free(void *data, void *ev);
static void _e_border_event_border_stack_free(void *data, void *ev);
static void _e_border_event_border_icon_change_free(void *data, void *ev);
static void _e_border_event_border_focus_in_free(void *data, void *ev);
static void _e_border_event_border_focus_out_free(void *data, void *ev);
static void _e_border_event_border_resize_free(void *data, void *ev);
static void _e_border_event_border_move_free(void *data, void *ev);
static void _e_border_event_border_show_free(void *data, void *ev);
static void _e_border_event_border_hide_free(void *data, void *ev);
static void _e_border_event_border_iconify_free(void *data, void *ev);
static void _e_border_event_border_uniconify_free(void *data, void *ev);
static void _e_border_event_border_stick_free(void *data, void *ev);
static void _e_border_event_border_unstick_free(void *data, void *ev);

static void _e_border_zone_update(E_Border *bd);

static int  _e_border_resize_begin(E_Border *bd);
static int  _e_border_resize_end(E_Border *bd);
static void _e_border_resize_update(E_Border *bd);

static int  _e_border_move_begin(E_Border *bd);
static int  _e_border_move_end(E_Border *bd);
static void _e_border_move_update(E_Border *bd);

static int  _e_border_cb_ping_timer(void *data);
static int  _e_border_cb_kill_timer(void *data);

static char *_e_border_winid_str_get(Ecore_X_Window win);

static void _e_border_app_change(void *data, E_App *app, E_App_Change change);

static void _e_border_pointer_resize_begin(E_Border *bd);
static void _e_border_pointer_resize_end(E_Border *bd);
static void _e_border_pointer_move_begin(E_Border *bd);
static void _e_border_pointer_move_end(E_Border *bd);
    
/* local subsystem globals */
static Evas_List *handlers = NULL;
static Evas_List *borders = NULL;
static Evas_Hash *borders_hash = NULL;
static E_Border  *focused = NULL;

static E_Border    *resize = NULL;
static E_Border    *move = NULL;

static int grabbed = 0;

static Evas_List *focus_stack = NULL;

static Ecore_X_Screen_Size screen_size = { -1, -1 };

EAPI int E_EVENT_BORDER_ADD = 0;
EAPI int E_EVENT_BORDER_REMOVE = 0;
EAPI int E_EVENT_BORDER_ZONE_SET = 0;
EAPI int E_EVENT_BORDER_DESK_SET = 0;
EAPI int E_EVENT_BORDER_RESIZE = 0;
EAPI int E_EVENT_BORDER_MOVE = 0;
EAPI int E_EVENT_BORDER_SHOW = 0;
EAPI int E_EVENT_BORDER_HIDE = 0;
EAPI int E_EVENT_BORDER_ICONIFY = 0;
EAPI int E_EVENT_BORDER_UNICONIFY = 0;
EAPI int E_EVENT_BORDER_STICK = 0;
EAPI int E_EVENT_BORDER_UNSTICK = 0;
EAPI int E_EVENT_BORDER_STACK = 0;
EAPI int E_EVENT_BORDER_ICON_CHANGE = 0;
EAPI int E_EVENT_BORDER_FOCUS_IN = 0;
EAPI int E_EVENT_BORDER_FOCUS_OUT = 0;

#define GRAV_SET(bd, grav) \
ecore_x_window_gravity_set(bd->bg_win, grav); \
ecore_x_window_gravity_set(bd->client.shell_win, grav); \
ecore_x_window_gravity_set(bd->client.win, grav); \
ecore_x_window_gravity_set(bd->bg_subwin, grav); \
ecore_x_window_pixel_gravity_set(bd->bg_subwin, grav);

/* externally accessible functions */
EAPI int
e_border_init(void)
{
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW_REQUEST, _e_border_cb_window_show_request, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY, _e_border_cb_window_destroy, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE, _e_border_cb_window_hide, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT, _e_border_cb_window_reparent, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE_REQUEST, _e_border_cb_window_configure_request, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_RESIZE_REQUEST, _e_border_cb_window_resize_request, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_GRAVITY, _e_border_cb_window_gravity, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK_REQUEST, _e_border_cb_window_stack_request, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _e_border_cb_window_property, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_COLORMAP, _e_border_cb_window_colormap, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE, _e_border_cb_window_shape, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN, _e_border_cb_window_focus_in, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _e_border_cb_window_focus_out, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _e_border_cb_client_message, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STATE_REQUEST, _e_border_cb_window_state_request, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_MOVE_RESIZE_REQUEST, _e_border_cb_window_move_resize_request, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_DESKTOP_CHANGE, _e_border_cb_desktop_change, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_SYNC_ALARM, _e_border_cb_sync_alarm, NULL));
   ecore_x_passive_grab_replay_func_set(_e_border_cb_grab_replay, NULL);

   handlers = evas_list_append(handlers, ecore_event_handler_add(E_EVENT_POINTER_WARP, _e_border_cb_pointer_warp, NULL));

   e_app_change_callback_add(_e_border_app_change, NULL);

   E_EVENT_BORDER_ADD = ecore_event_type_new();
   E_EVENT_BORDER_REMOVE = ecore_event_type_new();
   E_EVENT_BORDER_DESK_SET = ecore_event_type_new();
   E_EVENT_BORDER_ZONE_SET = ecore_event_type_new();
   E_EVENT_BORDER_RESIZE = ecore_event_type_new();
   E_EVENT_BORDER_MOVE = ecore_event_type_new();
   E_EVENT_BORDER_SHOW = ecore_event_type_new();
   E_EVENT_BORDER_HIDE = ecore_event_type_new();
   E_EVENT_BORDER_ICONIFY = ecore_event_type_new();
   E_EVENT_BORDER_UNICONIFY = ecore_event_type_new();
   E_EVENT_BORDER_STICK = ecore_event_type_new();
   E_EVENT_BORDER_UNSTICK = ecore_event_type_new();
   E_EVENT_BORDER_STACK = ecore_event_type_new();
   E_EVENT_BORDER_ICON_CHANGE = ecore_event_type_new();
   E_EVENT_BORDER_FOCUS_IN = ecore_event_type_new();
   E_EVENT_BORDER_FOCUS_OUT = ecore_event_type_new();

   return 1;
}

EAPI int
e_border_shutdown(void)
{
   while (handlers)
     {
	Ecore_Event_Handler *h;

	h = handlers->data;
	handlers = evas_list_remove_list(handlers, handlers);
	ecore_event_handler_del(h);
     }
   
   e_app_change_callback_del(_e_border_app_change, NULL);
   
   return 1;
}

EAPI E_Border *
e_border_new(E_Container *con, Ecore_X_Window win, int first_map, int internal)
{
   E_Border *bd;
   Ecore_X_Window_Attributes *att;
   unsigned int managed, desk[2];
   int deskx, desky;

   bd = E_OBJECT_ALLOC(E_Border, E_BORDER_TYPE, _e_border_free);
   if (!bd) return NULL;
   e_object_del_func_set(E_OBJECT(bd), E_OBJECT_CLEANUP_FUNC(_e_border_del));

   bd->w = 1;
   bd->h = 1;
   /* FIXME: ewww - round trip */
   bd->client.argb = ecore_x_window_argb_get(win);
   if (bd->client.argb)
     bd->win = ecore_x_window_manager_argb_new(con->win, 0, 0, bd->w, bd->h);
   else
     bd->win = ecore_x_window_override_new(con->win, 0, 0, bd->w, bd->h);
   ecore_x_window_shape_events_select(bd->win, 1);
   e_bindings_mouse_grab(E_BINDING_CONTEXT_BORDER, bd->win);
   e_bindings_wheel_grab(E_BINDING_CONTEXT_BORDER, bd->win);
   e_focus_setup(bd);
   bd->bg_ecore_evas = e_canvas_new(e_config->evas_engine_borders, bd->win,
				    0, 0, bd->w, bd->h, 1, 0,
				    &(bd->bg_win), &(bd->bg_subwin));
   e_canvas_add(bd->bg_ecore_evas);
   bd->event_win = ecore_x_window_input_new(bd->win, 0, 0, bd->w, bd->h);
   bd->bg_evas = ecore_evas_get(bd->bg_ecore_evas);
   ecore_x_window_shape_events_select(bd->bg_win, 1);
   ecore_evas_name_class_set(bd->bg_ecore_evas, "E", "Frame_Window");
   ecore_evas_title_set(bd->bg_ecore_evas, "Enlightenment Frame");
   if (bd->client.argb)
     bd->client.shell_win = ecore_x_window_manager_argb_new(bd->win, 0, 0, 1, 1);
   else
     bd->client.shell_win = ecore_x_window_override_new(bd->win, 0, 0, 1, 1);
   ecore_x_window_container_manage(bd->client.shell_win);
   if (!internal) ecore_x_window_client_manage(win);
   /* FIXME: Round trip. XCB */
   /* fetch needed to avoid grabbing the server as window may vanish */
   att = &bd->client.initial_attributes;
   if ((!ecore_x_window_attributes_get(win, att)) || (att->input_only))
     {
//	printf("##- ATTR FETCH FAILED/INPUT ONLY FOR 0x%x - ABORT MANAGE\n", win);
	e_canvas_del(bd->bg_ecore_evas);
	ecore_evas_free(bd->bg_ecore_evas);
	ecore_x_window_del(bd->client.shell_win);
	e_bindings_mouse_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
	e_bindings_wheel_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
	ecore_x_window_del(bd->win);
	free(bd);
	return NULL;
     }
   bd->handlers = evas_list_append(bd->handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN, _e_border_cb_mouse_in, bd));
   bd->handlers = evas_list_append(bd->handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT, _e_border_cb_mouse_out, bd));
   bd->handlers = evas_list_append(bd->handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN, _e_border_cb_mouse_down, bd));
   bd->handlers = evas_list_append(bd->handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP, _e_border_cb_mouse_up, bd));
   bd->handlers = evas_list_append(bd->handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE, _e_border_cb_mouse_move, bd));
   bd->handlers = evas_list_append(bd->handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL, _e_border_cb_mouse_wheel, bd));

   bd->client.win = win;

   bd->client.icccm.title = NULL;
   bd->client.icccm.name = strdup("");
   bd->client.icccm.class = strdup("");
   bd->client.icccm.icon_name = NULL;
   bd->client.icccm.machine = strdup("");
   bd->client.icccm.min_w = 1;
   bd->client.icccm.min_h = 1;
   bd->client.icccm.max_w = 32767;
   bd->client.icccm.max_h = 32767;
   bd->client.icccm.base_w = 0;
   bd->client.icccm.base_h = 0;
   bd->client.icccm.step_w = -1;
   bd->client.icccm.step_h = -1;
   bd->client.icccm.min_aspect = 0.0;
   bd->client.icccm.max_aspect = 0.0;
   bd->client.icccm.accepts_focus = 1;

   bd->client.netwm.pid = 0;
   bd->client.netwm.name = NULL;
   bd->client.netwm.icon_name = NULL;
   bd->client.netwm.desktop = 0;
   bd->client.netwm.state.modal = 0;
   bd->client.netwm.state.sticky = 0;
   bd->client.netwm.state.shaded = 0;
   bd->client.netwm.state.hidden = 0;
   bd->client.netwm.state.maximized_v = 0;
   bd->client.netwm.state.maximized_h = 0;
   bd->client.netwm.state.skip_taskbar = 0;
   bd->client.netwm.state.skip_pager = 0;
   bd->client.netwm.state.fullscreen = 0;
   bd->client.netwm.state.stacking = E_STACKING_NONE;
   bd->client.netwm.action.move = 0;
   bd->client.netwm.action.resize = 0;
   bd->client.netwm.action.minimize = 0;
   bd->client.netwm.action.shade = 0;
   bd->client.netwm.action.stick = 0;
   bd->client.netwm.action.maximized_h = 0;
   bd->client.netwm.action.maximized_v = 0;
   bd->client.netwm.action.fullscreen = 0;
   bd->client.netwm.action.change_desktop = 0;
   bd->client.netwm.action.close = 0;
   bd->client.netwm.type = ECORE_X_WINDOW_TYPE_UNKNOWN;

     {
	int at_num = 0, i;
	Ecore_X_Atom *atoms;
	
	atoms = ecore_x_window_prop_list(bd->client.win, &at_num);
	bd->client.icccm.fetch.command = 1;
	if (atoms)
	  {
	     /* icccm */
	     for (i = 0; i < at_num; i++)
	       {
		  if (atoms[i] == ECORE_X_ATOM_WM_NAME)
		    bd->client.icccm.fetch.title = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_CLASS)
		    bd->client.icccm.fetch.name_class = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_ICON_NAME)
		    bd->client.icccm.fetch.icon_name = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_CLIENT_MACHINE)
		    bd->client.icccm.fetch.machine = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_HINTS)
		    bd->client.icccm.fetch.hints = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_NORMAL_HINTS)
		    bd->client.icccm.fetch.size_pos_hints = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_PROTOCOLS)
		    bd->client.icccm.fetch.protocol = 1;
		  else if (atoms[i] == ECORE_X_ATOM_MOTIF_WM_HINTS)
		    bd->client.mwm.fetch.hints = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_TRANSIENT_FOR)
		    {
		       bd->client.icccm.fetch.transient_for = 1;
		       bd->client.netwm.fetch.type = 1;
		    }
		  else if (atoms[i] == ECORE_X_ATOM_WM_CLIENT_LEADER)
		    bd->client.icccm.fetch.client_leader = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_WINDOW_ROLE)
		    bd->client.icccm.fetch.window_role = 1;
		  else if (atoms[i] == ECORE_X_ATOM_WM_STATE)
		    bd->client.icccm.fetch.state = 1;
	       }
	     /* netwm, loop again, netwm will ignore some icccm, so we
	      * have to be sure that netwm is checked after */
	     for (i = 0; i < at_num; i++)
	       {
		  if (atoms[i] == ECORE_X_ATOM_NET_WM_NAME)
		    {
		       /* Ignore icccm */
		       bd->client.icccm.fetch.title = 0;
		       bd->client.netwm.fetch.name = 1;
		    }
		  else if (atoms[i] == ECORE_X_ATOM_NET_WM_ICON_NAME)
		    {
		       /* Ignore icccm */
		       bd->client.icccm.fetch.icon_name = 0;
		       bd->client.netwm.fetch.icon_name = 1;
		    }
		  else if (atoms[i] == ECORE_X_ATOM_NET_WM_ICON)
		    {
		       bd->client.netwm.fetch.icon = 1;
		    }
		  else if (atoms[i] == ECORE_X_ATOM_NET_WM_USER_TIME)
		    {
		       bd->client.netwm.fetch.user_time = 1;
		    }
		  else if (atoms[i] == ECORE_X_ATOM_NET_WM_STRUT)
		    {
		       printf("ECORE_X_ATOM_NET_WM_STRUT\n");
		       bd->client.netwm.fetch.strut = 1;
		    }
		  else if (atoms[i] == ECORE_X_ATOM_NET_WM_STRUT_PARTIAL)
		    {
		       printf("ECORE_X_ATOM_NET_WM_STRUT_PARTIAL\n");
		       bd->client.netwm.fetch.strut = 1;
		    }
		  else if (atoms[i] == ECORE_X_ATOM_NET_WM_WINDOW_TYPE)
		    {
		       /* Ignore mwm
		       bd->client.mwm.fetch.hints = 0;
		       */
		       bd->client.netwm.fetch.type = 1;
		    }
		  else if (atoms[i] == ECORE_X_ATOM_NET_WM_STATE)
		    {
		       bd->client.netwm.fetch.state = 1;
		    }
	       }
	     /* loop to check for own atoms */
	     for (i = 0; i < at_num; i++)
	       {
		  if (atoms[i] == E_ATOM_WINDOW_STATE)
		    {
		       bd->client.e.fetch.state = 1;
		    }
	       }
	     free(atoms);
	  }
     }
   bd->client.border.changed = 1;
   
   bd->client.w = att->w;
   bd->client.h = att->h;

   bd->w = bd->client.w;
   bd->h = bd->client.h;

   bd->resize_mode = RESIZE_NONE;
   bd->layer = 100;
   bd->changes.icon = 1;
   bd->changes.size = 1;
   bd->changes.shape = 1;

//   printf("##- ON MAP CLIENT 0x%x SIZE %ix%i\n",
//	  bd->client.win, bd->client.w, bd->client.h);

   /* FIXME: if first_map is 1 then we should ignore the first hide event
    * or ensure the window is alreayd hidden and events flushed before we
    * create a border for it */
   if (first_map)
     {
//	printf("##- FIRST MAP\n");
	bd->x = att->x;
	bd->y = att->y;
	bd->changes.pos = 1;
	bd->re_manage = 1;
	bd->ignore_first_unmap = 2;
     }

   /* just to friggin make java happy - we're DELAYING the reparent until
    * eval time...
    */
/*   ecore_x_window_reparent(win, bd->client.shell_win, 0, 0); */
   bd->need_reparent = 1;
   
   ecore_x_window_border_width_set(win, 0);
   ecore_x_window_show(bd->event_win);
   ecore_x_window_show(bd->client.shell_win);
   bd->shape = e_container_shape_add(con);

   if (e_config->focus_setting != E_FOCUS_NONE)
     bd->take_focus = 1;
   bd->new_client = 1;
   bd->changed = 1;

   bd->zone = e_zone_current_get(con);
   bd->desk = e_desk_current_get(bd->zone);
   e_container_border_add(bd);
   borders = evas_list_append(borders, bd);
   borders_hash = evas_hash_add(borders_hash, _e_border_winid_str_get(bd->client.win), bd);
   borders_hash = evas_hash_add(borders_hash, _e_border_winid_str_get(bd->bg_win), bd);
   borders_hash = evas_hash_add(borders_hash, _e_border_winid_str_get(bd->win), bd);
   managed = 1;
   ecore_x_window_prop_card32_set(win, E_ATOM_MANAGED, &managed, 1);
   ecore_x_window_prop_card32_set(win, E_ATOM_CONTAINER, &bd->zone->container->num, 1);
   ecore_x_window_prop_card32_set(win, E_ATOM_ZONE, &bd->zone->num, 1);
   e_desk_xy_get(bd->desk, &deskx, &desky);
   desk[0] = deskx;
   desk[1] = desky;
   ecore_x_window_prop_card32_set(win, E_ATOM_DESK, desk, 2);

   focus_stack = evas_list_append(focus_stack, bd);
   
   return bd;
}

EAPI void
e_border_zone_set(E_Border *bd, E_Zone *zone)
{
   E_Event_Border_Zone_Set *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   if (bd->zone == zone) return;

   /* if the window does not lie in the new zone, move it so that it does */
   if (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h, zone->x, zone->y, zone->w, zone->h))
     {
	int x, y;
	/* first guess -- get offset from old zone, and apply to new zone */
	x = zone->x + (bd->x - bd->zone->x);
	y = zone->y + (bd->y - bd->zone->y);

	/* keep window from hanging off bottom and left */
	if (x + bd->w > zone->x + zone->w) x += (zone->x + zone->w) - (x + bd->w);
	if (y + bd->h > zone->y + zone->h) y += (zone->y + zone->h) - (y + bd->h);

	/* make sure to and left are on screen (if the window is larger than the zone, it will hang off the bottom / right) */
	if (x < zone->x) x = zone->x;
	if (y < zone->y) y = zone->y;

	if (!E_INTERSECTS(x, y, bd->w, bd->h, zone->x, zone->y, zone->w, zone->h))	
	  {
	     /* still not in zone at all, so just move it to closest edge */
	     if (x < zone->x) x = zone->x;
	     if (x >= zone->x + zone->w) x = zone->x + zone->w - bd->w;
	     if (y < zone->y) y = zone->y;
	     if (y >= zone->y + zone->h) y = zone->y + zone->h - bd->h;
	  }
	e_border_move(bd, x, y);
     }

   bd->zone = zone;

   if (bd->desk->zone != bd->zone)
     {
	e_border_desk_set(bd, e_desk_current_get(bd->zone));
     }


   ev = calloc(1, sizeof(E_Event_Border_Zone_Set));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_zone_set_event");
   ev->zone = zone;
   e_object_ref(E_OBJECT(zone));

   ecore_event_add(E_EVENT_BORDER_ZONE_SET, ev, _e_border_event_border_zone_set_free, NULL);

   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_ZONE, &bd->zone->num, 1);
}

EAPI void
e_border_desk_set(E_Border *bd, E_Desk *desk)
{
   E_Event_Border_Desk_Set *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);
   if (bd->desk == desk) return;
   bd->desk = desk;
   e_border_zone_set(bd, desk->zone);

   e_hints_window_desktop_set(bd);

   ev = calloc(1, sizeof(E_Event_Border_Desk_Set));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_desk_set_event");
   ev->desk = desk;
   e_object_ref(E_OBJECT(desk));
   ecore_event_add(E_EVENT_BORDER_DESK_SET, ev, _e_border_event_border_desk_set_free, NULL);

   if (bd->desk->visible)
     e_border_show(bd);
   else
     e_border_hide(bd, 1);

   if (e_config->transient.desktop)
     {
	Evas_List *l;
	for (l = bd->transients; l; l = l->next)
	  {
	     E_Border *child;

	     child = l->data;
	     e_border_desk_set(child, bd->desk);
	  }
     }
}

EAPI void
e_border_show(E_Border *bd)
{
   E_Event_Border_Show *ev;
   unsigned int visible;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->visible) return;
   e_container_shape_show(bd->shape);
   if (!bd->need_reparent)
     ecore_x_window_show(bd->client.win);
   e_hints_window_visible_set(bd);
   bd->visible = 1;
   bd->changes.visible = 1;

   visible = 1;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &visible, 1);
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MANAGED, &visible, 1);

   ev = calloc(1, sizeof(E_Event_Border_Show));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_show_event");
   ecore_event_add(E_EVENT_BORDER_SHOW, ev, _e_border_event_border_show_free, NULL);
}

EAPI void
e_border_hide(E_Border *bd, int manage)
{
   unsigned int visible;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!bd->visible) return;
   if (bd->moving)
     _e_border_move_end(bd);
   if (bd->fullscreen)
     e_border_unfullscreen(bd);
   if (bd->resize_mode != RESIZE_NONE)
     {
	_e_border_pointer_resize_end(bd);
	bd->resize_mode = RESIZE_NONE;
	_e_border_resize_end(bd);
     }

   e_container_shape_hide(bd->shape);
   /* FIXME: If the client unmaps itself, the border should be
    * withdrawn, not iconic */
   if (!bd->iconic)
     e_hints_window_hidden_set(bd);

   bd->visible = 0;
   bd->changes.visible = 1;

   if (!bd->need_reparent)
     {
	if (bd->focused)
	  {
	     e_border_focus_set(bd, 0, 1);
	     if (manage != 2)
	       {
		  if (e_config->focus_revert_on_hide_or_close)
		    e_desk_last_focused_focus(bd->desk);
	       }
	  }
	if (manage == 1)
	  {
	     /* Make sure that this border isn't deleted */
	     bd->await_hide_event++;
	  }
	if (manage != 2)
	  ecore_x_window_hide(bd->client.win);
     }
   
   visible = 0;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &visible, 1);
   if (!manage)
     ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MANAGED, &visible, 1);
   
     {
	E_Event_Border_Hide *ev;
	
	ev = calloc(1, sizeof(E_Event_Border_Hide));
	ev->border = bd;
	e_object_ref(E_OBJECT(bd));
//	e_object_breadcrumb_add(E_OBJECT(bd), "border_hide_event");
	ecore_event_add(E_EVENT_BORDER_HIDE, ev, _e_border_event_border_hide_free, NULL);
     }
}

EAPI void
e_border_move(E_Border *bd, int x, int y)
{
   E_Event_Border_Move *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((bd->fullscreen) || 
       (((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN) && (!e_config->allow_manip))) 
	   return;
   if (bd->new_client)
     {
	E_Border_Pending_Move_Resize  *pnd;

	pnd = E_NEW(E_Border_Pending_Move_Resize, 1);
	if (!pnd) return;
	pnd->move = 1;
	pnd->x = x;
	pnd->y = y;
	bd->pending_move_resize = evas_list_append(bd->pending_move_resize, pnd);
	return;
     }
   if ((x == bd->x) && (y == bd->y)) return;
   bd->x = x;
   bd->y = y;
   bd->changed = 1;
   bd->changes.pos = 1;
#if 0
   if (bd->client.netwm.sync.request)
     {
	bd->client.netwm.sync.wait++;
	ecore_x_netwm_sync_request_send(bd->client.win, bd->client.netwm.sync.serial++);
     }
#endif
   if (bd->internal_ecore_evas)
     ecore_evas_managed_move(bd->internal_ecore_evas,
			     bd->x + bd->client_inset.l,
			     bd->y + bd->client_inset.t);
   ecore_x_icccm_move_resize_send(bd->client.win,
				  bd->x + bd->client_inset.l,
				  bd->y + bd->client_inset.t,
				  bd->client.w,
				  bd->client.h);
   _e_border_move_update(bd);
   ev = calloc(1, sizeof(E_Event_Border_Move));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//  e_object_breadcrumb_add(E_OBJECT(bd), "border_move_event");
   ecore_event_add(E_EVENT_BORDER_MOVE, ev, _e_border_event_border_move_free, NULL);
   _e_border_zone_update(bd);
}

EAPI void
e_border_resize(E_Border *bd, int w, int h)
{
   E_Event_Border_Resize *ev;
   
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((bd->shaded) || (bd->shading) || (bd->fullscreen) ||
       (((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN) && (!e_config->allow_manip)))
     return;
   if (bd->new_client)
     {
	E_Border_Pending_Move_Resize  *pnd;

	pnd = E_NEW(E_Border_Pending_Move_Resize, 1);
	if (!pnd) return;
	pnd->resize = 1;
	pnd->w = w - (bd->client_inset.l + bd->client_inset.r);
	pnd->h = h - (bd->client_inset.t + bd->client_inset.b);
	bd->pending_move_resize = evas_list_append(bd->pending_move_resize, pnd);
	return;
     }
   if ((w == bd->w) && (h == bd->h)) return;
   bd->w = w;
   bd->h = h;
   bd->client.w = bd->w - (bd->client_inset.l + bd->client_inset.r);
   bd->client.h = bd->h - (bd->client_inset.t + bd->client_inset.b);
   bd->changed = 1;
   bd->changes.size = 1;
   if ((bd->shaped) || (bd->client.shaped))
     {
	bd->need_shape_merge = 1;
	bd->need_shape_export = 1;
     }
   if (bd->client.netwm.sync.request)
     {
	bd->client.netwm.sync.wait++;
	ecore_x_netwm_sync_request_send(bd->client.win, bd->client.netwm.sync.serial++);
     }
   if (bd->internal_ecore_evas)
     ecore_evas_managed_move(bd->internal_ecore_evas,
			     bd->x + bd->client_inset.l,
			     bd->y + bd->client_inset.t);
   ecore_x_icccm_move_resize_send(bd->client.win,
				  bd->x + bd->client_inset.l,
				  bd->y + bd->client_inset.t,
				  bd->client.w,
				  bd->client.h);
   ev = calloc(1, sizeof(E_Event_Border_Resize));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
   ecore_event_add(E_EVENT_BORDER_RESIZE, ev, _e_border_event_border_resize_free, NULL);
   _e_border_zone_update(bd);
}

EAPI void
e_border_move_resize(E_Border *bd, int x, int y, int w, int h)
{
   E_Event_Border_Move		*mev;
   E_Event_Border_Resize	*rev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((bd->fullscreen) || 
       (((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN) && (!e_config->allow_manip))) 
	   return;
   if (bd->new_client)
     {
	E_Border_Pending_Move_Resize  *pnd;

	pnd = E_NEW(E_Border_Pending_Move_Resize, 1);
	if (!pnd) return;
	pnd->move = 1;
	pnd->resize = 1;
	pnd->x = x;
	pnd->y = y;
	pnd->w = w - (bd->client_inset.l + bd->client_inset.r);
	pnd->h = h - (bd->client_inset.t + bd->client_inset.b);
	bd->pending_move_resize = evas_list_append(bd->pending_move_resize, pnd);
	return;
     }
   if ((x == bd->x) && (y == bd->y) && (w == bd->w) && (h == bd->h)) return;
   bd->x = x;
   bd->y = y;
   bd->w = w;
   bd->h = h;
   bd->client.w = bd->w - (bd->client_inset.l + bd->client_inset.r);
   bd->client.h = bd->h - (bd->client_inset.t + bd->client_inset.b);
   bd->changed = 1;
   bd->changes.pos = 1;
   bd->changes.size = 1;
   if ((bd->shaped) || (bd->client.shaped))
     {
	bd->need_shape_merge = 1;
	bd->need_shape_export = 1;
     }
   if (bd->client.netwm.sync.request)
     {
	bd->client.netwm.sync.wait++;
	ecore_x_netwm_sync_request_send(bd->client.win, bd->client.netwm.sync.serial++);
     }
   if (bd->internal_ecore_evas)
     ecore_evas_managed_move(bd->internal_ecore_evas,
			     bd->x + bd->client_inset.l,
			     bd->y + bd->client_inset.t);
   ecore_x_icccm_move_resize_send(bd->client.win,
				  bd->x + bd->client_inset.l,
				  bd->y + bd->client_inset.t,
				  bd->client.w,
				  bd->client.h);
   _e_border_resize_update(bd);
   mev = calloc(1, sizeof(E_Event_Border_Move));
   mev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_move_event");
   ecore_event_add(E_EVENT_BORDER_MOVE, mev, _e_border_event_border_move_free, NULL);

   rev = calloc(1, sizeof(E_Event_Border_Resize));
   rev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
   ecore_event_add(E_EVENT_BORDER_RESIZE, rev, _e_border_event_border_resize_free, NULL);
   _e_border_zone_update(bd);
}

EAPI void
e_border_layer_set(E_Border *bd, int layer)
{
   int raise;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   raise = e_config->transient.raise;
   
   bd->layer = layer;
   if (e_config->transient.layer)
     {
	Evas_List *l;

	/* We need to set raise to one, else the child wont
	 * follow to the new layer. It should be like this,
	 * even if the user usually doesn't want to raise
	 * the transients.
	 */
	e_config->transient.raise = 1;
	for (l = bd->transients; l; l = l->next)
	  {
	     E_Border *child;

	     child = l->data;
	     child->layer = layer;
	  }
     }
   e_border_raise(bd);
   e_config->transient.raise = raise;
}

EAPI void
e_border_raise(E_Border *bd)
{
   E_Event_Border_Stack *ev;
   E_Border *last = NULL;
   Evas_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_config->transient.raise)
     {
	for (l = evas_list_last(bd->transients); l; l = l->prev)
	  {
	     E_Border *child;

	     child = l->data;
	     /* Don't stack iconic transients. If the user wants these shown,
	      * thats another option.
	      */
	     if (!child->iconic)
	       {
		  if (last)
		    e_border_stack_below(child, last);
		  else
		    {
		       E_Border *above;

		       /* First raise the border to find out which border we will end up above */
		       above = e_container_border_raise(child);

		       if (above)
			 {
			    /* We ended up above a border, now we must stack this border to
			     * generate the stacking event, and to check if this transient
			     * has other transients etc.
			     */
			    e_border_stack_above(child, above);
			 }
		       else
			 {
			    /* If we didn't end up above any border, we are on the bottom! */
			    e_border_lower(child);
			 }
		    }
		  last = child;
	       }
	  }
     }

   ev = calloc(1, sizeof(E_Event_Border_Stack));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));

   if (last)
     {
	e_container_border_stack_below(bd, last);
	ev->stack = last;
	e_object_ref(E_OBJECT(last));
	ev->type = E_STACKING_BELOW;
     }
   else
     {
	E_Border *above;

	/* If we don't have any children, raise this border */
	above = e_container_border_raise(bd);
	if (above)
	  {
	     /* We ended up above a border */
	     ev->stack = above;
	     e_object_ref(E_OBJECT(above));
	     ev->type = E_STACKING_ABOVE;
	  }
	else
	  {
	     /* No border to raise above, same as a lower! */
	     ev->stack = NULL;
	     ev->type = E_STACKING_ABOVE;
	  }
     }

   ecore_event_add(E_EVENT_BORDER_STACK, ev, _e_border_event_border_stack_free, NULL);
}

EAPI void
e_border_lower(E_Border *bd)
{
   E_Event_Border_Stack *ev;
   E_Border *last = NULL;
   Evas_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_config->transient.lower)
     {
	for (l = evas_list_last(bd->transients); l; l = l->prev)
	  {
	     E_Border *child;

	     child = l->data;
	     /* Don't stack iconic transients. If the user wants these shown,
	      * thats another option.
	      */
	     if (!child->iconic)
	       {
		  if (last)
		    e_border_stack_below(child, last);
		  else
		    {
		       E_Border *below;

		       /* First lower the border to find out which border we will end up below */
		       below = e_container_border_lower(child);

		       if (below)
			 {
			    /* We ended up below a border, now we must stack this border to
			     * generate the stacking event, and to check if this transient
			     * has other transients etc.
			     */
			    e_border_stack_below(child, below);
			 }
		       else
			 {
			    /* If we didn't end up below any border, we are on top! */
			    e_border_raise(child);
			 }
		    }
		  last = child;
	       }
	  }
     }

   ev = calloc(1, sizeof(E_Event_Border_Stack));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));

   if (last)
     {
	e_container_border_stack_below(bd, last);
	ev->stack = last;
	e_object_ref(E_OBJECT(last));
	ev->type = E_STACKING_BELOW;
     }
   else
     {
	E_Border *below;

	/* If we don't have any children, lower this border */
	below = e_container_border_lower(bd);
	if (below)
	  {
	     /* We ended up below a border */
	     ev->stack = below;
	     e_object_ref(E_OBJECT(below));
	     ev->type = E_STACKING_BELOW;
	  }
	else
	  {
	     /* No border to hide under, same as a raise! */
	     ev->stack = NULL;
	     ev->type = E_STACKING_BELOW;
	  }
     }

   ecore_event_add(E_EVENT_BORDER_STACK, ev, _e_border_event_border_stack_free, NULL);
}

EAPI void
e_border_stack_above(E_Border *bd, E_Border *above)
{
   /* TODO: Should stack above allow the border to change level */
   E_Event_Border_Stack *ev;
   E_Border *last = NULL;
   Evas_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_config->transient.raise)
     {
	for (l = evas_list_last(bd->transients); l; l = l->prev)
	  {
	     E_Border *child;

	     child = l->data;
	     /* Don't stack iconic transients. If the user wants these shown,
	      * thats another option.
	      */
	     if (!child->iconic)
	       {
		  if (last)
		    e_border_stack_below(child, last);
		  else
		    e_border_stack_above(child, above);
		  last = child;
	       }
	  }
     }

   ev = calloc(1, sizeof(E_Event_Border_Stack));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));

   if (last)
     {
	e_container_border_stack_below(bd, last);
	ev->stack = last;
	e_object_ref(E_OBJECT(last));
	ev->type = E_STACKING_BELOW;
     }
   else
     {
	e_container_border_stack_above(bd, above);
	ev->stack = above;
	e_object_ref(E_OBJECT(above));
	ev->type = E_STACKING_ABOVE;
     }

   ecore_event_add(E_EVENT_BORDER_STACK, ev, _e_border_event_border_stack_free, NULL);
}

EAPI void
e_border_stack_below(E_Border *bd, E_Border *below)
{
   /* TODO: Should stack below allow the border to change level */
   E_Event_Border_Stack *ev;
   E_Border *last = NULL;
   Evas_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_config->transient.lower)
     {
	for (l = evas_list_last(bd->transients); l; l = l->prev)
	  {
	     E_Border *child;

	     child = l->data;
	     /* Don't stack iconic transients. If the user wants these shown,
	      * thats another option.
	      */
	     if (!child->iconic)
	       {
		  if (last)
		    e_border_stack_below(child, last);
		  else
		    e_border_stack_below(child, below);
		  last = child;
	       }
	  }
     }

   ev = calloc(1, sizeof(E_Event_Border_Stack));
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));

   if (last)
     {
	e_container_border_stack_below(bd, last);
	ev->stack = last;
	e_object_ref(E_OBJECT(last));
	ev->type = E_STACKING_BELOW;
     }
   else
     {
	e_container_border_stack_below(bd, below);
	ev->stack = below;
	e_object_ref(E_OBJECT(below));
	ev->type = E_STACKING_BELOW;
     }

   ecore_event_add(E_EVENT_BORDER_STACK, ev, _e_border_event_border_stack_free, NULL);
}

EAPI void
e_border_focus_latest_set(E_Border *bd)
{
   focus_stack = evas_list_remove(focus_stack, bd);
   focus_stack = evas_list_prepend(focus_stack, bd);
}

EAPI void
e_border_focus_set(E_Border *bd, int focus, int set)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   //printf("e_border_focus_set(%p, %i %i);\n", bd, focus, set);
   if ((bd->modal) && (bd->modal != bd))
     {
	e_border_focus_set(bd->modal, focus, set);
	return;
     }
   else if ((bd->leader) && (bd->leader->modal) && (bd->leader->modal != bd))
     {
	e_border_focus_set(bd->leader->modal, focus, set);
	return;
     }
   if ((bd->visible) && (bd->changes.visible))
     {  
	if ((bd->want_focus) && (set) && (!focus))
	  bd->want_focus = 0;
     }
   if ((focus) && (!bd->focused))
     {
	if ((bd->visible) && (bd->changes.visible))
	  {
	     bd->want_focus = 1;
	     bd->changed = 1;
	     return;
	  }
	if (bd->visible)
	  {
	     if (!e_winlist_active_get())
	       e_border_focus_latest_set(bd);
	  }
//	printf("EMIT 0x%x activeve\n", bd->client.win);
	edje_object_signal_emit(bd->bg_object, "active", "");
	if (bd->icon_object)
	  edje_object_signal_emit(bd->icon_object, "active", "");
	e_focus_event_focus_in(bd);
     }
   else if ((!focus) && (bd->focused))
     {
//	printf("EMIT 0x%x passive\n", bd->client.win);
	edje_object_signal_emit(bd->bg_object, "passive", "");
	if (bd->icon_object)
	  edje_object_signal_emit(bd->icon_object, "passive", "");
	e_focus_event_focus_out(bd);
	/* FIXME: Sometimes we should leave the window fullscreen! */
//	if (bd->fullscreen)
//	  e_border_unfullscreen(bd);
	if (bd->raise_timer)
	  {
	     ecore_timer_del(bd->raise_timer);
	     bd->raise_timer = NULL;
	  }
     }
   bd->focused = focus;
   if (set)
     {
	if (bd->focused)
	  {
	     if (bd->internal)
	       e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_PASSIVE);
	     else
	       {
		  if ((!bd->client.icccm.accepts_focus) &&
		      (!bd->client.icccm.take_focus))
		    e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_NO_INPUT);
		  else if ((bd->client.icccm.accepts_focus) &&
			   (bd->client.icccm.take_focus))
		    e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_LOCALLY_ACTIVE);
		  else if ((!bd->client.icccm.accepts_focus) &&
			   (bd->client.icccm.take_focus))
		    e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_GLOBALLY_ACTIVE);
		  else if ((bd->client.icccm.accepts_focus) &&
			   (!bd->client.icccm.take_focus))
		    e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_PASSIVE);
	       }
	  }
	else
	  {
//	     ecore_x_window_focus(bd->zone->container->manager->root);
//	     ecore_x_window_focus(bd->zone->container->bg_win);
	     e_grabinput_focus(bd->zone->container->bg_win, E_FOCUS_METHOD_PASSIVE);
	  }
     }
   if ((bd->focused) && (focused != bd))
     {
	if (focused)
	  {
//	     printf("unfocus previous\n");
	     edje_object_signal_emit(focused->bg_object, "passive", "");
	     if (focused->icon_object)
	       edje_object_signal_emit(focused->icon_object, "passive", "");
	     e_focus_event_focus_out(focused);
	     /* FIXME: Sometimes we should leave the window fullscreen! */
//	     if (focused->fullscreen) e_border_unfullscreen(focused);
	     focused->focused = 0;
//		  e_border_focus_set(focused, 0, 0);
	     if (focused->raise_timer)
	       {
		  ecore_timer_del(focused->raise_timer);
		  focused->raise_timer = NULL;
	       }
	  }
	e_hints_active_window_set(bd->zone->container->manager, bd);
     }

#if 0
   /* i'm pretty sure this case is handled above -- this was resulting in the "passive"
    * event getting sent twice when going from a window to the desktop. --rephorm */
   else if ((!bd->focused) && (focused == bd))
     {
	if (focused)
	  {
//	     printf("unfocus previous 2\n");
	     edje_object_signal_emit(focused->bg_object, "passive", "");
	     if (focused->icon_object)
	       edje_object_signal_emit(focused->icon_object, "passive", "");
	     e_focus_event_focus_out(focused);
	     /* FIXME: Sometimes we should leave the window fullscreen! */
	     if (focused->fullscreen) e_border_unfullscreen(focused);
	     focused->focused = 0;
//		  e_border_focus_set(focused, 0, 0);
	     if (focused->raise_timer)
	       {
		  ecore_timer_del(focused->raise_timer);
		  focused->raise_timer = NULL;
	       }
	  }
	e_hints_active_window_set(bd->zone->container->manager, NULL);
     }
#endif
   if (bd->focused)
     {
	E_Event_Border_Focus_In	 *ev;

	focused = bd;
	//printf("set focused to %p\n", focused);
	
	if (focus && set)
	  { 
	     // Let send the focus event iff the focus is set explicitly,
	     // not via callback
	     ev = calloc(1, sizeof(E_Event_Border_Focus_In)); 
	     ev->border = bd; 
	     e_object_ref(E_OBJECT(bd)); 
	     
	     ecore_event_add(E_EVENT_BORDER_FOCUS_IN, ev,
			     _e_border_event_border_focus_in_free, NULL);
	  }
     }
   else if ((!bd->focused) && (focused == bd))
     {
	E_Event_Border_Focus_Out *ev;

	focused = NULL;
	//printf("set focused to %p\n", focused);

	if (set)
	  { 
	     // Let send the focus event iff the focus is set explicitly,
	     // not via callback
	     ev = calloc(1, sizeof(E_Event_Border_Focus_Out)); 
	     ev->border = bd; 
	     e_object_ref(E_OBJECT(bd));

	     ecore_event_add(E_EVENT_BORDER_FOCUS_OUT, ev,
			     _e_border_event_border_focus_out_free, NULL);
	  }
     }
}

EAPI void
e_border_shade(E_Border *bd, E_Direction dir)
{
   E_Event_Border_Resize *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((bd->fullscreen) || ((bd->maximized) && (!e_config->allow_manip)) ||
       (bd->shading)) return;
   if ((bd->client.border.name) && 
       (!strcmp("borderless", bd->client.border.name))) return;
   if (!bd->shaded)
     {
//	printf("SHADE!\n");

	bd->shade.x = bd->x;
	bd->shade.y = bd->y;
	bd->shade.dir = dir;

	e_hints_window_shaded_set(bd, 1);
	e_hints_window_shade_direction_set(bd, dir);

	if (e_config->border_shade_animate)
	  {
	     bd->shade.start = ecore_time_get();
	     bd->shading = 1;
	     bd->changes.shading = 1;
	     bd->changed = 1;

	     if (bd->shade.dir == E_DIRECTION_UP ||
		 bd->shade.dir == E_DIRECTION_LEFT)
	       ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_SW);
	     else
	       ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_NE);

	     bd->shade.anim = ecore_animator_add(_e_border_shade_animator, bd);
	     edje_object_signal_emit(bd->bg_object, "shading", "");
	  }
	else
	  {
	     if (bd->shade.dir == E_DIRECTION_UP)
	       {
		  bd->h = bd->client_inset.t + bd->client_inset.b;
	       }
	     else if (bd->shade.dir == E_DIRECTION_DOWN)
	       {
		  bd->h = bd->client_inset.t + bd->client_inset.b;
		  bd->y = bd->y + bd->client.h;
		  bd->changes.pos = 1;
	       }
	     else if (bd->shade.dir == E_DIRECTION_LEFT)
	       {
		  bd->w = bd->client_inset.l + bd->client_inset.r;
	       }
	     else if (bd->shade.dir == E_DIRECTION_RIGHT)
	       {
		  bd->w = bd->client_inset.l + bd->client_inset.r;
		  bd->x = bd->x + bd->client.w;
		  bd->changes.pos = 1;
	       }

	     if ((bd->shaped) || (bd->client.shaped))
	       {
		  bd->need_shape_merge = 1;
		  bd->need_shape_export = 1;
	       }
	     bd->changes.size = 1;
	     bd->shaded = 1;
	     bd->changes.shaded = 1;
	     bd->changed = 1;
	     if ((bd->shaped) || (bd->client.shaped))
	       {
		  bd->need_shape_merge = 1;
		  bd->need_shape_export = 1;
	       }
	     edje_object_signal_emit(bd->bg_object, "shaded", "");
	     ev = calloc(1, sizeof(E_Event_Border_Resize));
	     ev->border = bd;
	     /* The resize is added in the animator when animation complete */
	     /* For non-animated, we add it immediately with the new size */
	     e_object_ref(E_OBJECT(bd));
//	     e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
	     ecore_event_add(E_EVENT_BORDER_RESIZE, ev, _e_border_event_border_resize_free, NULL);
	  }

     }
}

EAPI void
e_border_unshade(E_Border *bd, E_Direction dir)
{
   E_Event_Border_Resize *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((bd->fullscreen) || ((bd->maximized) && (!e_config->allow_manip)) ||
       (bd->shading)) return;
   if (bd->shaded)
     {
//	printf("UNSHADE!\n");

	bd->shade.dir = dir;

	e_hints_window_shaded_set(bd, 0);
	e_hints_window_shade_direction_set(bd, dir);

	if (bd->shade.dir == E_DIRECTION_UP ||
	    bd->shade.dir == E_DIRECTION_LEFT)
	  {
	     bd->shade.x = bd->x;
	     bd->shade.y = bd->y;
	  }
	else
	  {
	     bd->shade.x = bd->x - bd->client.w;
	     bd->shade.y = bd->y - bd->client.h;
	  }
	if (e_config->border_shade_animate)
	  {
	     bd->shade.start = ecore_time_get();
	     bd->shading = 1;
	     bd->changes.shading = 1;
	     bd->changed = 1;

	     if (bd->shade.dir == E_DIRECTION_UP)
	       {
		  ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_SW);
		  ecore_x_window_move_resize(bd->client.win, 0,
					     bd->h - (bd->client_inset.t + bd->client_inset.b) -
					     bd->client.h,
					     bd->client.w, bd->client.h);
	       }
	     else if (bd->shade.dir == E_DIRECTION_LEFT)
	       {
		  ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_SW);
		  ecore_x_window_move_resize(bd->client.win,
					     bd->w - (bd->client_inset.l + bd->client_inset.r) -
					     bd->client.h,
					     0, bd->client.w, bd->client.h);
	       }
	     else
	       ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_NE);

	     bd->shade.anim = ecore_animator_add(_e_border_shade_animator, bd);
	     edje_object_signal_emit(bd->bg_object, "unshading", "");
	  }
	else
	  {
	     if (bd->shade.dir == E_DIRECTION_UP)
	       {
		  bd->h = bd->client_inset.t + bd->client.h + bd->client_inset.b;
	       }
	     else if (bd->shade.dir == E_DIRECTION_DOWN)
	       {
		  bd->h = bd->client_inset.t + bd->client.h + bd->client_inset.b;
		  bd->y = bd->y - bd->client.h;
		  bd->changes.pos = 1;
	       }
	     else if (bd->shade.dir == E_DIRECTION_LEFT)
	       {
		  bd->w = bd->client_inset.l + bd->client.w + bd->client_inset.r;
	       }
	     else if (bd->shade.dir == E_DIRECTION_RIGHT)
	       {
		  bd->w = bd->client_inset.l + bd->client.w + bd->client_inset.r;
		  bd->x = bd->x - bd->client.w;
		  bd->changes.pos = 1;
	       }
	     if ((bd->shaped) || (bd->client.shaped))
	       {
		  bd->need_shape_merge = 1;
		  bd->need_shape_export = 1;
	       }
	     bd->changes.size = 1;
	     bd->shaded = 0;
	     bd->changes.shaded = 1;
	     bd->changed = 1;
	     if ((bd->shaped) || (bd->client.shaped))
	       {
		  bd->need_shape_merge = 1;
		  bd->need_shape_export = 1;
	       }
	     edje_object_signal_emit(bd->bg_object, "unshaded", "");
	     ev = calloc(1, sizeof(E_Event_Border_Resize));
	     ev->border = bd;
	     /* The resize is added in the animator when animation complete */
	     /* For non-animated, we add it immediately with the new size */
	     e_object_ref(E_OBJECT(bd));
//	     e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
	     ecore_event_add(E_EVENT_BORDER_RESIZE, ev, _e_border_event_border_resize_free, NULL);
	  }

     }
}

EAPI void
e_border_maximize(E_Border *bd, E_Maximize max)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (!(max & E_MAXIMIZE_DIRECTION))
     {
	printf("BUG: Maximize call without direction!\n");
	return;
     }

   if ((bd->shaded) || (bd->shading)) return;
   if (bd->fullscreen)
     e_border_unfullscreen(bd);
   /* Only allow changes in vertical/ horisontal maximization */
   if (((bd->maximized & E_MAXIMIZE_DIRECTION) == (max & E_MAXIMIZE_DIRECTION)) || 
       ((bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_BOTH)) return;
   if (bd->new_client)
     {
	bd->need_maximize = 1;
	bd->maximized &= ~E_MAXIMIZE_TYPE;
	bd->maximized |= max;
	return;
     }

     {
	int x1, y1, x2, y2;
	int w, h;

	if (!(bd->maximized & E_MAXIMIZE_HORIZONTAL))
	  {
	     /* Horisontal hasn't been set */
	     bd->saved.x = bd->x; 
	     bd->saved.w = bd->w; 
	  }
	if (!(bd->maximized & E_MAXIMIZE_VERTICAL))
	  {
	     /* Vertical hasn't been set */
	     bd->saved.y = bd->y; 
	     bd->saved.h = bd->h;
	  }
	e_hints_window_size_set(bd);

	e_border_raise(bd);
	switch (max & E_MAXIMIZE_TYPE)
	  {
	   case E_MAXIMIZE_NONE:
	      /* Ignore */
	      max = E_MAXIMIZE_NONE;
	      break;
	   case E_MAXIMIZE_FULLSCREEN:
	      if (bd->bg_object)
		{
		   Evas_Coord cx, cy, cw, ch;

		   edje_object_signal_emit(bd->bg_object, "maximize,fullscreen", "");
		   edje_object_message_signal_process(bd->bg_object);

		   evas_object_resize(bd->bg_object, 1000, 1000);
		   edje_object_calc_force(bd->bg_object);
		   edje_object_part_geometry_get(bd->bg_object, "client", &cx, &cy, &cw, &ch);
		   bd->client_inset.l = cx;
		   bd->client_inset.r = 1000 - (cx + cw);
		   bd->client_inset.t = cy;
		   bd->client_inset.b = 1000 - (cy + ch);
		   ecore_x_netwm_frame_size_set(bd->client.win,
						bd->client_inset.l, bd->client_inset.r,
						bd->client_inset.t, bd->client_inset.b);
		   ecore_x_e_frame_size_set(bd->client.win,
					    bd->client_inset.l, bd->client_inset.r,
					    bd->client_inset.t, bd->client_inset.b);
		}
	      w = bd->zone->w;
	      h = bd->zone->h;
	      /* center x-direction */
	      e_border_resize_limit(bd, &w, &h);
	      x1 = bd->zone->x + (bd->zone->w - w) / 2;
	      /* center y-direction */
	      y1 = bd->zone->y + (bd->zone->h - h) / 2;
	      e_border_move_resize(bd, x1, y1, w, h);
	      /* FULLSCREEN doesn't work with VERTICAL/HORIZONTAL */
	      max |= E_MAXIMIZE_BOTH;
	      break;
	   case E_MAXIMIZE_SMART:
	   case E_MAXIMIZE_EXPAND:
	      x1 = bd->zone->x;
	      y1 = bd->zone->y;
	      x2 = bd->zone->x + bd->zone->w;
	      y2 = bd->zone->y + bd->zone->h;

	      /* walk through all gadgets */
	      e_maximize_border_gadman_fit(bd, &x1, &y1, &x2, &y2);

	      /* walk through docks and toolbars */
	      e_maximize_border_dock_fit(bd, &x1, &y1, &x2, &y2);

	      w = x2 - x1;
	      h = y2 - y1;
	      e_border_resize_limit(bd, &w, &h);
	      if ((max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_BOTH)
		e_border_move_resize(bd, x1, y1, w, h);
	      else if ((max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL)
		e_border_move_resize(bd, bd->x, y1, bd->w, h);
	      else if ((max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL)
		e_border_move_resize(bd, x1, bd->y, w, bd->h);
	      edje_object_signal_emit(bd->bg_object, "maximize", "");
	      break;
	   case E_MAXIMIZE_FILL:
	      x1 = bd->zone->x;
	      y1 = bd->zone->y;
	      x2 = bd->zone->x + bd->zone->w;
	      y2 = bd->zone->y + bd->zone->h;

	      /* walk through all gadgets */
	      e_maximize_border_gadman_fill(bd, &x1, &y1, &x2, &y2);

	      /* walk through all windows */
	      e_maximize_border_border_fill(bd, &x1, &y1, &x2, &y2);

	      w = x2 - x1;
	      h = y2 - y1;
	      e_border_resize_limit(bd, &w, &h);
	      if ((max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_BOTH)
		e_border_move_resize(bd, x1, y1, w, h);
	      else if ((max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL)
		e_border_move_resize(bd, bd->x, y1, bd->w, h);
	      else if ((max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL)
		e_border_move_resize(bd, x1, bd->y, w, bd->h);
	      break;
	  }
	/* Remove previous type */
	bd->maximized &= ~E_MAXIMIZE_TYPE;
	/* Add new maximization. It must be added, so that VERTICAL + HORIZONTAL == BOTH */
	bd->maximized |= max;

	e_hints_window_maximized_set(bd, bd->maximized & E_MAXIMIZE_HORIZONTAL,
	                             bd->maximized & E_MAXIMIZE_VERTICAL);

     }
}

EAPI void
e_border_unmaximize(E_Border *bd, E_Maximize max)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!(max & E_MAXIMIZE_DIRECTION))
     {
	printf("BUG: Unmaximize call without direction!\n");
	return;
     }

   if ((bd->shaded) || (bd->shading)) return;
   /* Remove directions not used */
   max &= (bd->maximized & E_MAXIMIZE_DIRECTION);
   /* Can only remove existing maximization directions */
   if (!max) return;
   if (bd->maximized & E_MAXIMIZE_TYPE)
     {
//	printf("UNMAXIMIZE!!\n");
	E_Maximize dir;
	int signal;

	/* Get the resulting directions */
	dir = (bd->maximized & E_MAXIMIZE_DIRECTION);
	dir &= ~max;

	bd->need_maximize = 0;

	signal = 1;
	switch (bd->maximized & E_MAXIMIZE_TYPE)
	  {
	   case E_MAXIMIZE_NONE:
	      /* Ignore */
	      break;
	   case E_MAXIMIZE_FULLSCREEN:
	      if (bd->bg_object)
		{
		   Evas_Coord cx, cy, cw, ch;

		   edje_object_signal_emit(bd->bg_object, "unmaximize,fullscreen", "");
		   signal = 0;
		   edje_object_message_signal_process(bd->bg_object);

		   evas_object_resize(bd->bg_object, 1000, 1000);
		   edje_object_calc_force(bd->bg_object);
		   edje_object_part_geometry_get(bd->bg_object, "client", &cx, &cy, &cw, &ch);
		   bd->client_inset.l = cx;
		   bd->client_inset.r = 1000 - (cx + cw);
		   bd->client_inset.t = cy;
		   bd->client_inset.b = 1000 - (cy + ch);
		   ecore_x_netwm_frame_size_set(bd->client.win,
						bd->client_inset.l, bd->client_inset.r,
						bd->client_inset.t, bd->client_inset.b);
		   ecore_x_e_frame_size_set(bd->client.win,
					    bd->client_inset.l, bd->client_inset.r,
					    bd->client_inset.t, bd->client_inset.b);
		}
	      dir = 0;
	      break;
	   case E_MAXIMIZE_SMART:
	      /* Don't have to do anything special */
	      break;
	   case E_MAXIMIZE_EXPAND:
	      /* Ignore */
	      break;
	   case E_MAXIMIZE_FILL:
	      /* Ignore */
	      break;
	   case E_MAXIMIZE_VERTICAL:
	      /*Ignore*/
	     break;
	   case E_MAXIMIZE_HORIZONTAL:
	      /*Ignore*/
	     break;
	  }
	if (dir & E_MAXIMIZE_HORIZONTAL)
	  {
	     /* Remove vertical */
	     signal = 0;
	     bd->maximized &= ~E_MAXIMIZE_VERTICAL;
	     e_border_move_resize(bd, bd->x, bd->saved.y, bd->w, bd->saved.h);
	     bd->saved.y = bd->saved.h = 0;
	     e_hints_window_size_set(bd);
	  }
	else if (dir & E_MAXIMIZE_VERTICAL)
	  {
	     /* Remove horisontal */
	     signal = 0;
	     bd->maximized &= ~E_MAXIMIZE_HORIZONTAL;
	     e_border_move_resize(bd, bd->saved.x, bd->y, bd->saved.w, bd->h);
	     bd->saved.x = bd->saved.w = 0;
	     e_hints_window_size_set(bd);
	  }
	else
	  {
	     int x, y, w, h;
	     /* Maybe some of the sizes has already been set to 0 */
	     if (bd->saved.x) x = bd->saved.x;
	     else x = bd->x;
	     if (bd->saved.y) y = bd->saved.y;
	     else y = bd->y;
	     if (bd->saved.w) w = bd->saved.w;
	     else w = bd->w;
	     if (bd->saved.h) h = bd->saved.h;
	     else h = bd->h;

	     bd->maximized = E_MAXIMIZE_NONE;
	     e_border_move_resize(bd, x, y, w, h);
	     bd->saved.x = bd->saved.y = bd->saved.w = bd->saved.h = 0;
	     e_hints_window_size_unset(bd);
	  }
	e_hints_window_maximized_set(bd, bd->maximized & E_MAXIMIZE_HORIZONTAL,
	                             bd->maximized & E_MAXIMIZE_VERTICAL);

	if (signal)
	  edje_object_signal_emit(bd->bg_object, "unmaximize", "");
     }
}

EAPI void
e_border_fullscreen(E_Border *bd, E_Fullscreen policy)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((bd->shaded) || (bd->shading)) return;
   if (bd->maximized)
     e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
   if (bd->new_client)
     {
	bd->need_fullscreen = 1;
	return;
     }
   if (!bd->fullscreen)
     {
#if 0
	int x, y, w, h;
#endif

	bd->saved.x = bd->x;
	bd->saved.y = bd->y;
	bd->saved.w = bd->w;
	bd->saved.h = bd->h;
	e_hints_window_size_set(bd);

	bd->client_inset.sl = bd->client_inset.l;
	bd->client_inset.sr = bd->client_inset.r;
	bd->client_inset.st = bd->client_inset.t;
	bd->client_inset.sb = bd->client_inset.b;
	bd->client_inset.l = 0;
	bd->client_inset.r = 0;
	bd->client_inset.t = 0;
	bd->client_inset.b = 0;

	/* e_zone_fullscreen_set(bd->zone, 1); */

	e_border_layer_set(bd, 200);
#if 0
	x = bd->zone->x;
	y = bd->zone->y;
	w = bd->zone->w;
	h = bd->zone->h;
	e_border_resize_limit(bd, &w, &h);
	/* center */
	x = x + (bd->zone->w - w) / 2;
	y = y + (bd->zone->h - h) / 2;
	e_border_move_resize(bd, x, y, w, h);
#endif
	if ((evas_list_count(bd->zone->container->zones) > 1) || (policy == E_FULLSCREEN_RESIZE))
	  {
	     e_border_move_resize(bd, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
	  }
	else if (policy == E_FULLSCREEN_ZOOM)
	  {
	     Ecore_X_Screen_Size *sizes;
	     int                  num_sizes, i;

	     screen_size = ecore_x_randr_current_screen_size_get(bd->zone->container->manager->root);
	     sizes = ecore_x_randr_screen_sizes_get(bd->zone->container->manager->root, &num_sizes);
	     if (sizes)
	       {
		  Ecore_X_Screen_Size best_size = { -1, -1 };
		  int best_dist = INT_MAX, dist;

		  for (i = 0; i < num_sizes; i++)
		    {
		       if ((sizes[i].width > bd->w) && (sizes[i].height > bd->h))
			 {
			    dist = (sizes[i].width * sizes[i].height) - (bd->w * bd->h);
			    if (dist < best_dist)
			      {
				 best_size = sizes[i];
				 best_dist = dist;
			      }
			 }
		    }
		  if (((best_size.width != -1) && (best_size.height != -1)) &&
		      ((best_size.width != screen_size.width) ||
		       (best_size.height != screen_size.height)))
		    {
		       ecore_x_randr_screen_size_set(bd->zone->container->manager->root,
						     best_size);
		       e_border_move_resize(bd, 0, 0, best_size.width, best_size.height);
		    }
		  else
		    {
		       screen_size.width = -1;
		       screen_size.height = -1;
		       e_border_move_resize(bd, 0, 0, bd->zone->w, bd->zone->h);
		    }
		  free(sizes);
	       }
	     else
	       e_border_move_resize(bd, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
	  }
	ecore_evas_hide(bd->bg_ecore_evas);

	bd->fullscreen = 1;

	e_hints_window_fullscreen_set(bd, 1);
	e_hints_window_size_unset(bd);
	edje_object_signal_emit(bd->bg_object, "fullscreen", "");
     }
}

EAPI void
e_border_unfullscreen(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((bd->shaded) || (bd->shading)) return;
   if (bd->fullscreen)
     {
//	printf("UNFULLSCREEEN!\n");
	bd->fullscreen = 0;
	bd->need_fullscreen = 0;
	bd->client_inset.l = bd->client_inset.sl;
	bd->client_inset.r = bd->client_inset.sr;
	bd->client_inset.t = bd->client_inset.st;
	bd->client_inset.b = bd->client_inset.sb;

	/* e_zone_fullscreen_set(bd->zone, 0); */

	if ((screen_size.width != -1) && (screen_size.height != -1))
	  {
	     ecore_x_randr_screen_size_set(bd->zone->container->manager->root, screen_size);
	     screen_size.width = -1;
	     screen_size.height = -1;
	  }
	e_border_move_resize(bd, bd->saved.x, bd->saved.y, bd->saved.w, bd->saved.h);
	ecore_evas_show(bd->bg_ecore_evas);

	/* FIXME: Find right layer */
	e_border_layer_set(bd, 100);

	e_hints_window_fullscreen_set(bd, 0);
	edje_object_signal_emit(bd->bg_object, "unfullscreen", "");
     }
}

EAPI void
e_border_iconify(E_Border *bd)
{
   E_Event_Border_Iconify *ev;
   unsigned int iconic;
   
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((bd->fullscreen) || (bd->shading)) return;
   if (!bd->iconic)
     {
	bd->iconic = 1;
	e_border_hide(bd, 1);
	edje_object_signal_emit(bd->bg_object, "iconify", "");
     }
   iconic = 1;
   e_hints_window_iconic_set(bd);
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &iconic, 1);

   ev = E_NEW(E_Event_Border_Iconify, 1);
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_iconify_event");
   ecore_event_add(E_EVENT_BORDER_ICONIFY, ev, _e_border_event_border_iconify_free, NULL);

   if (e_config->transient.iconify)
     {
	Evas_List *l;

	for (l = bd->transients; l; l = l->next)
	  {
	     E_Border *child;

	     child = l->data;
	     e_border_iconify(child);
	  }
     }
}

EAPI void
e_border_uniconify(E_Border *bd)
{
   E_Desk *desk;
   E_Event_Border_Uniconify *ev;
   unsigned int iconic;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((bd->fullscreen) || (bd->shading)) return;
   if (bd->iconic)
     {
	bd->iconic = 0;
	desk = e_desk_current_get(bd->desk->zone);
	e_border_desk_set(bd, desk);
	e_border_show(bd);
	e_border_raise(bd);
	edje_object_signal_emit(bd->bg_object, "uniconify", "");
     }
   iconic = 0;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &iconic, 1);

   ev = E_NEW(E_Event_Border_Uniconify, 1);
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_uniconify_event");
   ecore_event_add(E_EVENT_BORDER_UNICONIFY, ev, _e_border_event_border_uniconify_free, NULL);

   if (e_config->transient.iconify)
     {
	Evas_List *l;

	for (l = bd->transients; l; l = l->next)
	  {
	     E_Border *child;

	     child = l->data;
	     e_border_uniconify(child);
	  }
     }
}

EAPI void
e_border_stick(E_Border *bd)
{
   E_Event_Border_Stick *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->sticky) return;
//   printf("STICK!\n");
   bd->sticky = 1;
   e_hints_window_sticky_set(bd, 1);
   e_border_show(bd);

   ev = E_NEW(E_Event_Border_Stick, 1);
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_stick_event");
   ecore_event_add(E_EVENT_BORDER_STICK, ev, _e_border_event_border_stick_free, NULL);
}

EAPI void
e_border_unstick(E_Border *bd)
{
   E_Event_Border_Unstick *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   /* Set the desk before we unstick the border */
   if (!bd->sticky) return;
//   printf("UNSTICK!\n");
   bd->sticky = 0;
   e_hints_window_sticky_set(bd, 0);

   ev = E_NEW(E_Event_Border_Unstick, 1);
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
//   e_object_breadcrumb_add(E_OBJECT(bd), "border_unstick_event");
   ecore_event_add(E_EVENT_BORDER_UNSTICK, ev, _e_border_event_border_unstick_free, NULL);

   e_border_desk_set(bd, e_desk_current_get(bd->zone));
}

EAPI E_Border *
e_border_find_by_client_window(Ecore_X_Window win)
{
   E_Border *bd;
   
   bd = evas_hash_find(borders_hash, _e_border_winid_str_get(win));
   if ((bd) && (!e_object_is_del(E_OBJECT(bd))) &&
       (bd->client.win == win))
     return bd;
   return NULL;
}

EAPI E_Border *
e_border_find_by_frame_window(Ecore_X_Window win)
{
   E_Border *bd;
   
   bd = evas_hash_find(borders_hash, _e_border_winid_str_get(win));
   if ((bd) && (!e_object_is_del(E_OBJECT(bd))) &&
       (bd->bg_win == win))
     return bd;
   return NULL;
}

EAPI E_Border *
e_border_find_by_window(Ecore_X_Window win)
{
   E_Border *bd;
   
   bd = evas_hash_find(borders_hash, _e_border_winid_str_get(win));
   if ((bd) && (!e_object_is_del(E_OBJECT(bd))) &&
       (bd->win == win))
     return bd;
   return NULL;
}

EAPI E_Border *
e_border_find_by_alarm(Ecore_X_Sync_Alarm alarm)
{
   Evas_List *l;
   
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;

	bd = l->data;
	if ((bd) && (!e_object_is_del(E_OBJECT(bd))) &&
	    (bd->client.netwm.sync.alarm == alarm))
	  return bd;
     }
   return NULL;
}

EAPI E_Border *
e_border_focused_get(void)
{
   return focused;
}

EAPI void
e_border_idler_before(void)
{
   Evas_List *ml, *cl;

   if (!borders)
     return;

   /* We need to loop two times through the borders.
    * 1. show windows
    * 2. hide windows and evaluate rest
    */
   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	E_Manager *man;

	man = ml->data;
	for (cl = man->containers; cl; cl = cl->next)
	  {
	     E_Container *con;
	     E_Border_List *bl;
	     E_Border *bd;

	     con = cl->data;
	     bl = e_container_border_list_last(con);
	     while ((bd = e_container_border_list_prev(bl)))
	       {
		  if ((bd->changes.visible) && (bd->visible) && 
		      (!bd->new_client))
		    {
		       ecore_evas_show(bd->bg_ecore_evas);
		       ecore_x_window_show(bd->win);
		       bd->changes.visible = 0;
		    }
	       }
	     e_container_border_list_free(bl);
	  }
     }

   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	E_Manager *man;

	man = ml->data;
	for (cl = man->containers; cl; cl = cl->next)
	  {
	     E_Container *con;
	     E_Border_List *bl;
	     E_Border *bd;

	     con = cl->data;
	     bl = e_container_border_list_first(con);
	     while ((bd = e_container_border_list_next(bl)))
	       {
		  if ((bd->changes.visible) && (!bd->visible))
		    {
		       ecore_x_window_hide(bd->win);
		       ecore_evas_hide(bd->bg_ecore_evas);
		       bd->changes.visible = 0;
		    }
		  if (bd->changed) _e_border_eval(bd);
	       }
	     e_container_border_list_free(bl);
	  }
     }
}

EAPI Evas_List *
e_border_client_list()
{
   /* FIXME: This should be a somewhat ordered list */
   return borders;
}

EAPI void
e_border_act_move_begin(E_Border *bd, Ecore_X_Event_Mouse_Button_Down *ev)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->lock_user_location) return;
   if (!bd->moving)
     {
	if (!_e_border_move_begin(bd))
	  return;

	e_zone_flip_win_disable();
	bd->moving = 1;
	_e_border_pointer_move_begin(bd);
	if (ev)
	  {
	     char source[256];
	     
	     snprintf(source, sizeof(source) - 1, "mouse,down,%i", ev->button);
	     _e_border_moveinfo_gather(bd, source);
	  }
     }
}

EAPI void
e_border_act_move_end(E_Border *bd, Ecore_X_Event_Mouse_Button_Up *ev)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!bd->moving) return;
   bd->moving = 0;
   _e_border_pointer_move_end(bd);
   e_zone_flip_win_restore();
   _e_border_move_end(bd);
   e_zone_flip_coords_handle(bd->zone, -1, -1);
}

EAPI void
e_border_act_resize_begin(E_Border *bd, Ecore_X_Event_Mouse_Button_Down *ev)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->lock_user_size) return;
   if (bd->resize_mode == RESIZE_NONE)
     {
	if (!_e_border_resize_begin(bd))
	  return;
	if (bd->mouse.current.mx < (bd->x + bd-> w / 2))
	  {
	     if (bd->mouse.current.my < (bd->y + bd->h / 2))
	       {
		  bd->resize_mode = RESIZE_TL;
		  GRAV_SET(bd, ECORE_X_GRAVITY_SE);
	       }
	     else
	       {
		  bd->resize_mode = RESIZE_BL;
		  GRAV_SET(bd, ECORE_X_GRAVITY_NE);
	       }
	  }
	else
	  {
	     if (bd->mouse.current.my < (bd->y + bd->h / 2))
	       {
		  bd->resize_mode = RESIZE_TR;
		  GRAV_SET(bd, ECORE_X_GRAVITY_SW);
	       }
	     else
	       {
		  bd->resize_mode = RESIZE_BR;
		  GRAV_SET(bd, ECORE_X_GRAVITY_NW);
	       }
	  }
	_e_border_pointer_resize_begin(bd);
	if (ev)
	  {
	     char source[256];
	     
	     snprintf(source, sizeof(source) - 1, "mouse,down,%i", ev->button);
	     _e_border_moveinfo_gather(bd, source);
	  }
     }
}

EAPI void
e_border_act_resize_end(E_Border *bd, Ecore_X_Event_Mouse_Button_Up *ev)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->resize_mode != RESIZE_NONE)
     {
	_e_border_pointer_resize_end(bd);
	bd->resize_mode = RESIZE_NONE;
	_e_border_resize_end(bd);
	bd->changes.reset_gravity = 1;
	bd->changed = 1;
     }
}

EAPI void
e_border_act_menu_begin(E_Border *bd, Ecore_X_Event_Mouse_Button_Down *ev, int key)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (ev)
     {
	e_int_border_menu_show(bd,
			       bd->x + ev->x - bd->zone->container->x,
			       bd->y + ev->y - bd->zone->container->y, key,
			       ev->time);
     }
   else
     {
	int x, y;
	
	ecore_x_pointer_xy_get(bd->zone->container->win, &x, &y);
	e_int_border_menu_show(bd, x, y, key, 0);
     }
}

EAPI void
e_border_act_close_begin(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->lock_close) return;
   if (bd->client.icccm.delete_request)
     {
	bd->delete_requested = 1;
	ecore_x_window_delete_request_send(bd->client.win);
	if (bd->client.netwm.ping)
	  e_border_ping(bd);
     }
   else if (e_config->kill_if_close_not_possible)
     e_border_act_kill_begin(bd);
}

EAPI void
e_border_act_kill_begin(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->internal) return;
   if (bd->lock_close) return;
   if ((bd->client.netwm.pid > 1) && (e_config->kill_process))
     {
	kill(bd->client.netwm.pid, SIGINT);
	bd->kill_timer = ecore_timer_add(e_config->kill_timer_wait,
					 _e_border_cb_kill_timer, bd);
     }
   else
     {
	if (!bd->internal) ecore_x_kill(bd->client.win);
     }
   e_border_hide(bd, 0);
}

/* FIXME: Prefer app icon or own icon? */
EAPI Evas_Object *
e_border_icon_add(E_Border *bd, Evas *evas)
{
   Evas_Object *o;
   E_App *a = NULL;
   
   E_OBJECT_CHECK_RETURN(bd, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, NULL);
   if (bd->app)
     {
	e_object_unref(E_OBJECT(bd->app));
	bd->app = NULL;
     }

   o = NULL;
   if (bd->internal)
     {
	o = edje_object_add(evas);
	e_util_edje_icon_set(o, "enlightenment/e");
	return o;
     }
   if (e_config->use_app_icon)
     {
	if (bd->client.netwm.icons)
	  {
	     o = e_icon_add(evas);
	     e_icon_data_set(o, bd->client.netwm.icons[0].data,
			     bd->client.netwm.icons[0].width,
			     bd->client.netwm.icons[0].height);
	     e_icon_alpha_set(o, 1);
	     return o;
	  }
     }
   if (!o)
     {
	if ((bd->client.icccm.name) && (bd->client.icccm.class))
	  {
	     char *title = "";
	     
	     if (bd->client.netwm.name) title = bd->client.netwm.name;
	     else title = bd->client.icccm.title;
	     a = e_app_window_name_class_title_role_find(bd->client.icccm.name,
							 bd->client.icccm.class,
							 title,
							 bd->client.icccm.window_role);
	  }
	if (!a)
	  {
	     a = e_app_launch_id_pid_find(bd->client.netwm.startup_id,
					  bd->client.netwm.pid);
	  }
	if (a)
	  {
	     o = e_app_icon_add(evas, a);
	     bd->app = a;
	     e_object_ref(E_OBJECT(bd->app));
	     return o;
	  }
	else if (bd->client.netwm.icons)
	  {
	     o = e_icon_add(evas);
	     e_icon_data_set(o, bd->client.netwm.icons[0].data,
			     bd->client.netwm.icons[0].width,
			     bd->client.netwm.icons[0].height);
	     e_icon_alpha_set(o, 1);
	     return o;
	  }
     }
   if (!o)
     {
	o = edje_object_add(evas);
	e_util_edje_icon_set(o, "enlightenment/unknown");
	return o;
     }
   return o;
}

EAPI void
e_border_button_bindings_ungrab_all(void)
{
   Evas_List *l;
   
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;
	
	bd = l->data;
	e_focus_setdown(bd);
	e_bindings_mouse_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
	e_bindings_wheel_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
     }
}

EAPI void
e_border_button_bindings_grab_all(void)
{
   Evas_List *l;
   
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;
	
	bd = l->data;
	e_bindings_mouse_grab(E_BINDING_CONTEXT_BORDER, bd->win);
	e_bindings_wheel_grab(E_BINDING_CONTEXT_BORDER, bd->win);
	e_focus_setup(bd);
     }
}

EAPI Evas_List *
e_border_focus_stack_get(void)
{
   return focus_stack;
}

EAPI Evas_List *
e_border_lost_windows_get(E_Zone *zone)
{
   Evas_List *list = NULL, *l;
   int loss_overlap = 5;
   
   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;
	
	bd = l->data;
	if (bd->zone)
	  {
	     if ((bd->zone == zone) ||
		 (bd->zone->container == zone->container))
	       {
		  if (!E_INTERSECTS(bd->zone->x + loss_overlap, 
				    bd->zone->y + loss_overlap,
				    bd->zone->w - (2 * loss_overlap),
				    bd->zone->h - (2 * loss_overlap),
				    bd->x, bd->y, bd->w, bd->h))
		    {
		       list = evas_list_append(list, bd);
		    }
		  else if ((!E_CONTAINS(bd->zone->x, bd->zone->y,
					bd->zone->w, bd->zone->h,
					bd->x, bd->y, bd->w, bd->h)) &&
			   (bd->shaped))
		    {
		       Ecore_X_Rectangle *rect;
		       int i, num;
		       
		       rect = ecore_x_window_shape_rectangles_get(bd->win, &num);
		       if (rect)
			 {
			    int ok;
			    
			    ok = 0;
			    for (i = 0; i < num; i++)
			      {
				 if (E_INTERSECTS(bd->zone->x + loss_overlap,
						  bd->zone->y + loss_overlap,
						  bd->zone->w - (2 * loss_overlap),
						  bd->zone->h - (2 * loss_overlap),
						  rect[i].x, rect[i].y,
						  rect[i].width, rect[i].height))
				   {
				      ok = 1;
				      break;
				   }
			      }
			    free(rect);
			    if (!ok)
			      list = evas_list_append(list, bd);
			 }
		    }
	       }
	  }
     }
   return list;
}

EAPI void
e_border_ping(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!e_config->ping_clients) return;
   bd->ping_ok = 0;
   ecore_x_netwm_ping_send(bd->client.win);
   bd->ping = ecore_time_get();
   if (bd->ping_timer) ecore_timer_del(bd->ping_timer);
   bd->ping_timer = ecore_timer_add(e_config->ping_clients_wait,
				    _e_border_cb_ping_timer, bd);
}

EAPI void
e_border_move_cancel(void)
{
   if (move) _e_border_move_end(move);
}

EAPI void
e_border_resize_cancel(void)
{
   if (resize)
     {
	resize->resize_mode = RESIZE_NONE;
	_e_border_resize_end(resize);
     }
}

EAPI void
e_border_frame_recalc(E_Border *bd)
{
   Evas_Coord cx, cy, cw, ch;

   if (!bd->bg_object) return;

   bd->w -= (bd->client_inset.l + bd->client_inset.r);
   bd->h -= (bd->client_inset.t + bd->client_inset.b);

   evas_object_resize(bd->bg_object, 1000, 1000);
   edje_object_calc_force(bd->bg_object);
   edje_object_part_geometry_get(bd->bg_object, "client", &cx, &cy, &cw, &ch);
   bd->client_inset.l = cx;
   bd->client_inset.r = 1000 - (cx + cw);
   bd->client_inset.t = cy;
   bd->client_inset.b = 1000 - (cy + ch);
   ecore_x_netwm_frame_size_set(bd->client.win,
				bd->client_inset.l, bd->client_inset.r,
				bd->client_inset.t, bd->client_inset.b);
   ecore_x_e_frame_size_set(bd->client.win,
			    bd->client_inset.l, bd->client_inset.r,
			    bd->client_inset.t, bd->client_inset.b);

   bd->w += (bd->client_inset.l + bd->client_inset.r);
   bd->h += (bd->client_inset.t + bd->client_inset.b);

   bd->changed = 1;
   bd->changes.size = 1;
   if ((bd->shaped) || (bd->client.shaped))
     {
	bd->need_shape_merge = 1;
	bd->need_shape_export = 1;
     }
   if (bd->internal_ecore_evas)
     ecore_evas_managed_move(bd->internal_ecore_evas,
			     bd->x + bd->client_inset.l,
			     bd->y + bd->client_inset.t);
   ecore_x_icccm_move_resize_send(bd->client.win,
				  bd->x + bd->client_inset.l,
				  bd->y + bd->client_inset.t,
				  bd->client.w,
				  bd->client.h);
}

EAPI Evas_List *
e_border_immortal_windows_get(void)
{
   Evas_List *list = NULL, *l;
   
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;
	
	bd = l->data;
	if (bd->lock_life)
	  list = evas_list_append(list, bd);
     }
   return list;
}

EAPI const char *
e_border_name_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, "");
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, "");
   if (bd->client.netwm.name)
     return bd->client.netwm.name;
   else if (bd->client.icccm.title)
     return bd->client.icccm.title;
   return "";
}


EAPI void
e_border_signal_move_begin(E_Border *bd, const char *sig, const char *src)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!_e_border_move_begin(bd)) return;
   bd->moving = 1;
   _e_border_pointer_move_begin(bd);
   e_zone_flip_win_disable();
   _e_border_moveinfo_gather(bd, sig);
}

EAPI void
e_border_signal_move_end(E_Border *bd, const char *sig, const char *src)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!bd->moving) return;
   bd->moving = 0;
   _e_border_pointer_move_end(bd);
   e_zone_flip_win_restore();
   _e_border_move_end(bd);
   e_zone_flip_coords_handle(bd->zone, -1, -1);
}

EAPI int
e_border_resizing_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, 0);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, 0);
   if (bd->resize_mode == RESIZE_NONE) return 0;
   return 1;
}

EAPI void
e_border_signal_resize_begin(E_Border *bd, const char *dir, const char *sig, const char *src)
{
   Ecore_X_Gravity grav = ECORE_X_GRAVITY_NW;
   int resize_mode = RESIZE_BR;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!_e_border_resize_begin(bd))
     return;
   if (!strcmp(dir, "tl"))
     {
	resize_mode = RESIZE_TL;
	grav = ECORE_X_GRAVITY_SE;
     }
   else if (!strcmp(dir, "t"))
     {
	resize_mode = RESIZE_T;
	grav = ECORE_X_GRAVITY_S;
     }
   else if (!strcmp(dir, "tr"))
     {
	resize_mode = RESIZE_TR;
	grav = ECORE_X_GRAVITY_SW;
     }
   else if (!strcmp(dir, "r"))
     {
	resize_mode = RESIZE_R;
	grav = ECORE_X_GRAVITY_W;
     }
   else if (!strcmp(dir, "br"))
     {
	resize_mode = RESIZE_BR;
	grav = ECORE_X_GRAVITY_NW;
     }
   else if (!strcmp(dir, "b"))
     {
	resize_mode = RESIZE_B;
	grav = ECORE_X_GRAVITY_N;
     }
   else if (!strcmp(dir, "bl"))
     {
	resize_mode = RESIZE_BL;
	grav = ECORE_X_GRAVITY_NE;
     }
   else if (!strcmp(dir, "l"))
     {
	resize_mode = RESIZE_L;
	grav = ECORE_X_GRAVITY_E;
     }
   bd->resize_mode = resize_mode;
   _e_border_pointer_resize_begin(bd);
   _e_border_moveinfo_gather(bd, sig);
   GRAV_SET(bd, grav);
}

EAPI void
e_border_signal_resize_end(E_Border *bd, const char *dir, const char *sig, const char *src)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->resize_mode == RESIZE_NONE) return;
   _e_border_resize_handle(bd);
   _e_border_pointer_resize_end(bd);
   bd->resize_mode = RESIZE_NONE;
   _e_border_resize_end(bd);
   bd->changes.reset_gravity = 1;
   bd->changed = 1;
}

EAPI void
e_border_resize_limit(E_Border *bd, int *w, int *h)
{
   double a;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   *w -= bd->client_inset.l + bd->client_inset.r;
   *h -= bd->client_inset.t + bd->client_inset.b;
   if (*h < 1) *h = 1;
   if (*w < 1) *w = 1;
   if ((bd->client.icccm.base_w >= 0) &&
       (bd->client.icccm.base_h >= 0))
     {
	int tw, th;
	
	tw = *w - bd->client.icccm.base_w;
	th = *h - bd->client.icccm.base_h;
	if (tw < 1) tw = 1;
	if (th < 1) th = 1;
	a = (double)(tw) / (double)(th);
	if ((bd->client.icccm.min_aspect != 0.0) &&
	    (a < bd->client.icccm.min_aspect))
	  {
	     th = tw / bd->client.icccm.max_aspect;
	     *h = th + bd->client.icccm.base_h;
	  }
	else if ((bd->client.icccm.max_aspect != 0.0) &&
		 (a > bd->client.icccm.max_aspect))
	  {
	     tw = th * bd->client.icccm.max_aspect;
	     *w = tw + bd->client.icccm.base_w;
	  }
     }
   else
     {
	a = (double)*w / (double)*h;
	if ((bd->client.icccm.min_aspect != 0.0) &&
	    (a < bd->client.icccm.min_aspect))
	  *h = *w / bd->client.icccm.min_aspect;
	else if ((bd->client.icccm.max_aspect != 0.0) &&
		 (a > bd->client.icccm.max_aspect))
	  *w = *h * bd->client.icccm.max_aspect;
     }
   if (bd->client.icccm.step_w > 0)
     {
	if (bd->client.icccm.base_w >= 0)
	  *w = bd->client.icccm.base_w +
	  (((*w - bd->client.icccm.base_w) / bd->client.icccm.step_w) *
	   bd->client.icccm.step_w);
	else
	  *w = bd->client.icccm.min_w +
	  (((*w - bd->client.icccm.min_w) / bd->client.icccm.step_w) *
	   bd->client.icccm.step_w);
     }
   if (bd->client.icccm.step_h > 0)
     {
	if (bd->client.icccm.base_h >= 0)
	  *h = bd->client.icccm.base_h +
	  (((*h - bd->client.icccm.base_h) / bd->client.icccm.step_h) *
	   bd->client.icccm.step_h);
	else
	  *h = bd->client.icccm.min_h +
	  (((*h - bd->client.icccm.min_h) / bd->client.icccm.step_h) *
	   bd->client.icccm.step_h);
     }

   if (*h < 1) *h = 1;
   if (*w < 1) *w = 1;

   if      (*w > bd->client.icccm.max_w) *w = bd->client.icccm.max_w;
   else if (*w < bd->client.icccm.min_w) *w = bd->client.icccm.min_w;
   if      (*h > bd->client.icccm.max_h) *h = bd->client.icccm.max_h;
   else if (*h < bd->client.icccm.min_h) *h = bd->client.icccm.min_h;

   *w += bd->client_inset.l + bd->client_inset.r;
   *h += bd->client_inset.t + bd->client_inset.b;
}

/* local subsystem functions */
static void
_e_border_free(E_Border *bd)
{
   if (bd->focused)
     {
	if (e_config->focus_revert_on_hide_or_close)
	  e_desk_last_focused_focus(bd->desk);
     }
   if (resize == bd)
     _e_border_resize_end(bd);
   if (move == bd)
     _e_border_move_end(bd);
   /* TODO: Other states to end before dying? */

   if (bd->cur_mouse_action)
     {
	e_object_unref(E_OBJECT(bd->cur_mouse_action));
	bd->cur_mouse_action = NULL;
     }
   if (bd->app)
     {
	e_object_unref(E_OBJECT(bd->app));
	bd->app = NULL;
     }
   
   E_FREE(bd->shape_rects);
   bd->shape_rects_num = 0;
/*   
   if (bd->dangling_ref_check)
     {
	ecore_timer_del(bd->dangling_ref_check);
	bd->dangling_ref_check = NULL;
     }
 */
   if (bd->kill_timer)
     {
	ecore_timer_del(bd->kill_timer);
	bd->kill_timer = NULL;
     }
   if (bd->ping_timer)
     {
	ecore_timer_del(bd->ping_timer);
	bd->ping_timer = NULL;
     }
   while (bd->pending_move_resize)
     {
	free(bd->pending_move_resize->data);
	bd->pending_move_resize = evas_list_remove_list(bd->pending_move_resize, bd->pending_move_resize);
     }

   if (bd->shade.anim) ecore_animator_del(bd->shade.anim);
   if (bd->border_menu) e_menu_deactivate(bd->border_menu);

   if (bd->border_locks_dialog)
     {
	e_object_del(E_OBJECT(bd->border_locks_dialog));
	bd->border_locks_dialog = NULL;
     }
   if (bd->border_remember_dialog)
     {
	e_object_del(E_OBJECT(bd->border_remember_dialog));
	bd->border_remember_dialog = NULL;
     }
   if (bd->border_border_dialog)
     {
	e_object_del(E_OBJECT(bd->border_border_dialog));
	bd->border_border_dialog = NULL;
     }
   
   e_int_border_menu_del(bd);

   if (focused == bd)
     {
//	ecore_x_window_focus(bd->zone->container->manager->root);
	e_grabinput_focus(bd->zone->container->bg_win, E_FOCUS_METHOD_PASSIVE);
	e_hints_active_window_set(bd->zone->container->manager, NULL);
	focused = NULL;
     }
   while (bd->handlers)
     {
	Ecore_Event_Handler *h;

	h = bd->handlers->data;
	bd->handlers = evas_list_remove_list(bd->handlers, bd->handlers);
	ecore_event_handler_del(h);
     }
   if (bd->remember)
     {
	E_Remember *rem;
	
	rem = bd->remember;
	bd->remember = NULL;
	e_remember_unuse(rem);
     }
   if (!bd->already_unparented)
     {
	ecore_x_window_reparent(bd->client.win, bd->zone->container->manager->root,
				bd->x + bd->client_inset.l, bd->y + bd->client_inset.t);
	ecore_x_window_save_set_del(bd->client.win);
	bd->already_unparented = 1;
     }
   if (bd->group) evas_list_free(bd->group);
   if (bd->transients) evas_list_free(bd->transients);
   if (bd->stick_desks) evas_list_free(bd->stick_desks);
   if (bd->client.netwm.icons)
     {
	int i;
	for (i = 0; i < bd->client.netwm.num_icons; i++)
	  free(bd->client.netwm.icons[i].data);
	free(bd->client.netwm.icons);
     }
   if (bd->client.border.name) evas_stringshare_del(bd->client.border.name);
   if (bd->client.icccm.title) free(bd->client.icccm.title);
   if (bd->client.icccm.name) free(bd->client.icccm.name);
   if (bd->client.icccm.class) free(bd->client.icccm.class);
   if (bd->client.icccm.icon_name) free(bd->client.icccm.icon_name);
   if (bd->client.icccm.machine) free(bd->client.icccm.machine);
   if (bd->client.icccm.window_role) free(bd->client.icccm.window_role);
   if ((bd->client.icccm.command.argc > 0) && (bd->client.icccm.command.argv))
     {
	int i;
	
	for (i = 0; i < bd->client.icccm.command.argc; i++)
	  free(bd->client.icccm.command.argv[i]);
	free(bd->client.icccm.command.argv);
     }
   if (bd->client.netwm.name) free(bd->client.netwm.name);
   if (bd->client.netwm.icon_name) free(bd->client.netwm.icon_name);
   e_object_del(E_OBJECT(bd->shape));
   if (bd->icon_object) evas_object_del(bd->icon_object);
   evas_object_del(bd->bg_object);
   e_canvas_del(bd->bg_ecore_evas);
   ecore_evas_free(bd->bg_ecore_evas);
   ecore_x_window_del(bd->client.shell_win);
   e_focus_setdown(bd);
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
   ecore_x_window_del(bd->win);

   borders_hash = evas_hash_del(borders_hash, _e_border_winid_str_get(bd->client.win), bd);
   borders_hash = evas_hash_del(borders_hash, _e_border_winid_str_get(bd->bg_win), bd);
   borders_hash = evas_hash_del(borders_hash, _e_border_winid_str_get(bd->win), bd);
   borders = evas_list_remove(borders, bd);
   focus_stack = evas_list_remove(focus_stack, bd);
   
   free(bd);
}

/*
static int
_e_border_del_dangling_ref_check(void *data)
{
   E_Border *bd;
   
   bd = data;
   printf("---\n");
   printf("EEK EEK border still around 1 second after being deleted!\n");
   printf("%p, %i, \"%s\" [\"%s\" \"%s\"]\n",
	  bd, e_object_ref_get(E_OBJECT(bd)), bd->client.icccm.title,
	  bd->client.icccm.name, bd->client.icccm.class);
//   e_object_breadcrumb_debug(E_OBJECT(bd));
   printf("---\n");
   return 1;
}
*/

static void
_e_border_del(E_Border *bd)
{
   E_Event_Border_Remove *ev;

   if (bd->border_menu) e_menu_deactivate(bd->border_menu);

   if (bd->border_locks_dialog)
     {
	e_object_del(E_OBJECT(bd->border_locks_dialog));
	bd->border_locks_dialog = NULL;
     }
   if (bd->border_remember_dialog)
     {
	e_object_del(E_OBJECT(bd->border_remember_dialog));
	bd->border_remember_dialog = NULL;
     }
   if (bd->border_border_dialog)
     {
	e_object_del(E_OBJECT(bd->border_border_dialog));
	bd->border_border_dialog = NULL;
     }

   e_int_border_menu_del(bd);
   
   if (bd->raise_timer)
     {
	ecore_timer_del(bd->raise_timer);
	bd->raise_timer = NULL;
     }
   if (!bd->already_unparented)
     {
	ecore_x_window_reparent(bd->client.win,
				bd->zone->container->manager->root,
				bd->x + bd->client_inset.l,
				bd->y + bd->client_inset.t);
	ecore_x_window_save_set_del(bd->client.win);
     }
   bd->already_unparented = 1;

   if (!bd->new_client)
     {
	ev = calloc(1, sizeof(E_Event_Border_Remove));
	ev->border = bd;
	/* FIXME Don't ref this during shutdown. And the event is pointless
	 * during shutdown.. */
	e_object_ref(E_OBJECT(bd));
	// e_object_breadcrumb_add(E_OBJECT(bd), "border_remove_event");
	ecore_event_add(E_EVENT_BORDER_REMOVE, ev, _e_border_event_border_remove_free, NULL);
     }

   /* remove all pointers for this win. */
   e_pointer_type_pop(bd->zone->container->manager->pointer, bd, NULL);
   e_container_border_remove(bd);
   if (bd->parent)
     {
	bd->parent->transients = evas_list_remove(bd->parent->transients, bd);
	if (bd->parent->modal == bd)
	  {
	     bd->parent->modal = NULL;
	     if (bd->focused)
	       e_border_focus_set(bd->parent, 1, 1);
	  }
	bd->parent = NULL;
     }
   while (bd->transients)
     {
	E_Border *child;

	child = bd->transients->data;
	child->parent = NULL;
	bd->transients = evas_list_remove_list(bd->transients, bd->transients);
     }

   if (bd->leader)
     {
	bd->leader->group = evas_list_remove(bd->leader->group, bd);
	if (bd->leader->modal == bd)
	  {
	     bd->leader->modal = NULL;
	     /* TODO Should we focus leader when this window closes? */
#if 0
	     if (bd->focused)
	       e_border_focus_set(bd->leader, 1, 1);
#endif
	  }
	bd->leader = NULL;
     }
   while (bd->group)
     {
	E_Border *child;

	child = bd->group->data;
	child->leader = NULL;
	bd->group = evas_list_remove_list(bd->group, bd->group);
     }
}

static int
_e_border_cb_window_show_request(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Show_Request *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
   if (bd->iconic)
     {
	if (!bd->lock_client_iconify)
	  e_border_uniconify(bd);
     }
   else
     {
	/* FIXME: make border "urgent" for a bit - it wants attention */
/*	e_border_show(bd); */
	if (!bd->lock_client_stacking)
	  e_border_raise(bd);
     }
   return 1;
}

static int
_e_border_cb_window_destroy(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Destroy *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
   e_border_hide(bd, 0);
   e_object_del(E_OBJECT(bd));
   return 1;
}

static int
_e_border_cb_window_hide(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Hide *e;

//   printf("in hide cb\n");
   bd = data;
   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
   if (bd->ignore_first_unmap > 0)
     {
	bd->ignore_first_unmap--;
	return 1;
     }
   /* Don't delete hidden or iconified windows */
   if ((bd->iconic) || (!bd->visible) || (bd->await_hide_event > 0))
     {
	if (bd->await_hide_event > 0)
	  bd->await_hide_event--;
	else
	  {
	     /* Only hide the border if it is visible */
	     if (bd->visible) e_border_hide(bd, 1);
	  }
     }
   else
     {
	e_border_hide(bd, 0);
	e_object_del(E_OBJECT(bd));
     }
   return 1;
}

static int
_e_border_cb_window_reparent(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Reparent *e;

   bd = data;
   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
//   if (e->parent == bd->client.shell_win) return 1;
   if (ecore_x_window_parent_get(e->win) == bd->client.shell_win) return 1;
   e_border_hide(bd, 0);
   e_object_del(E_OBJECT(bd));
   return 1;
}

static int
_e_border_cb_window_configure_request(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Configure_Request *e;

   bd = data;
   e = ev;
//   printf("##- CONF REQ 0x%x , %iX%i+%i+%i\n",
//	  e->win, e->w, e->h, e->x, e->y);
   bd = e_border_find_by_client_window(e->win);
   if (!bd)
     {
	if (e_stolen_win_get(e->win)) return 1;
//	printf("generic config request 0x%x 0x%lx %i %i %ix%i %i 0x%x 0x%x...\n",
//	       e->win, e->value_mask, e->x, e->y, e->w, e->h, e->border, e->abovewin, e->detail);
	if (!e_util_container_window_find(e->win))
	  ecore_x_window_configure(e->win, e->value_mask,
				   e->x, e->y, e->w, e->h, e->border,
				   e->abovewin, e->detail);
	return 1;
     }
#if 0
   printf("##- CONFIGURE REQ 0x%0x mask: %c%c%c%c%c%c%c\n",
	  e->win,
	  (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_X) ? 'X':' ',
	  (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_Y) ? 'Y':' ',
	  (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W) ? 'W':' ',
	  (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H) ? 'H':' ',
	  (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_BORDER_WIDTH) ? 'B':' ',
	  (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING) ? 'C':' ',
	  (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE) ? 'S':' '
	  );
#endif

   if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_X) ||
       (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_Y))
     {
	int x, y;

	y = bd->y;
	x = bd->x;
#if 0
	printf("##- ASK FOR 0x%x TO MOVE TO [FLG X%liY%li] %i,%i\n",
	       bd->client.win,
	       e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_X,
	       e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_Y,
	       x, y);
#endif
	if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_X)
	  x = e->x;
	if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_Y)
	  y = e->y;
	if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W) ||
	    (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H))
	  {
	     int w, h;

	     h = bd->h;
	     w = bd->w;
	     if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W)
	       w = e->w + bd->client_inset.l + bd->client_inset.r;
	     if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H)
	       h = e->h + bd->client_inset.t + bd->client_inset.b;
#if 0
	     printf("##- ASK FOR 0x%x TO RESIZE TO [FLG W%liH%li] %i,%i\n",
		    bd->client.win,
		    e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W,
		    e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H,
		    e->w, e->h);
#endif
	     if ((!bd->lock_client_location) && (!bd->lock_client_size))
	       e_border_move_resize(bd, x, y, w, h);
	     else if (!bd->lock_client_location)
	       e_border_move(bd, x, y);
	     else if (!bd->lock_client_size)
	       {
		  if ((bd->shaded) || (bd->shading))
		    {
		       int pw, ph;
		       
		       pw = bd->client.w;
		       ph = bd->client.h;
		       if ((bd->shade.dir == E_DIRECTION_UP) ||
			   (bd->shade.dir == E_DIRECTION_DOWN))
			 {
			    e_border_resize(bd, w, bd->h);
			    bd->client.h = ph;
			 }
		       else
			 {
			    e_border_resize(bd, bd->w, h);
			    bd->client.w = pw;
			 }
		    }
		  else
		    e_border_resize(bd, w, h);
	       }
	  }
	else
	  {
	     if (!bd->lock_client_location)
	       e_border_move(bd, x, y);
	  }
     }
   else if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W) ||
	    (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H))
     {
	int w, h;

	h = bd->h;
	w = bd->w;
	if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W)
	  w = e->w + bd->client_inset.l + bd->client_inset.r;
	if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H)
	  h = e->h + bd->client_inset.t + bd->client_inset.b;
#if 0
	printf("##- ASK FOR 0x%x TO RESIZE TO [FLG W%liH%li] %i,%i\n",
	       bd->client.win,
	       e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W,
	       e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H,
	       e->w, e->h);
#endif
	if (!bd->lock_client_size)
	  {
	     if ((bd->shaded) || (bd->shading))
	       {
		  int pw, ph;
		  
		  pw = bd->client.w;
		  ph = bd->client.h;
		  if ((bd->shade.dir == E_DIRECTION_UP) ||
		      (bd->shade.dir == E_DIRECTION_DOWN))
		    {
		       e_border_resize(bd, w, bd->h);
		       bd->client.h = ph;
		    }
		  else
		    {
		       e_border_resize(bd, bd->w, h);
		       bd->client.w = pw;
		    }
	       }
	     else
	       e_border_resize(bd, w, h);
	  }
     }
   if (!bd->lock_client_stacking)
     {
	if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE) &&
	    (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING))
	  {
	     E_Border *obd;
	     
	     if (e->detail == ECORE_X_WINDOW_STACK_ABOVE)
	       {
		  obd = e_border_find_by_client_window(e->abovewin);
		  if (obd)
		    e_border_stack_above(bd, obd);
	       }
	     else if (e->detail == ECORE_X_WINDOW_STACK_BELOW)
	       {
		  obd = e_border_find_by_client_window(e->abovewin);
		  if (obd)
		    e_border_stack_below(bd, obd);
	       }
	     else if (e->detail == ECORE_X_WINDOW_STACK_TOP_IF)
	       {
		  /* FIXME: do */
	       }
	     else if (e->detail == ECORE_X_WINDOW_STACK_BOTTOM_IF)
	       {
		  /* FIXME: do */
	       }
	     else if (e->detail == ECORE_X_WINDOW_STACK_OPPOSITE)
	       {
		  /* FIXME: do */
	       }
	  }
	else if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE)
	  {
	     if (e->detail == ECORE_X_WINDOW_STACK_ABOVE)
	       {
		  e_border_raise(bd);
	       }
	     else if (e->detail == ECORE_X_WINDOW_STACK_BELOW)
	       {
		  e_border_lower(bd);
	       }
	     else if (e->detail == ECORE_X_WINDOW_STACK_TOP_IF)
	       {
		  /* FIXME: do */
	       }
	     else if (e->detail == ECORE_X_WINDOW_STACK_BOTTOM_IF)
	       {
		  /* FIXME: do */
	       }
	     else if (e->detail == ECORE_X_WINDOW_STACK_OPPOSITE)
	       {
		  /* FIXME: do */
	       }
	  }
     }
   return 1;
}

static int
_e_border_cb_window_resize_request(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Resize_Request *e;

   bd = data;
   e = ev;
//   printf("##- RESZ REQ 0x%x , %iX%i\n",
//	  e->win, e->w, e->h);
   bd = e_border_find_by_client_window(e->win);
   if (!bd)
     {
	if (e_stolen_win_get(e->win)) return 1;
//	printf("generic resize request %x %ix%i ...\n",
//	       e->win, e->w, e->h);
	ecore_x_window_resize(e->win, e->w, e->h);
	return 1;
     }
//   printf("##- RESIZE REQ 0x%x\n", bd->client.win);
     {
	int w, h;

	h = bd->h;
	w = bd->w;
	w = e->w + bd->client_inset.l + bd->client_inset.r;
	h = e->h + bd->client_inset.t + bd->client_inset.b;
//	printf("##- ASK FOR 0x%x TO RESIZE TO %i,%i\n",
//	       bd->client.win, e->w, e->h);
	if ((bd->shaded) || (bd->shading))
	  {
	     int pw, ph;
	     
	     pw = bd->client.w;
	     ph = bd->client.h;
	     if ((bd->shade.dir == E_DIRECTION_UP) ||
		 (bd->shade.dir == E_DIRECTION_DOWN))
	       {
		  e_border_resize(bd, w, bd->h);
		  bd->client.h = ph;
	       }
	     else
	       {
		  e_border_resize(bd, bd->w, h);
		  bd->client.w = pw;
	       }
	  }
	else
	  e_border_resize(bd, w, h);
     }
   return 1;
}

static int
_e_border_cb_window_gravity(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Gravity *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
//   printf("gravity for %0x\n", e->win);
   return 1;
}

static int
_e_border_cb_window_stack_request(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Stack_Request *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
//   printf("stack req for %0x bd %p\n", e->win, bd);
   if (!bd)
     {
	if (e_stolen_win_get(e->win)) return 1;
	if (!e_util_container_window_find(e->win))
	  {
	     if (e->detail == ECORE_X_WINDOW_STACK_ABOVE)
	       ecore_x_window_raise(e->win);
	     else if (e->detail == ECORE_X_WINDOW_STACK_BELOW)
	       ecore_x_window_lower(e->win);
	  }
	return 1;
     }
   if (e->detail == ECORE_X_WINDOW_STACK_ABOVE)
     e_border_raise(bd);
   else if (e->detail == ECORE_X_WINDOW_STACK_BELOW)
     e_border_lower(bd);
   return 1;
}

static int
_e_border_cb_window_property(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Property *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
   if (e->atom == ECORE_X_ATOM_WM_NAME)
     {
	if ((!bd->client.netwm.name) &&
	    (!bd->client.netwm.fetch.name))
	  {
	     bd->client.icccm.fetch.title = 1;
	     bd->changed = 1;
	  }
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_NAME)
     {
	bd->client.netwm.fetch.name = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_CLASS)
     {
	bd->client.icccm.fetch.name_class = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_ICON_NAME)
     {
	if ((!bd->client.netwm.icon_name) &&
	    (!bd->client.netwm.fetch.icon_name))
	  {
	     bd->client.icccm.fetch.icon_name = 1;
	     bd->changed = 1;
	  }
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_ICON_NAME)
     {
	bd->client.netwm.fetch.icon_name = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_CLIENT_MACHINE)
     {
	bd->client.icccm.fetch.machine = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_PROTOCOLS)
     {
	bd->client.icccm.fetch.protocol = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_HINTS)
     {
	bd->client.icccm.fetch.hints = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_NORMAL_HINTS)
     {
	bd->client.icccm.fetch.size_pos_hints = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_MOTIF_WM_HINTS)
     {
	/*
	if ((bd->client.netwm.type == ECORE_X_WINDOW_TYPE_UNKNOWN) &&
	    (!bd->client.netwm.fetch.type))
	  {
	*/
	     bd->client.mwm.fetch.hints = 1;
	     bd->changed = 1;
	/*
	  }
	*/
     }
   else if (e->atom == ECORE_X_ATOM_WM_TRANSIENT_FOR)
     {
	bd->client.icccm.fetch.transient_for = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_CLIENT_LEADER)
     {
	bd->client.icccm.fetch.client_leader = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_WINDOW_ROLE)
     {
	bd->client.icccm.fetch.window_role = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_ICON)
     {
	bd->client.netwm.fetch.icon = 1;
	bd->changed = 1;
     }
   /*
   else if (e->atom == ECORE_X_ATOM_NET_WM_USER_TIME)
     {
	bd->client.netwm.fetch.user_time = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_STRUT)
     {
	bd->client.netwm.fetch.strut = 1;
	bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_STRUT_PARTIAL)
     {
	bd->client.netwm.fetch.strut = 1;
	bd->changed = 1;
     }
   */
   else if (e->atom == ECORE_X_ATOM_NET_WM_SYNC_REQUEST_COUNTER)
     {
	printf("ECORE_X_ATOM_NET_WM_SYNC_REQUEST_COUNTER\n");
     }
   return 1;
}

static int
_e_border_cb_window_colormap(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Colormap *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
   return 1;
}

static int
_e_border_cb_window_shape(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Shape *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (bd)
     {
	bd->changes.shape = 1;
	bd->changed = 1;
	return 1;
     }
   bd = e_border_find_by_window(e->win);
   if (bd)
     {
	bd->need_shape_export = 1;
	bd->changed = 1;
	return 1;
     }
   bd = e_border_find_by_frame_window(e->win);
   if (bd)
     {
	bd->need_shape_merge = 1;
	bd->changed = 1;
	return 1;
     }
   return 1;
}

static int
_e_border_cb_window_focus_in(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Focus_In *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
#ifdef INOUTDEBUG_FOCUS
     {
	time_t t;
	char *ct;

	const char *modes[] = {
	     "MODE_NORMAL",
	     "MODE_WHILE_GRABBED",
	     "MODE_GRAB",
	     "MODE_UNGRAB"
	};
	const char *details[] = {
	     "DETAIL_ANCESTOR",
	     "DETAIL_VIRTUAL",
	     "DETAIL_INFERIOR",
	     "DETAIL_NON_LINEAR",
	     "DETAIL_NON_LINEAR_VIRTUAL",
	     "DETAIL_POINTER",
	     "DETAIL_POINTER_ROOT",
	     "DETAIL_DETAIL_NONE"
	};
	t = time(NULL);
	ct = ctime(&t);
	ct[strlen(ct) - 1] = 0;
	printf("FF ->IN %i 0x%x %s md=%s dt=%s\n",
	       e->time,
	       e->win,
	       ct,
	       modes[e->mode],
	       details[e->detail]);
     }
#endif
   if (e->mode == ECORE_X_EVENT_MODE_GRAB)
     {
	if (e->detail == ECORE_X_EVENT_DETAIL_POINTER) return 1;
     }
   else if (e->mode == ECORE_X_EVENT_MODE_UNGRAB)
     {
	if (e->detail == ECORE_X_EVENT_DETAIL_POINTER) return 1;
     }
   e_border_focus_set(bd, 1, 0);
   return 1;
}

static int
_e_border_cb_window_focus_out(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Focus_Out *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
#ifdef INOUTDEBUG_FOCUS
     {
	time_t t;
	char *ct;

	const char *modes[] = {
	     "MODE_NORMAL",
	     "MODE_WHILE_GRABBED",
	     "MODE_GRAB",
	     "MODE_UNGRAB"
	};
	const char *details[] = {
	     "DETAIL_ANCESTOR",
	     "DETAIL_VIRTUAL",
	     "DETAIL_INFERIOR",
	     "DETAIL_NON_LINEAR",
	     "DETAIL_NON_LINEAR_VIRTUAL",
	     "DETAIL_POINTER",
	     "DETAIL_POINTER_ROOT",
	     "DETAIL_DETAIL_NONE"
	};
	t = time(NULL);
	ct = ctime(&t);
	ct[strlen(ct) - 1] = 0;
	printf("FF <-OUT %i 0x%x %s md=%s dt=%s\n",
	       e->time,
	       e->win,
	       ct,
	       modes[e->mode],
	       details[e->detail]);
     }
#endif   
   if (e->mode == ECORE_X_EVENT_MODE_NORMAL)
     {
	if (e->detail == ECORE_X_EVENT_DETAIL_INFERIOR) return 1;
	else if (e->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR) return 1;
	else if (e->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR_VIRTUAL) return 1;
     }
   else if (e->mode == ECORE_X_EVENT_MODE_GRAB)
     {
	if (e->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR) return 1;
	else if (e->detail == ECORE_X_EVENT_DETAIL_INFERIOR) return 1;
	else if (e->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR_VIRTUAL) return 1;
	else if (e->detail == ECORE_X_EVENT_DETAIL_ANCESTOR) return 1;
	else if (e->detail == ECORE_X_EVENT_DETAIL_VIRTUAL) return 1;
     }
   else if (e->mode == ECORE_X_EVENT_MODE_UNGRAB)
     {
	/* for firefox/thunderbird (xul) menu walking */
	/* NB: why did i disable this before? */
	if (e->detail == ECORE_X_EVENT_DETAIL_INFERIOR) return 1;
	else if (e->detail == ECORE_X_EVENT_DETAIL_POINTER) return 1;
     }
   else if (e->mode == ECORE_X_EVENT_MODE_WHILE_GRABBED)
     {
	if (e->detail == ECORE_X_EVENT_DETAIL_ANCESTOR) return 1;
	else if (e->detail == ECORE_X_EVENT_DETAIL_INFERIOR) return 1;
     }
   e_border_focus_set(bd, 0, 0);
   return 1;
}

static int
_e_border_cb_client_message(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Client_Message *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
   return 1;
}

static int
_e_border_cb_window_state_request(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_State_Request *e;
   int i;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;

   for (i = 0; i < 2; i++)
     e_hints_window_state_update(bd, e->state[i], e->action);

   return 1;
}

static int
_e_border_cb_window_move_resize_request(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Move_Resize_Request *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;

   if ((bd->shaded) || (bd->shading) ||
       (((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN) && (!e_config->allow_manip)) ||
       (bd->fullscreen) || (bd->moving) ||
       (bd->resize_mode != RESIZE_NONE))
     return 1;

   if ((e->button >= 1) && (e->button <= 3))
     {
	bd->mouse.last_down[e->button - 1].mx = e->x;
	bd->mouse.last_down[e->button - 1].my = e->y;
	bd->mouse.last_down[e->button - 1].x = bd->x;
	bd->mouse.last_down[e->button - 1].y = bd->y;
	bd->mouse.last_down[e->button - 1].w = bd->w;
	bd->mouse.last_down[e->button - 1].h = bd->h;
     }
   else
     {
	bd->moveinfo.down.x = bd->x;
	bd->moveinfo.down.y = bd->y;
	bd->moveinfo.down.w = bd->w;
	bd->moveinfo.down.h = bd->h;
     }
   bd->mouse.current.mx = e->x;
   bd->mouse.current.my = e->y;
   bd->moveinfo.down.button = e->button;
   bd->moveinfo.down.mx = e->x;
   bd->moveinfo.down.my = e->y;
   grabbed = 1;

   if (!bd->lock_user_stacking)
     e_border_raise(bd);
   if (e->direction == RESIZE_TL)
     {
	if (!_e_border_resize_begin(bd))
	  return 1;
	bd->resize_mode = RESIZE_TL;

	bd->cur_mouse_action = e_action_find("window_resize");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       e_object_ref(E_OBJECT(bd->cur_mouse_action));
	  }
	GRAV_SET(bd, ECORE_X_GRAVITY_SE);
     }
   else if (e->direction == RESIZE_T)
     {
	if (!_e_border_resize_begin(bd))
	  return 1;
	bd->resize_mode = RESIZE_T;

	bd->cur_mouse_action = e_action_find("window_resize");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       e_object_ref(E_OBJECT(bd->cur_mouse_action));
	  }
	GRAV_SET(bd, ECORE_X_GRAVITY_S);
     }
   else if (e->direction == RESIZE_TR)
     {
	if (!_e_border_resize_begin(bd))
	  return 1;
	bd->resize_mode = RESIZE_TR;

	bd->cur_mouse_action = e_action_find("window_resize");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       e_object_ref(E_OBJECT(bd->cur_mouse_action));
	  }
	GRAV_SET(bd, ECORE_X_GRAVITY_SW);
     }
   else if (e->direction == RESIZE_R)
     {
	if (!_e_border_resize_begin(bd))
	  return 1;
	bd->resize_mode = RESIZE_R;

	bd->cur_mouse_action = e_action_find("window_resize");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       e_object_ref(E_OBJECT(bd->cur_mouse_action));
	  }
	GRAV_SET(bd, ECORE_X_GRAVITY_W);
     }
   else if (e->direction == RESIZE_BR)
     {
	if (!_e_border_resize_begin(bd))
	  return 1;
	bd->resize_mode = RESIZE_BR;

	bd->cur_mouse_action = e_action_find("window_resize");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       e_object_ref(E_OBJECT(bd->cur_mouse_action));
	  }
	GRAV_SET(bd, ECORE_X_GRAVITY_NW);
     }
   else if (e->direction == RESIZE_B)
     {
	if (!_e_border_resize_begin(bd))
	  return 1;
	bd->resize_mode = RESIZE_B;

	bd->cur_mouse_action = e_action_find("window_resize");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       e_object_ref(E_OBJECT(bd->cur_mouse_action));
	  }
	GRAV_SET(bd, ECORE_X_GRAVITY_N);
     }
   else if (e->direction == RESIZE_BL)
     {
	if (!_e_border_resize_begin(bd))
	  return 1;
	bd->resize_mode = RESIZE_BL;

	bd->cur_mouse_action = e_action_find("window_resize");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       e_object_ref(E_OBJECT(bd->cur_mouse_action));
	  }
	GRAV_SET(bd, ECORE_X_GRAVITY_NE);
     }
   else if (e->direction == RESIZE_L)
     {
	if (!_e_border_resize_begin(bd))
	  return 1;
	bd->resize_mode = RESIZE_L;

	bd->cur_mouse_action = e_action_find("window_resize");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       e_object_ref(E_OBJECT(bd->cur_mouse_action));
	  }
	GRAV_SET(bd, ECORE_X_GRAVITY_E);
     }
   else if (e->direction == MOVE)
     {
	bd->cur_mouse_action = e_action_find("window_move");
	if (bd->cur_mouse_action)
	  {
	     if ((!bd->cur_mouse_action->func.end_mouse) &&
		 (!bd->cur_mouse_action->func.end))
	       bd->cur_mouse_action = NULL;
	     if (bd->cur_mouse_action)
	       {
		  e_object_ref(E_OBJECT(bd->cur_mouse_action));
		  bd->cur_mouse_action->func.go(E_OBJECT(bd), NULL); 
	       }
	  }
     }
   return 1;
}

static int
_e_border_cb_desktop_change(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Desktop_Change *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (bd)
     {
	if (e->desk == 0xffffffff)
	  e_border_stick(bd);
	else if (e->desk < (bd->zone->desk_x_count * bd->zone->desk_y_count))
	  {
	     E_Desk *desk;

	     desk = e_desk_at_pos_get(bd->zone, e->desk);
	     if (desk)
	       e_border_desk_set(bd, desk);
	  }
     }
   else
     {
	ecore_x_netwm_desktop_set(e->win, e->desk);
     }
   return 1;
}

static int
_e_border_cb_sync_alarm(void *data, int ev_type, void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Sync_Alarm *e;

   e = ev;
   bd = e_border_find_by_alarm(e->alarm);
   if (!bd) return 1;
   bd->client.netwm.sync.wait--;
   bd->client.netwm.sync.send_time = ecore_time_get();
   if (bd->client.netwm.sync.wait <= 0)
     _e_border_resize_handle(bd);
   return 1;
}

/* FIXME:
 * Using '2' is bad, may change in zone flip code.
 * Calculate pos from e->x and e->y
 */
static int
_e_border_cb_pointer_warp(void *data, int ev_type, void *ev)
{
   E_Event_Pointer_Warp *e;

   e = ev;
   if (!move) return 1;
   e_border_move(move, move->x + (e->curr.x - e->prev.x), move->y + (e->curr.y - e->prev.y));
   return 1;
}

static void
_e_border_cb_signal_bind(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Border *bd;

   bd = data;
   if (e_dnd_active()) return;
   e_bindings_signal_handle(E_BINDING_CONTEXT_BORDER, E_OBJECT(bd), 
			    (char *)emission, (char *)source);
}

static int
_e_border_cb_mouse_in(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_In *ev;
   E_Border *bd;

   ev = event;
   bd = data;
#ifdef INOUTDEBUG_MOUSE
     {
	time_t t;
	char *ct;

	const char *modes[] = {
	     "MODE_NORMAL",
	     "MODE_WHILE_GRABBED",
	     "MODE_GRAB",
	     "MODE_UNGRAB"
	};
	const char *details[] = {
	     "DETAIL_ANCESTOR",
	     "DETAIL_VIRTUAL",
	     "DETAIL_INFERIOR",
	     "DETAIL_NON_LINEAR",
	     "DETAIL_NON_LINEAR_VIRTUAL",
	     "DETAIL_POINTER",
	     "DETAIL_POINTER_ROOT",
	     "DETAIL_DETAIL_NONE"
	};
	t = time(NULL);
	ct = ctime(&t);
	ct[strlen(ct) - 1] = 0;
	printf("@@ ->IN 0x%x 0x%x %s md=%s dt=%s\n",
	       ev->win, ev->event_win,
	       ct,
	       modes[ev->mode],
	       details[ev->detail]);
     }
#endif
   if (grabbed) return 1;
   if (ev->event_win == bd->win)
     {
	e_focus_event_mouse_in(bd);
     }
#if 0
   if ((ev->win != bd->win) &&
       (ev->win != bd->event_win) &&
       (ev->event_win != bd->win) &&
       (ev->event_win != bd->event_win))
     return 1;
#else
   if (ev->win != bd->event_win) return 1;
#endif
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;
   evas_event_feed_mouse_in(bd->bg_evas, ev->time, NULL);
   return 1;
}

static int
_e_border_cb_mouse_out(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Out *ev;
   E_Border *bd;

   ev = event;
   bd = data;
#ifdef INOUTDEBUG_MOUSE
     {
	time_t t;
	char *ct;

	const char *modes[] = {
	     "MODE_NORMAL",
	     "MODE_WHILE_GRABBED",
	     "MODE_GRAB",
	     "MODE_UNGRAB"
	};
	const char *details[] = {
	     "DETAIL_ANCESTOR",
	     "DETAIL_VIRTUAL",
	     "DETAIL_INFERIOR",
	     "DETAIL_NON_LINEAR",
	     "DETAIL_NON_LINEAR_VIRTUAL",
	     "DETAIL_POINTER",
	     "DETAIL_POINTER_ROOT",
	     "DETAIL_DETAIL_NONE"
	};
	t = time(NULL);
	ct = ctime(&t);
	ct[strlen(ct) - 1] = 0;
	printf("@@ <-OUT 0x%x 0x%x %s md=%s dt=%s\n",
	       ev->win, ev->event_win,
	       ct,
	       modes[ev->mode],
	       details[ev->detail]);
     }
#endif
   if (grabbed) return 1;
#if 0   
   if (ev->event_win == bd->win)
     {
	if (bd->fullscreen)
	  return 1;
	if ((ev->mode == ECORE_X_EVENT_MODE_UNGRAB) &&
	    (ev->detail == ECORE_X_EVENT_DETAIL_INFERIOR))
	  return 1;
	if (ev->mode == ECORE_X_EVENT_MODE_GRAB)
	  return 1;
	if ((ev->mode == ECORE_X_EVENT_MODE_NORMAL) &&
	    (ev->detail == ECORE_X_EVENT_DETAIL_INFERIOR))
	  return 1;
	e_focus_event_mouse_out(bd);
     }
#endif   
#if 0
   if ((ev->win != bd->win) &&
       (ev->win != bd->event_win) &&
       (ev->event_win != bd->win) &&
       (ev->event_win != bd->event_win))
     return 1;
#else
   if (ev->win != bd->event_win) return 1;
#endif
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;
   evas_event_feed_mouse_out(bd->bg_evas, ev->time, NULL);
   return 1;
}

static int
_e_border_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;
   E_Border *bd;

   ev = event;
   bd = data;
   if (ev->event_win == bd->win)
     {
	bd->mouse.current.mx = ev->root.x;
	bd->mouse.current.my = ev->root.y;
	if (!bd->cur_mouse_action)
	  e_bindings_wheel_event_handle(E_BINDING_CONTEXT_BORDER,
					E_OBJECT(bd), ev);
     }
   evas_event_feed_mouse_wheel(bd->bg_evas, ev->direction, ev->z, ev->time, NULL);
   return 1;
}

static int
_e_border_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Down *ev;
   E_Border *bd;

   ev = event;
   bd = data;
   if (ev->event_win == bd->win)
     {
	if ((ev->button >= 1) && (ev->button <= 3))
	  {
	     bd->mouse.last_down[ev->button - 1].mx = ev->root.x;
	     bd->mouse.last_down[ev->button - 1].my = ev->root.y;
	     bd->mouse.last_down[ev->button - 1].x = bd->x;
	     bd->mouse.last_down[ev->button - 1].y = bd->y;
	     bd->mouse.last_down[ev->button - 1].w = bd->w;
	     bd->mouse.last_down[ev->button - 1].h = bd->h;
	  }
	else
	  {
	     bd->moveinfo.down.x = bd->x;
	     bd->moveinfo.down.y = bd->y;
	     bd->moveinfo.down.w = bd->w;
	     bd->moveinfo.down.h = bd->h;
	  }
	bd->mouse.current.mx = ev->root.x;
	bd->mouse.current.my = ev->root.y;
	if (!bd->cur_mouse_action)
	  {
	     bd->cur_mouse_action = 
	       e_bindings_mouse_down_event_handle(E_BINDING_CONTEXT_BORDER,
						  E_OBJECT(bd), ev);
	     if (bd->cur_mouse_action)
	       {
		  if ((!bd->cur_mouse_action->func.end_mouse) &&
		      (!bd->cur_mouse_action->func.end))
		    bd->cur_mouse_action = NULL;
		  if (bd->cur_mouse_action)
		    e_object_ref(E_OBJECT(bd->cur_mouse_action));
	       }
	  }
	e_focus_event_mouse_down(bd);
     }
   if (ev->win != ev->event_win)
     {
	return 1;
     }
   if ((ev->win != bd->event_win) && (ev->event_win != bd->win))
     {
//	printf("abort ev\n");
	return 1;
     }
   if ((ev->button >= 1) && (ev->button <= 3))
     {
	bd->mouse.last_down[ev->button - 1].mx = ev->root.x;
	bd->mouse.last_down[ev->button - 1].my = ev->root.y;
	bd->mouse.last_down[ev->button - 1].x = bd->x;
	bd->mouse.last_down[ev->button - 1].y = bd->y;
	bd->mouse.last_down[ev->button - 1].w = bd->w;
	bd->mouse.last_down[ev->button - 1].h = bd->h;
     }
   else
     {
	bd->moveinfo.down.x = bd->x;
	bd->moveinfo.down.y = bd->y;
	bd->moveinfo.down.w = bd->w;
	bd->moveinfo.down.h = bd->h;
     }
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;
/*   
   if (bd->moving)
     {

 printf("moving\n");
     }
   else if (bd->resize_mode != RESIZE_NONE)
     {
     }
   else
*/
     {
	Evas_Button_Flags flags = EVAS_BUTTON_NONE;

	if (ev->double_click) flags |= EVAS_BUTTON_DOUBLE_CLICK;
	if (ev->triple_click) flags |= EVAS_BUTTON_TRIPLE_CLICK;
	evas_event_feed_mouse_down(bd->bg_evas, ev->button, flags, ev->time, NULL);
     }
   return 1;
}

static int
_e_border_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   E_Border *bd;

   ev = event;
   bd = data;
   if (ev->event_win == bd->win)
     {
	if ((ev->button >= 1) && (ev->button <= 3))
	  {
	     bd->mouse.last_up[ev->button - 1].mx = ev->root.x;
	     bd->mouse.last_up[ev->button - 1].my = ev->root.y;
	     bd->mouse.last_up[ev->button - 1].x = bd->x;
	     bd->mouse.last_up[ev->button - 1].y = bd->y;
	  }
	bd->mouse.current.mx = ev->root.x;
	bd->mouse.current.my = ev->root.y;
	/* also we dont pass the same params that went in - then again that */
	/* should be ok as we are just ending the action if it has an end */
	if (bd->cur_mouse_action)
	  {
	     if (bd->cur_mouse_action->func.end_mouse)
	       bd->cur_mouse_action->func.end_mouse(E_OBJECT(bd), "", ev);
	     else if (bd->cur_mouse_action->func.end)
	       bd->cur_mouse_action->func.end(E_OBJECT(bd), "");
	     e_object_unref(E_OBJECT(bd->cur_mouse_action));
	     bd->cur_mouse_action = NULL;
	  }
	else
	  {
	     if (!e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_BORDER, E_OBJECT(bd), ev))
	       e_focus_event_mouse_up(bd);
	  }
     }
   if (ev->win != bd->event_win) return 1;
   if ((ev->button >= 1) && (ev->button <= 3))
     {
	bd->mouse.last_up[ev->button - 1].mx = ev->root.x;
	bd->mouse.last_up[ev->button - 1].my = ev->root.y;
	bd->mouse.last_up[ev->button - 1].x = bd->x;
	bd->mouse.last_up[ev->button - 1].y = bd->y;
     }
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;

   bd->drag.start = 0;

   evas_event_feed_mouse_up(bd->bg_evas, ev->button, EVAS_BUTTON_NONE, ev->time, NULL);
   return 1;
}

static int
_e_border_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;
   E_Border *bd;

   ev = event;
   bd = data;
   if (ev->event_win == bd->win)
     {
//	printf("GRABMOVE2!\n");
     }
   if ((ev->win != bd->event_win) &&
       (ev->event_win != bd->win)) return 1;
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;
   if (bd->moving)
     {
	int x, y, new_x, new_y;
	int new_w, new_h;
	Evas_List *skiplist = NULL;

#if 0
	if ((ecore_time_get() - bd->client.netwm.sync.time) > 0.5)
	  bd->client.netwm.sync.wait = 0;
	if ((bd->client.netwm.sync.request) &&
	    (bd->client.netwm.sync.alarm) &&
	    (bd->client.netwm.sync.wait > 1)) return 1;
#endif

	if ((bd->moveinfo.down.button >= 1) && (bd->moveinfo.down.button <= 3))
	  {
	     x = bd->mouse.last_down[bd->moveinfo.down.button - 1].x +
	       (bd->mouse.current.mx - bd->moveinfo.down.mx);
	     y = bd->mouse.last_down[bd->moveinfo.down.button - 1].y +
	       (bd->mouse.current.my - bd->moveinfo.down.my);
	  }
	else
	  {
	     x = bd->moveinfo.down.x +
	       (bd->mouse.current.mx - bd->moveinfo.down.mx);
	     y = bd->moveinfo.down.y +
	       (bd->mouse.current.my - bd->moveinfo.down.my);
	  }
	new_x = x;
	new_y = y;
	skiplist = evas_list_append(skiplist, bd);
	e_resist_container_border_position(bd->zone->container, skiplist,
					   bd->x, bd->y, bd->w, bd->h,
					   x, y, bd->w, bd->h,
					   &new_x, &new_y, &new_w, &new_h);
	evas_list_free(skiplist);
	e_border_move(bd, new_x, new_y);
	e_zone_flip_coords_handle(bd->zone, ev->root.x, ev->root.y);
     }
   else if (bd->resize_mode != RESIZE_NONE)
     {
#if 0
	/* FIXME: it seems we send sync requests we dont get replies to */
	/* ie our sync wait > 1 often - try eclipse - its slow enough to */
	/* REALLY show how bad this is */
	printf("SYNC %i - %3.3f\n", bd->client.netwm.sync.wait,
	       ecore_time_get() - bd->client.netwm.sync.time);
	if ((ecore_time_get() - bd->client.netwm.sync.time) > 0.5)
	  bd->client.netwm.sync.wait = 0;
	if ((bd->client.netwm.sync.request) &&
	    (bd->client.netwm.sync.alarm) &&
	    (bd->client.netwm.sync.wait > 1)) return 1;
#endif
	_e_border_resize_handle(bd);
     }
   else
     {
	if (bd->drag.start)
	  {
	     if ((bd->drag.x == -1) && (bd->drag.y == -1))
	       {
		  bd->drag.x = ev->root.x;
		  bd->drag.y = ev->root.y;
	       }
	     else
	       {
		  int dx, dy;
		  
		  dx = bd->drag.x - ev->root.x;
		  dy = bd->drag.y - ev->root.y;
		  if (((dx * dx) + (dy * dy)) > 
		      (e_config->drag_resist * e_config->drag_resist))
		    {
		       /* start drag! */
		       if (bd->icon_object)
			 {
			    E_Drag *drag;
			    Evas_Object *o = NULL;
			    Evas_Coord x, y, w, h;
			    const char *file = NULL, *part = NULL;
			    const char *drag_types[] = { "enlightenment/border" };

			    evas_object_geometry_get(bd->icon_object,
						     &x, &y, &w, &h);
			    drag = e_drag_new(bd->zone->container, bd->x + x, bd->y + y,
					      drag_types, 1, bd, -1, NULL);
			    edje_object_file_get(bd->icon_object, &file, &part);
			    if ((file) && (part))
			      {
				 o = edje_object_add(drag->evas);
				 edje_object_file_set(o, file, part);
			      }
			    else
			      {
				 int iw, ih;
				 void *data;
				 
				 data = e_icon_data_get(bd->icon_object, &iw, &ih);
				 if (data)
				   {
				      o = e_icon_add(drag->evas);
				      e_icon_data_set(o, data, iw, ih);
				      e_icon_alpha_set(o, 1);
				   }
			      }
			    if (!o)
			      {
				 /* FIXME: fallback icon for drag */
				 o = evas_object_rectangle_add(drag->evas);
				 evas_object_color_set(o, 255, 255, 255, 255);
			      }
			    e_drag_object_set(drag, o);

			    e_drag_resize(drag, w, h);
			    e_drag_start(drag, bd->drag.x, bd->drag.y);
			    evas_event_feed_mouse_up(bd->bg_evas, 1,
						     EVAS_BUTTON_NONE, ev->time, 
						     NULL);
			 }
		       bd->drag.start = 0;
		    }
	       }
	  }
	evas_event_feed_mouse_move(bd->bg_evas, ev->x, ev->y, ev->time, NULL);
     }
   return 1;
}

static int
_e_border_cb_grab_replay(void *data, int type, void *event)
{ 
   Ecore_X_Event_Mouse_Button_Down *ev;
   if (type != ECORE_X_EVENT_MOUSE_BUTTON_DOWN) return 0;
   
   ev = event;
   if ((e_config->pass_click_on) || (e_config->always_click_to_raise) ||
       (e_config->always_click_to_focus))
     {
	E_Border *bd;
	
	bd = e_border_find_by_window(ev->event_win);
	if (bd)
	  {
	     if (bd->cur_mouse_action)
	       return 0;
	     if (ev->event_win == bd->win)
	       {
		  if (!e_bindings_mouse_down_find(E_BINDING_CONTEXT_BORDER,
						  E_OBJECT(bd), ev, NULL))
		    return 1;
	       }
	  }
     }
   return 0;
}

static void
_e_border_eval(E_Border *bd)
{
   int change_urgent = 0;
   
   /* fetch any info queued to be fetched */
   if (bd->client.icccm.fetch.client_leader)
     {
	bd->client.icccm.client_leader = ecore_x_icccm_client_leader_get(bd->client.win);
	bd->client.icccm.fetch.client_leader = 0;
     }
   if (bd->client.icccm.fetch.title)
     {
	if (bd->client.icccm.title) free(bd->client.icccm.title);
	bd->client.icccm.title = ecore_x_icccm_title_get(bd->client.win);

	bd->client.icccm.fetch.title = 0;
	if (bd->bg_object)
	  {
	     edje_object_part_text_set(bd->bg_object, "title_text",
				       bd->client.icccm.title);
	  }
     }
   if (bd->client.netwm.fetch.name)
     {
	if (bd->client.netwm.name) free(bd->client.netwm.name);
	ecore_x_netwm_name_get(bd->client.win, &bd->client.netwm.name);

	bd->client.netwm.fetch.name = 0;
	if (bd->bg_object)
	  {
	     edje_object_part_text_set(bd->bg_object, "title_text",
				       bd->client.netwm.name);
	  }
     }
   if (bd->client.icccm.fetch.name_class)
     {
	int nc_change = 0;
	char *pname, *pclass;

	pname = bd->client.icccm.name;
	pclass = bd->client.icccm.class;
	bd->client.icccm.name = NULL;
	bd->client.icccm.class = NULL;
	ecore_x_icccm_name_class_get(bd->client.win, &bd->client.icccm.name, &bd->client.icccm.class);
	if ((pname) && (bd->client.icccm.name) &&
	    (pclass) && (bd->client.icccm.class))
	  {
	     if (!((!strcmp(bd->client.icccm.name, pname)) &&
		   (!strcmp(bd->client.icccm.class, pclass))))
	       nc_change = 1;
	  }
	else if (((!pname) || (!pclass)) &&
		 ((bd->client.icccm.name) || (bd->client.icccm.class)))
	  nc_change = 1;
	else if (((bd->client.icccm.name) || (bd->client.icccm.class)) &&
		 ((!pname) || (!pclass)))
	  nc_change = 1;
	if (pname) free(pname);
	if (pclass) free(pclass);
	if (nc_change)
	  bd->changes.icon = 1;
	bd->client.icccm.fetch.name_class = 0;
     }
   if (bd->client.icccm.fetch.state)
     {
	bd->client.icccm.state = ecore_x_icccm_state_get(bd->client.win);
	bd->client.icccm.fetch.state = 0;
     }
   if (bd->client.netwm.fetch.state)
     {
	e_hints_window_state_get(bd);
	bd->client.netwm.fetch.state = 0;
     }
   if (bd->client.e.fetch.state)
     {
	e_hints_window_e_state_get(bd);
	bd->client.e.fetch.state = 0;
     }
   if (bd->client.netwm.fetch.type)
     {
	e_hints_window_type_get(bd);
	if ((!bd->lock_border) || (!bd->client.border.name))
	  {
	     if (bd->client.border.name) evas_stringshare_del(bd->client.border.name);
	     bd->client.border.name = NULL;
	     bd->client.border.changed = 1;
	  }
	if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DOCK)
	  {
	     /* TODO: Make this user options */
	     if (!bd->client.netwm.state.skip_pager)
	       {
		  bd->client.netwm.state.skip_pager = 1;
		  bd->client.netwm.update.state = 1;
	       }
	     if (!bd->client.netwm.state.skip_taskbar)
	       {
		  bd->client.netwm.state.skip_taskbar = 1;
		  bd->client.netwm.update.state = 1;
	       }
	  }

	bd->client.netwm.fetch.type = 0;
     }
   if (bd->client.icccm.fetch.machine)
     {
	if (bd->client.icccm.machine) free(bd->client.icccm.machine);
	bd->client.icccm.machine = ecore_x_icccm_client_machine_get(bd->client.win);
	if ((bd->client.icccm.client_leader) &&
	    (!bd->client.icccm.machine))
	  ecore_x_icccm_client_machine_get(bd->client.icccm.client_leader);
	bd->client.icccm.fetch.machine = 0;
     }
   if (bd->client.icccm.fetch.command)
     {
	if ((bd->client.icccm.command.argc > 0) && (bd->client.icccm.command.argv))
	  {
	     int i;
	     
	     for (i = 0; i < bd->client.icccm.command.argc; i++)
	       free(bd->client.icccm.command.argv[i]);
	     free(bd->client.icccm.command.argv);
	  }
	bd->client.icccm.command.argc = 0;
	bd->client.icccm.command.argv = NULL;
	ecore_x_icccm_command_get(bd->client.win,
				  &(bd->client.icccm.command.argc),
				  &(bd->client.icccm.command.argv));
	if ((bd->client.icccm.client_leader) &&
	    (!bd->client.icccm.command.argv))
	  ecore_x_icccm_command_get(bd->client.icccm.client_leader,
				    &(bd->client.icccm.command.argc),
				    &(bd->client.icccm.command.argv));
	bd->client.icccm.fetch.command = 0;
     }
   if (bd->client.icccm.fetch.hints)
     {
	int accepts_focus = 1;
	int is_urgent = 0;

	bd->client.icccm.initial_state = ECORE_X_WINDOW_STATE_HINT_NORMAL;
	if (ecore_x_icccm_hints_get(bd->client.win,
				    &accepts_focus,
				    &bd->client.icccm.initial_state,
				    &bd->client.icccm.icon_pixmap,
				    &bd->client.icccm.icon_mask,
				    &bd->client.icccm.icon_window,
				    &bd->client.icccm.window_group,
				    &is_urgent))
	  {
	     bd->client.icccm.accepts_focus = accepts_focus;
	     if (bd->client.icccm.urgent != is_urgent)
	       change_urgent = 1;
	     bd->client.icccm.urgent = is_urgent;

	     /* If this is a new window, set the state as requested. */
	     if ((bd->new_client) &&
		 (bd->client.icccm.initial_state == ECORE_X_WINDOW_STATE_HINT_ICONIC))
	       e_border_iconify(bd);
	  }
	bd->client.icccm.fetch.hints = 0;
     }
   if (bd->client.icccm.fetch.size_pos_hints)
     {
	int request_pos = 0;

	if (ecore_x_icccm_size_pos_hints_get(bd->client.win,
					     &request_pos,
					     &bd->client.icccm.gravity,
					     &bd->client.icccm.min_w,
					     &bd->client.icccm.min_h,
					     &bd->client.icccm.max_w,
					     &bd->client.icccm.max_h,
					     &bd->client.icccm.base_w,
					     &bd->client.icccm.base_h,
					     &bd->client.icccm.step_w,
					     &bd->client.icccm.step_h,
					     &bd->client.icccm.min_aspect,
					     &bd->client.icccm.max_aspect))
	  {
	     bd->client.icccm.request_pos = request_pos;
	  }
	else
	  {
//	     printf("##- NO SIZE HINTS!\n");
	  }
	if (bd->client.icccm.min_w > 32767) bd->client.icccm.min_w = 32767;
	if (bd->client.icccm.min_h > 32767) bd->client.icccm.min_h = 32767;
	if (bd->client.icccm.max_w > 32767) bd->client.icccm.max_w = 32767;
	if (bd->client.icccm.max_h > 32767) bd->client.icccm.max_h = 32767;
	if (bd->client.icccm.base_w > 32767) bd->client.icccm.base_w = 32767;
	if (bd->client.icccm.base_h > 32767) bd->client.icccm.base_h = 32767;
//	if (bd->client.icccm.step_w < 1) bd->client.icccm.step_w = 1;
//	if (bd->client.icccm.step_h < 1) bd->client.icccm.step_h = 1;
#if 0
	printf("##- SIZE HINTS for 0x%x: min %ix%i, max %ix%i, base %ix%i, step %ix%i, aspect (%f, %f)\n",
	       bd->client.win,
	       bd->client.icccm.min_w, bd->client.icccm.min_h,
	       bd->client.icccm.max_w, bd->client.icccm.max_h,
	       bd->client.icccm.base_w, bd->client.icccm.base_h,
	       bd->client.icccm.step_w, bd->client.icccm.step_h,
	       bd->client.icccm.min_aspect, bd->client.icccm.max_aspect);
#endif

	bd->client.icccm.fetch.size_pos_hints = 0;
     }
   if (bd->client.icccm.fetch.protocol)
     {
	int i, num;
	Ecore_X_WM_Protocol *proto;

	proto = ecore_x_window_prop_protocol_list_get(bd->client.win, &num);
	if (proto)
	  {
	     for (i = 0; i < num; i++)
	       {
		  if (proto[i] == ECORE_X_WM_PROTOCOL_DELETE_REQUEST)
		    bd->client.icccm.delete_request = 1;
		  else if (proto[i] == ECORE_X_WM_PROTOCOL_TAKE_FOCUS)
		    bd->client.icccm.take_focus = 1;
		  else if (proto[i] == ECORE_X_NET_WM_PROTOCOL_PING)
		    bd->client.netwm.ping = 1;
		  else if (proto[i] == ECORE_X_NET_WM_PROTOCOL_SYNC_REQUEST)
		    {
		       bd->client.netwm.sync.request = 1;
		       if (!ecore_x_netwm_sync_counter_get(bd->client.win,
							   &bd->client.netwm.sync.counter))
			 bd->client.netwm.sync.request = 0;
		    }
	       }
	     free(proto);
	  }
	if (bd->client.netwm.ping)
	  e_border_ping(bd);
	else
	  {
	     if (bd->ping_timer) ecore_timer_del(bd->ping_timer);
	     bd->ping_timer = NULL;
	  }
	bd->client.icccm.fetch.protocol = 0;
     }
   if (bd->client.icccm.fetch.transient_for)
     {
	bd->client.icccm.transient_for = ecore_x_icccm_transient_for_get(bd->client.win);
	bd->client.icccm.fetch.transient_for = 0;
     }
   if (bd->client.icccm.fetch.window_role)
     {
	bd->client.icccm.window_role = ecore_x_icccm_window_role_get(bd->client.win);
	bd->client.icccm.fetch.window_role = 0;
     }
   if (bd->client.icccm.fetch.icon_name)
     {
	if (bd->client.icccm.icon_name) free(bd->client.icccm.icon_name);
	bd->client.icccm.icon_name = ecore_x_icccm_icon_name_get(bd->client.win);

	bd->client.icccm.fetch.icon_name = 0;
     }
   if (bd->client.netwm.fetch.icon_name)
     {
	if (bd->client.netwm.icon_name) free(bd->client.netwm.icon_name);
	ecore_x_netwm_icon_name_get(bd->client.win, &bd->client.netwm.icon_name);

	bd->client.netwm.fetch.icon_name = 0;
     }
   if (bd->client.netwm.fetch.icon)
     {
	if (bd->client.netwm.icons)
	  {
	     int i;
	     
	     for (i = 0; i < bd->client.netwm.num_icons; i++)
	       free(bd->client.netwm.icons[i].data);
	     free(bd->client.netwm.icons);
	  }
	if (!ecore_x_netwm_icons_get(bd->client.win,
				     &bd->client.netwm.icons, &bd->client.netwm.num_icons))
	  {
	     printf("ERROR: Fetch icon from client\n");
	     bd->client.netwm.icons = NULL;
	     bd->client.netwm.num_icons = 0;
	  }
	else
	  bd->changes.icon = 1;
	bd->client.netwm.fetch.icon = 0;
     }
   if (bd->client.netwm.fetch.user_time)
     {
	ecore_x_netwm_user_time_get(bd->client.win, &bd->client.netwm.user_time);

	bd->client.netwm.fetch.user_time = 0;
     }
   if (bd->client.netwm.fetch.strut)
     {
	if (!ecore_x_netwm_strut_partial_get(bd->client.win,
					     &bd->client.netwm.strut.left,
					     &bd->client.netwm.strut.right,
					     &bd->client.netwm.strut.top,
					     &bd->client.netwm.strut.bottom,
					     &bd->client.netwm.strut.left_start_y,
					     &bd->client.netwm.strut.left_end_y,
					     &bd->client.netwm.strut.right_start_y,
					     &bd->client.netwm.strut.right_end_y,
					     &bd->client.netwm.strut.top_start_x,
					     &bd->client.netwm.strut.top_end_x,
					     &bd->client.netwm.strut.bottom_start_x,
					     &bd->client.netwm.strut.bottom_end_x))
	  {
	     ecore_x_netwm_strut_get(bd->client.win,
				     &bd->client.netwm.strut.left, &bd->client.netwm.strut.right,
				     &bd->client.netwm.strut.top, &bd->client.netwm.strut.bottom);

	     bd->client.netwm.strut.left_start_y = 0;
	     bd->client.netwm.strut.left_end_y = 0;
	     bd->client.netwm.strut.right_start_y = 0;
	     bd->client.netwm.strut.right_end_y = 0;
	     bd->client.netwm.strut.top_start_x = 0;
	     bd->client.netwm.strut.top_end_x = 0;
	     bd->client.netwm.strut.bottom_start_x = 0;
	     bd->client.netwm.strut.bottom_end_x = 0;
	  }

	bd->client.netwm.fetch.strut = 0;
     }
   if (bd->changes.shape)
     {
	Ecore_X_Rectangle *rects;
	int num;
	
	bd->changes.shape = 0;
	rects = ecore_x_window_shape_rectangles_get(bd->client.win, &num);
	if (rects)
	  {
	     int cw = 0, ch = 0;
	     
	     /* This doesnt fix the race, but makes it smaller. we detect
	      * this and if cw and ch != client w/h then mark this as needing
	      * a shape change again to fixup next event loop.
	      */
	     ecore_x_window_size_get(bd->client.win, &cw, &ch);
	     if ((cw != bd->client.w) || (ch != bd->client.h))
	       bd->changes.shape = 1;
	     if ((num == 1) &&
		 (rects[0].x == 0) &&
		 (rects[0].y == 0) &&
		 (rects[0].width == cw) &&
		 (rects[0].height == ch))
	       {
		  if (bd->client.shaped)
		    {
		       bd->client.shaped = 0;
		    }
	       }
	     else
	       {
		  if (!bd->client.shaped)
		    {
		       bd->client.shaped = 1;
		    }
	       }
	     free(rects);
	  }
	else
	  bd->client.shaped = 0;
	bd->need_shape_merge = 1;
     }
   if (bd->client.mwm.fetch.hints)
     {
	int pb;

	bd->client.mwm.exists =
	  ecore_x_mwm_hints_get(bd->client.win,
				&bd->client.mwm.func,
				&bd->client.mwm.decor,
				&bd->client.mwm.input);
	pb = bd->client.mwm.borderless;
	bd->client.mwm.borderless = 0;
	if (bd->client.mwm.exists)
	  {
//	     printf("##- MWM HINTS SET 0x%x!\n", bd->client.win);
	     if ((!(bd->client.mwm.decor & ECORE_X_MWM_HINT_DECOR_ALL)) &&
		 (!(bd->client.mwm.decor & ECORE_X_MWM_HINT_DECOR_TITLE)) &&
		 (!(bd->client.mwm.decor & ECORE_X_MWM_HINT_DECOR_BORDER)))
	       bd->client.mwm.borderless = 1;
	  }
	if (bd->client.mwm.borderless != pb)
	  {
	     if ((!bd->lock_border) || (!bd->client.border.name))
	       {
		  if (bd->client.border.name)
		    evas_stringshare_del(bd->client.border.name);
		  bd->client.border.name = NULL;
		  bd->client.border.changed = 1;
	       }
	  }
	bd->client.mwm.fetch.hints = 0;
     }
   if (bd->client.netwm.update.state)
     {
	e_hints_window_state_set(bd);
	bd->client.netwm.update.state = 0;
     }

   if (bd->new_client)
     {
	E_Remember *rem = NULL;
	E_Event_Border_Add *ev;

	ev = calloc(1, sizeof(E_Event_Border_Add));
	ev->border = bd;
	e_object_ref(E_OBJECT(bd));
//	e_object_breadcrumb_add(E_OBJECT(bd), "border_add_event");
	ecore_event_add(E_EVENT_BORDER_ADD, ev, _e_border_event_border_add_free, NULL);

	if ((!bd->lock_border) || (!bd->client.border.name))
	  {
	     if (bd->client.border.name)
	       evas_stringshare_del(bd->client.border.name);
	     bd->client.border.name = NULL;
	     bd->client.border.changed = 1;
	  }
	if (!bd->remember)
	  {
	     rem = e_remember_find(bd);
	     if ((rem) && (e_remember_usable_get(rem)))
	       {
		  bd->remember = rem;
		  e_remember_use(rem);
	       }
	  }
	if (bd->remember)
	  {
	     rem  = bd->remember;
	     
	     if (rem->apply & E_REMEMBER_APPLY_ZONE)
	       {
		  E_Zone *zone;
		  
		  zone = e_container_zone_number_get(bd->zone->container, rem->prop.zone);
		  if (zone)
		    e_border_zone_set(bd, zone);
	       }
	     if (rem->apply & E_REMEMBER_APPLY_DESKTOP)
	       {
		  E_Desk *desk;
		  
		  desk = e_desk_at_xy_get(bd->zone, rem->prop.desk_x, rem->prop.desk_y);
		  if (desk)
		    e_border_desk_set(bd, desk);
	       }
	     if ((rem->apply & E_REMEMBER_APPLY_POS) && (!bd->re_manage))
	       {
		  bd->x = rem->prop.pos_x;
		  bd->y = rem->prop.pos_y;
		  if (bd->zone->w != rem->prop.res_x)
		    {
		       bd->x = (rem->prop.pos_x * rem->prop.res_x) / bd->zone->w;
		    }
		  if (bd->zone->h != rem->prop.res_y)
		    {
		       bd->y = (rem->prop.pos_y * rem->prop.res_y) / bd->zone->h;
		    }
		  bd->x += bd->zone->x;
		  bd->y += bd->zone->y;
		  bd->placed = 1;
		  bd->changes.pos = 1;
	       }
	     if (rem->apply & E_REMEMBER_APPLY_SIZE)
	       {
		  bd->w = rem->prop.w + bd->client_inset.l + bd->client_inset.r;
		  bd->h = rem->prop.h + bd->client_inset.t + bd->client_inset.b;
		  bd->client.w = bd->w - (bd->client_inset.l + bd->client_inset.r);
		  bd->client.h = bd->h - (bd->client_inset.t + bd->client_inset.b);
		  bd->changes.size = 1;
		  bd->changes.shape = 1;
	       }
	     if (rem->apply & E_REMEMBER_APPLY_LAYER)
	       {
		  bd->layer = rem->prop.layer;
		  if (bd->layer == 100)
		    e_hints_window_stacking_set(bd, E_STACKING_NONE);
		  else if (bd->layer == 150)
		    e_hints_window_stacking_set(bd, E_STACKING_ABOVE);
		  e_container_border_raise(bd);
	       }
	     if (rem->apply & E_REMEMBER_APPLY_BORDER)
	       {
		  if (bd->client.border.name) evas_stringshare_del(bd->client.border.name);
		  bd->client.border.name = NULL;
		  if (rem->prop.border)
		    bd->client.border.name = evas_stringshare_add(rem->prop.border);
		  bd->client.border.changed = 1;
	       }
	     if (rem->apply & E_REMEMBER_APPLY_STICKY)
	       {
		  if (rem->prop.sticky) e_border_stick(bd);
	       }
	     if (rem->apply & E_REMEMBER_APPLY_SHADE)
	       {
		  if (rem->prop.shaded >= 100)
		    e_border_shade(bd, rem->prop.shaded - 100);
		  else if (rem->prop.shaded >= 50)
		    e_border_unshade(bd, rem->prop.shaded - 50);
	       }
	     if (rem->apply & E_REMEMBER_APPLY_LOCKS)
	       {
		  bd->lock_user_location = rem->prop.lock_user_location;
		  bd->lock_client_location = rem->prop.lock_client_location;
		  bd->lock_user_size = rem->prop.lock_user_size;
		  bd->lock_client_size = rem->prop.lock_client_size;
		  bd->lock_user_stacking = rem->prop.lock_user_stacking;
		  bd->lock_client_stacking = rem->prop.lock_client_stacking;
		  bd->lock_user_iconify = rem->prop.lock_user_iconify;
		  bd->lock_client_iconify = rem->prop.lock_client_iconify;
		  bd->lock_user_desk = rem->prop.lock_user_desk;
		  bd->lock_client_desk = rem->prop.lock_client_desk;
		  bd->lock_user_sticky = rem->prop.lock_user_sticky;
		  bd->lock_client_sticky = rem->prop.lock_client_sticky;
		  bd->lock_user_shade = rem->prop.lock_user_shade;
		  bd->lock_client_shade = rem->prop.lock_client_shade;
		  bd->lock_user_maximize = rem->prop.lock_user_maximize;
		  bd->lock_client_maximize = rem->prop.lock_client_maximize;
		  bd->lock_user_fullscreen = rem->prop.lock_user_fullscreen;
		  bd->lock_client_fullscreen = rem->prop.lock_client_fullscreen;
		  bd->lock_border = rem->prop.lock_border;
		  bd->lock_close = rem->prop.lock_close;
		  bd->lock_focus_in = rem->prop.lock_focus_in;
		  bd->lock_focus_out = rem->prop.lock_focus_out;
		  bd->lock_life = rem->prop.lock_life;
	       }
	     if (rem->apply & E_REMEMBER_APPLY_SKIP_WINLIST)
	       {
		  bd->user_skip_winlist = rem->prop.skip_winlist;
	       }
	  }
     }
   
   if ((bd->client.border.changed) && (!bd->shaded))
     {
	Evas_Object *o;
	char buf[4096];
	Evas_Coord cx, cy, cw, ch;
	int l, r, t, b;
	int ok;
	
	if (!bd->client.border.name)
	  {
	     if (bd->client.mwm.borderless)
	       bd->client.border.name = evas_stringshare_add("borderless");
	     else if (((bd->client.icccm.transient_for != 0) ||
		       (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG)) &&
		      (bd->client.icccm.min_w == bd->client.icccm.max_w) &&
		      (bd->client.icccm.min_h == bd->client.icccm.max_h))
	       bd->client.border.name = evas_stringshare_add("noresize_dialog");
	     else if ((bd->client.icccm.min_w == bd->client.icccm.max_w) &&
		      (bd->client.icccm.min_h == bd->client.icccm.max_h))
	       bd->client.border.name = evas_stringshare_add("noresize");
	     else if (bd->client.shaped)
	       bd->client.border.name = evas_stringshare_add("shaped");
	     else if ((!bd->client.icccm.accepts_focus) &&
		      (!bd->client.icccm.take_focus))
	       bd->client.border.name = evas_stringshare_add("nofocus");
	     else if (bd->client.icccm.urgent)
	       bd->client.border.name = evas_stringshare_add("urgent");
	     else if ((bd->client.icccm.transient_for != 0) ||
		      (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG))
	       bd->client.border.name = evas_stringshare_add("dialog");
	     else if (bd->client.netwm.state.modal)
	       bd->client.border.name = evas_stringshare_add("modal");
	     else if ((bd->client.netwm.state.skip_taskbar) ||
		      (bd->client.netwm.state.skip_pager))
	       bd->client.border.name = evas_stringshare_add("skipped");
	     else
	       bd->client.border.name = evas_stringshare_add("default");
	  }
	if (bd->bg_object)
	  {
	     bd->w -= (bd->client_inset.l + bd->client_inset.r);
	     bd->h -= (bd->client_inset.t + bd->client_inset.b);
	     bd->client_inset.l = 0;
	     bd->client_inset.r = 0;
	     bd->client_inset.t = 0;
	     bd->client_inset.b = 0;
	     bd->changes.size = 1;
	     evas_object_del(bd->bg_object);
	  }
        o = edje_object_add(bd->bg_evas);
	snprintf(buf, sizeof(buf), "widgets/border/%s/border",
		 bd->client.border.name);
	ok = e_theme_edje_object_set(o, "base/theme/borders", buf);
	if ((!ok) && (strcmp(bd->client.border.name, "borderless")))
	  ok = e_theme_edje_object_set(o, "base/theme/borders",
				       "widgets/border/default/border");
	if (ok)
	  {
	     const char *shape_option;
	     
	     bd->bg_object = o;
	     shape_option = edje_object_data_get(o, "shaped");
	     if (shape_option)
	       {
		  if (!strcmp(shape_option, "1"))
		    {
		       if (!bd->shaped)
			 {
			    bd->shaped = 1;
			    ecore_evas_shaped_set(bd->bg_ecore_evas, bd->shaped);
			 }
		    }
		  else
		    {
		       if (bd->shaped)
			 {
			    bd->shaped = 0;
			    ecore_evas_shaped_set(bd->bg_ecore_evas, bd->shaped);
			 }
		    }
	       }
	     else
	       {
		  if (bd->shaped)
		    {
		       bd->shaped = 0;
		       ecore_evas_shaped_set(bd->bg_ecore_evas, bd->shaped);
		    }
	       }
	     
	     if (bd->client.netwm.name)
	       edje_object_part_text_set(o, "title_text",
					 bd->client.netwm.name);
	     else if (bd->client.icccm.title)
	       edje_object_part_text_set(o, "title_text",
					 bd->client.icccm.title);
	     evas_object_resize(o, 1000, 1000);
	     edje_object_calc_force(o);
	     edje_object_part_geometry_get(o, "client", &cx, &cy, &cw, &ch);
	     l = cx;
	     r = 1000 - (cx + cw);
	     t = cy;
	     b = 1000 - (cy + ch);
	  }
	else
	  {
	     evas_object_del(o);
	     bd->bg_object = NULL;
	     l = 0;
	     r = 0;
	     t = 0;
	     b = 0;
	  }
	bd->client_inset.l = l;
	bd->client_inset.r = r;
	bd->client_inset.t = t;
	bd->client_inset.b = b;
	ecore_x_netwm_frame_size_set(bd->client.win, l, r, t, b);
	ecore_x_e_frame_size_set(bd->client.win, l, r, t, b);
	bd->w += (bd->client_inset.l + bd->client_inset.r);
	bd->h += (bd->client_inset.t + bd->client_inset.b);
	bd->changes.size = 1;
	ecore_x_window_move(bd->client.shell_win, l, t);
	if (bd->bg_object)
	  {
	     edje_object_signal_callback_add(bd->bg_object, "*", "*",
					     _e_border_cb_signal_bind, bd);
	     if (bd->focused)
	       {
		  edje_object_signal_emit(bd->bg_object, "active", "");
		  if (bd->icon_object)
		    edje_object_signal_emit(bd->icon_object, "active", "");
	       }
	     if (bd->shaded)
	       edje_object_signal_emit(bd->bg_object, "shaded", "");
	     if ((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN)
	       edje_object_signal_emit(bd->bg_object, "maximize,fullscreen", "");
	     else if ((bd->maximized & E_MAXIMIZE_TYPE) != E_MAXIMIZE_NONE)
	       edje_object_signal_emit(bd->bg_object, "maximize", "");
	     if (bd->fullscreen)
	       edje_object_signal_emit(bd->bg_object, "fullscreen", "");
	     if (bd->hung)
	       edje_object_signal_emit(bd->bg_object, "hung", "");
	     if (bd->client.icccm.urgent)
	       edje_object_signal_emit(bd->bg_object, "urgent", "");
	     
	     evas_object_move(bd->bg_object, 0, 0);
	     evas_object_resize(bd->bg_object, bd->w, bd->h);
	     evas_object_show(bd->bg_object);
	  }
	bd->client.border.changed = 0;
	
	if (bd->icon_object)
	  {
	     if (bd->bg_object)
	       {
		  evas_object_show(bd->icon_object);
		  edje_object_part_swallow(bd->bg_object, "icon_swallow", bd->icon_object);
	       }
	     else
	       {
		  evas_object_hide(bd->icon_object);
	       }
	  }
     }
   
   if (bd->need_reparent)
     {     
	ecore_x_window_save_set_add(bd->client.win);
	ecore_x_window_reparent(bd->client.win, bd->client.shell_win, 0, 0);
	if (bd->visible)
	  ecore_x_window_show(bd->client.win);
	bd->need_reparent = 0;
     }
   
   if (bd->new_client)
     {
	if (bd->client.icccm.transient_for)
	  {
	     E_Border *bd_parent;

	     bd_parent = e_border_find_by_client_window(bd->client.icccm.transient_for);
	     if ((bd_parent) && (bd_parent != bd))
	       {
		  bd_parent->transients = evas_list_append(bd_parent->transients, bd);
		  bd->parent = bd_parent;
		  e_border_layer_set(bd, bd->parent->layer);
		  if ((e_config->modal_windows) && (bd->client.netwm.state.modal))
		    bd->parent->modal = bd;
	       }
	  }
	if (bd->client.icccm.client_leader)
	  {
	     E_Border *bd_leader;

	     bd_leader = e_border_find_by_client_window(bd->client.icccm.client_leader);
	     /* If this border is the leader of the group, don't register itself */
	     if ((bd_leader) && (bd_leader != bd))
	       {
		  bd_leader->group = evas_list_append(bd_leader->group, bd);
		  bd->leader = bd_leader;
		  if ((e_config->modal_windows) && (bd->client.netwm.state.modal))
		    bd->leader->modal = bd;
	       }
	  }
//	printf("##- NEW CLIENT SETUP 0x%x\n", bd->client.win);
	if (bd->re_manage)
	  {
//	     printf("##- REMANAGE!\n");
	     bd->x -= bd->client_inset.l;
	     bd->y -= bd->client_inset.t;
	     bd->changes.pos = 1;
	     bd->placed = 1;
	  }
	else if (!bd->placed)
	  {
	     if (bd->client.icccm.request_pos)
	       {
		  Ecore_X_Window_Attributes *att;
		  int bw;

		  att = &bd->client.initial_attributes;
//		  printf("##- REQUEST POS 0x%x [%i,%i]\n",
//			 bd->client.win, att->x, att->y);
		  bw = att->border * 2;
		  switch (bd->client.icccm.gravity)
		    {
		     case ECORE_X_GRAVITY_N:
		       bd->x = (att->x - (bw / 2));
		       bd->y = att->y;
		       break;
		     case ECORE_X_GRAVITY_NE:
		       bd->x = (att->x - (bw)) - (bd->client_inset.l);
		       bd->y = att->y;
		       break;
		     case ECORE_X_GRAVITY_E:
		       bd->x = (att->x - (bw)) - (bd->client_inset.l);
		       bd->y = (att->y - (bw / 2));
		       break;
		     case ECORE_X_GRAVITY_SE:
		       bd->x = (att->x - (bw)) - (bd->client_inset.l);
		       bd->y = (att->y - (bw)) - (bd->client_inset.t);
		       break;
		     case ECORE_X_GRAVITY_S:
		       bd->x = (att->x - (bw / 2));
		       bd->y = (att->y - (bw)) - (bd->client_inset.t);
		       break;
		     case ECORE_X_GRAVITY_SW:
		       bd->x = att->x;
		       bd->y = (att->y - (bw)) - (bd->client_inset.t);
		       break;
		     case ECORE_X_GRAVITY_W:
		       bd->x = att->x;
		       bd->y = (att->y - (bw)) - (bd->client_inset.t);
		       break;
		     case ECORE_X_GRAVITY_NW:
		     default:
		       bd->x = att->x;
		       bd->y = att->y;
		    }
		  bd->changes.pos = 1;
		  bd->placed = 1;
	       }
	     else
	       {
		  /* FIXME: special placement for dialogs etc. etc. etc goes
		   * here */
		  /* FIXME: what if parent is not on this desktop - or zone? */
		  if ((bd->parent) && (bd->parent->visible))
		    {
		       bd->x = bd->parent->x + ((bd->parent->w - bd->w) / 2);
		       bd->y = bd->parent->y + ((bd->parent->h - bd->h) / 2);
		       bd->changes.pos = 1;
		       bd->placed = 1;
		    }
		  if (!bd->placed)
		    {
		       Evas_List *skiplist = NULL;
		       int new_x, new_y;

//		  printf("##- AUTO POS 0x%x\n", bd->client.win);
		       if (bd->zone->w > bd->w)
			 new_x = bd->zone->x + (rand() % (bd->zone->w - bd->w));
		       else
			 new_x = bd->zone->x;
		       if (bd->zone->h > bd->h)
			 new_y = bd->zone->y + (rand() % (bd->zone->h - bd->h));
		       else
			 new_y = bd->zone->y;

		       if ((e_config->window_placement_policy == E_WINDOW_PLACEMENT_SMART)||(e_config->window_placement_policy == E_WINDOW_PLACEMENT_ANTIGADGET))
			 {
			    skiplist = evas_list_append(skiplist, bd);
			    e_place_zone_region_smart(bd->zone, skiplist,
						      bd->x, bd->y, bd->w, bd->h,
						      &new_x, &new_y);
			    evas_list_free(skiplist);
			 }
		       else if (e_config->window_placement_policy == E_WINDOW_PLACEMENT_MANUAL)
			 {
			    e_place_zone_manual(bd->zone, bd->w, bd->client_inset.t,
						&new_x, &new_y);
			 }
		       else
			 {
			    e_place_zone_cursor(bd->zone, bd->x, bd->y, bd->w, bd->h, 
						bd->client_inset.t, &new_x, &new_y);
			 }
		       bd->x = new_x;
		       bd->y = new_y;
		       bd->changes.pos = 1;
		    }
	       }
	  }
	while (bd->pending_move_resize)
	  {
	     E_Border_Pending_Move_Resize *pnd;

	     pnd = bd->pending_move_resize->data;
	     if ((!bd->lock_client_location) && (pnd->move))
	       {
		  bd->x = pnd->x;
		  bd->y = pnd->y;
		  bd->changes.pos = 1;
		  bd->placed = 1;
	       }
	     if ((!bd->lock_client_size) && (pnd->resize))
	       {
		  bd->w = pnd->w + (bd->client_inset.l + bd->client_inset.r);
		  bd->h = pnd->h + (bd->client_inset.t + bd->client_inset.b);
		  bd->client.w = pnd->w;
		  bd->client.h = pnd->h;
		  bd->changes.size = 1;
	       }
	     free(pnd);
	     bd->pending_move_resize = evas_list_remove_list(bd->pending_move_resize,
							     bd->pending_move_resize);
	  }

     	/* Recreate state */
	e_hints_window_init(bd);
	
        if (bd->client.e.state.centered)
	  {
	     bd->x = bd->zone->x + (bd->zone->w - bd->w) / 2;
	     bd->y = bd->zone->y + (bd->zone->h - bd->h) / 2;
	     bd->changes.pos = 1;
	     bd->placed = 1;
	  }

	if (bd->internal_ecore_evas)
	  ecore_evas_managed_move(bd->internal_ecore_evas,
				  bd->x + bd->client_inset.l,
				  bd->y + bd->client_inset.t);
	ecore_x_icccm_move_resize_send(bd->client.win,
				       bd->x + bd->client_inset.l,
				       bd->y + bd->client_inset.t,
				       bd->client.w,
				       bd->client.h);
	/* if the explicit geometry request asks for the app to be
	 * in another zone - well move it there */
	  {
	     E_Zone *zone;
	     
	     zone = e_container_zone_at_point_get(bd->zone->container,
						  bd->x + (bd->w / 2),
						  bd->y + (bd->h / 2));
	     if (!zone)
	       zone = e_container_zone_at_point_get(bd->zone->container,
						    bd->x,
						    bd->y);
	     if (!zone)
	       zone = e_container_zone_at_point_get(bd->zone->container,
						    bd->x + bd->w - 1,
						    bd->y);
	     if (!zone)
	       zone = e_container_zone_at_point_get(bd->zone->container,
						    bd->x + bd->w - 1,
						    bd->y + bd->h - 1);
	     if (!zone)
	       zone = e_container_zone_at_point_get(bd->zone->container,
						    bd->x,
						    bd->y + bd->h - 1);
	     if ((zone) && (zone != bd->zone))
	       e_border_zone_set(bd, zone);
	  }
     }
   
   /* effect changes to the window border itself */
   if ((bd->changes.shading))
     {
	/*  show at start of unshade (but don't hide until end of shade) */
	if (bd->shaded)
	  ecore_x_window_raise(bd->client.shell_win);
	bd->changes.shading = 0;
     }
   if ((bd->changes.shaded) && (bd->changes.pos) && (bd->changes.size))
     {
	if (bd->shaded)
	  ecore_x_window_lower(bd->client.shell_win);
	else
	  ecore_x_window_raise(bd->client.shell_win);
	bd->changes.shaded = 0;
     }
   else if ((bd->changes.shaded) && (bd->changes.pos))
     {
	if (bd->shaded)
	  ecore_x_window_lower(bd->client.shell_win);
	else
	  ecore_x_window_raise(bd->client.shell_win);
	bd->changes.size = 1;
	bd->changes.shaded = 0;
     }
   else if ((bd->changes.shaded) && (bd->changes.size))
     {
	if (bd->shaded)
	  ecore_x_window_lower(bd->client.shell_win);
	else
	  ecore_x_window_raise(bd->client.shell_win);
	bd->changes.shaded = 0;
     }
   else if (bd->changes.shaded)
     {
	if (bd->shaded)
	  ecore_x_window_lower(bd->client.shell_win);
	else
	  ecore_x_window_raise(bd->client.shell_win);
	bd->changes.size = 1;
	bd->changes.shaded = 0;
     }

   if ((bd->changes.pos) && (bd->changes.size))
     {
	if (bd->internal_ecore_evas)
	  ecore_evas_managed_move(bd->internal_ecore_evas,
				  bd->x + bd->client_inset.l,
				  bd->y + bd->client_inset.t);
//	printf("##- BORDER NEEDS POS/SIZE CHANGE 0x%x\n", bd->client.win);
	if ((bd->shaded) && (!bd->shading))
	  {
	     evas_obscured_clear(bd->bg_evas);
	     ecore_x_window_move_resize(bd->win, bd->x, bd->y, bd->w, bd->h);
	     ecore_x_window_move_resize(bd->event_win, 0, 0, bd->w, bd->h);
	     if (bd->internal_ecore_evas)
	       ecore_evas_move_resize(bd->internal_ecore_evas, 0, 0, bd->client.w, bd->client.h);
	     else
	       ecore_x_window_move_resize(bd->client.win, 0, 0, bd->client.w, bd->client.h);
	     ecore_evas_move_resize(bd->bg_ecore_evas, 0, 0, bd->w, bd->h);
	     evas_object_resize(bd->bg_object, bd->w, bd->h);
	     e_container_shape_resize(bd->shape, bd->w, bd->h);
	     e_container_shape_move(bd->shape, bd->x, bd->y);
	  }
	else
	  {
	     evas_obscured_clear(bd->bg_evas);
	     evas_obscured_rectangle_add(bd->bg_evas,
					 bd->client_inset.l, bd->client_inset.t,
					 bd->w - (bd->client_inset.l + bd->client_inset.r),
					 bd->h - (bd->client_inset.t + bd->client_inset.b));
	     ecore_x_window_move_resize(bd->win, bd->x, bd->y, bd->w, bd->h);
	     ecore_x_window_move_resize(bd->event_win, 0, 0, bd->w, bd->h);
	     ecore_x_window_move_resize(bd->client.shell_win,
					bd->client_inset.l, bd->client_inset.t,
					bd->w - (bd->client_inset.l + bd->client_inset.r),
					bd->h - (bd->client_inset.t + bd->client_inset.b));
	     if (bd->shading)
	       {
		  if (bd->shade.dir == E_DIRECTION_UP)
		    {
		       if (bd->internal_ecore_evas)
			 ecore_evas_move_resize(bd->internal_ecore_evas, 0,
						bd->h - (bd->client_inset.t + bd->client_inset.b) -
						bd->client.h,
						bd->client.w, bd->client.h);
		       else
			 ecore_x_window_move_resize(bd->client.win, 0,
						    bd->h - (bd->client_inset.t + bd->client_inset.b) -
						    bd->client.h,
						    bd->client.w, bd->client.h);
		    }
		  else if (bd->shade.dir == E_DIRECTION_LEFT)
		    {
		       if (bd->internal_ecore_evas)
			 ecore_evas_move_resize(bd->internal_ecore_evas,
						bd->w - (bd->client_inset.l + bd->client_inset.r) -
						bd->client.h,
						0, bd->client.w, bd->client.h);
		       else
			 ecore_x_window_move_resize(bd->client.win,
						    bd->w - (bd->client_inset.l + bd->client_inset.r) -
						    bd->client.h,
						    0, bd->client.w, bd->client.h);
		    }
		  else
		    {
		       if (bd->internal_ecore_evas)
			 ecore_evas_move_resize(bd->internal_ecore_evas, 0, 0, 
						bd->client.w, bd->client.h);
		       else
			 ecore_x_window_move_resize(bd->client.win, 0, 0,
						    bd->client.w, bd->client.h);
		    }
	       }
	     else
	       {
		  if (bd->internal_ecore_evas)
		    ecore_evas_move_resize(bd->internal_ecore_evas, 0, 0, 
					   bd->client.w, bd->client.h);
		  else
		    ecore_x_window_move_resize(bd->client.win, 0, 0,
					       bd->client.w, bd->client.h);
	       }
	     ecore_evas_move_resize(bd->bg_ecore_evas, 0, 0, bd->w, bd->h);
	     evas_object_resize(bd->bg_object, bd->w, bd->h);
	     e_container_shape_resize(bd->shape, bd->w, bd->h);
	     e_container_shape_move(bd->shape, bd->x, bd->y);
	  }
	bd->changes.pos = 0;
	bd->changes.size = 0;
    }
   else if (bd->changes.pos)
     {
	if (bd->internal_ecore_evas)
	  ecore_evas_managed_move(bd->internal_ecore_evas,
				  bd->x + bd->client_inset.l,
				  bd->y + bd->client_inset.t);
	ecore_x_window_move(bd->win, bd->x, bd->y);
	e_container_shape_move(bd->shape, bd->x, bd->y);
	bd->changes.pos = 0;
     }
   else if (bd->changes.size)
     {
	if (bd->internal_ecore_evas)
	  ecore_evas_managed_move(bd->internal_ecore_evas,
				  bd->x + bd->client_inset.l,
				  bd->y + bd->client_inset.t);
//	printf("##- BORDER NEEDS SIZE CHANGE 0x%x\n", bd->client.win);
	if (bd->shaded && !bd->shading)
	  {
	     evas_obscured_clear(bd->bg_evas);
	     ecore_x_window_move_resize(bd->event_win, 0, 0, bd->w, bd->h);
	     ecore_x_window_resize(bd->win, bd->w, bd->h);
	     if (bd->internal_ecore_evas)
	       ecore_evas_move_resize(bd->internal_ecore_evas, 0, 0, bd->client.w, bd->client.h);
	     else
	       ecore_x_window_move_resize(bd->client.win, 0, 0, bd->client.w, bd->client.h);
	     ecore_evas_move_resize(bd->bg_ecore_evas, 0, 0, bd->w, bd->h);
	     evas_object_resize(bd->bg_object, bd->w, bd->h);
	     e_container_shape_resize(bd->shape, bd->w, bd->h);
	  }
	else
	  {
	     evas_obscured_clear(bd->bg_evas);
	     evas_obscured_rectangle_add(bd->bg_evas,
					 bd->client_inset.l, bd->client_inset.t,
					 bd->w - (bd->client_inset.l + bd->client_inset.r),
					 bd->h - (bd->client_inset.t + bd->client_inset.b));
	     ecore_x_window_move_resize(bd->event_win, 0, 0, bd->w, bd->h);
	     ecore_x_window_resize(bd->win, bd->w, bd->h);
	     ecore_x_window_move_resize(bd->client.shell_win,
					bd->client_inset.l, bd->client_inset.t,
					bd->w - (bd->client_inset.l + bd->client_inset.r),
					bd->h - (bd->client_inset.t + bd->client_inset.b));
	     if (bd->shading)
	       {
		  if (bd->shade.dir == E_DIRECTION_UP)
		    {
		       if (bd->internal_ecore_evas)
			 ecore_evas_move_resize(bd->internal_ecore_evas, 0,
						bd->h - (bd->client_inset.t + bd->client_inset.b) -
						bd->client.h,
						bd->client.w, bd->client.h);
		       else
			 ecore_x_window_move_resize(bd->client.win, 0,
						    bd->h - (bd->client_inset.t + bd->client_inset.b) -
						    bd->client.h,
						    bd->client.w, bd->client.h);
		    }
		  else if (bd->shade.dir == E_DIRECTION_LEFT)
		    {
		       if (bd->internal_ecore_evas)
			 ecore_evas_move_resize(bd->internal_ecore_evas,
						bd->w - (bd->client_inset.l + bd->client_inset.r) -
						bd->client.h,
						0, bd->client.w, bd->client.h);
		       else
			 ecore_x_window_move_resize(bd->client.win,
						    bd->w - (bd->client_inset.l + bd->client_inset.r) -
						    bd->client.h,
						    0, bd->client.w, bd->client.h);
		    }
		  else
		    {
		       if (bd->internal_ecore_evas)
			 ecore_evas_move_resize(bd->internal_ecore_evas, 0, 0, 
						bd->client.w, bd->client.h);
		       else
			 ecore_x_window_move_resize(bd->client.win, 0, 0,
						    bd->client.w, bd->client.h);
		    }
	       }
	     else
	       {
		  if (bd->internal_ecore_evas)
		    ecore_evas_move_resize(bd->internal_ecore_evas, 0, 0, 
					   bd->client.w, bd->client.h);
		  else
		    ecore_x_window_move_resize(bd->client.win, 0, 0,
					       bd->client.w, bd->client.h);
	       }
	     ecore_evas_move_resize(bd->bg_ecore_evas, 0, 0, bd->w, bd->h);
	     evas_object_resize(bd->bg_object, bd->w, bd->h);
	     e_container_shape_resize(bd->shape, bd->w, bd->h);
	  }
	bd->changes.size = 0;
     }

   if (bd->changes.reset_gravity)
     {
	GRAV_SET(bd, ECORE_X_GRAVITY_NW);
	bd->changes.reset_gravity = 0;
     }
   
   if (bd->need_shape_merge)
     {
	if ((bd->shaped) || (bd->client.shaped))
	  {
	     Ecore_X_Window twin, twin2;
	     int x, y;
	     
	     twin = ecore_x_window_override_new(bd->win, 0, 0, bd->w, bd->h);
	     if (bd->shaped)
	       {
		  ecore_x_window_shape_window_set(twin, bd->bg_win);
	       }
	     else
	       {
		  Ecore_X_Rectangle rects[4];
		  
		  rects[0].x      = 0;
		  rects[0].y      = 0;
		  rects[0].width  = bd->w;
		  rects[0].height = bd->client_inset.t;
		  rects[1].x      = 0;
		  rects[1].y      = bd->client_inset.t;
		  rects[1].width  = bd->client_inset.l;
		  rects[1].height = bd->h - bd->client_inset.t - bd->client_inset.b;
		  rects[2].x      = bd->w - bd->client_inset.r;
		  rects[2].y      = bd->client_inset.t;
		  rects[2].width  = bd->client_inset.r;
		  rects[2].height = bd->h - bd->client_inset.t - bd->client_inset.b;
		  rects[3].x      = 0;
		  rects[3].y      = bd->h - bd->client_inset.b;
		  rects[3].width  = bd->w;
		  rects[3].height = bd->client_inset.b;
		  ecore_x_window_shape_rectangles_set(twin, rects, 4);
	       }
	     twin2 = ecore_x_window_override_new(bd->win, 0, 0, 
						 bd->w - bd->client_inset.l - bd->client_inset.r,
						 bd->h - bd->client_inset.t - bd->client_inset.b);
	     x = 0;
	     y = 0;
	     if ((bd->shading) || (bd->shaded))
	       {
		  if (bd->shade.dir == E_DIRECTION_UP)
		    y = bd->h - bd->client_inset.t - bd->client_inset.b - bd->client.h;
		  else if (bd->shade.dir == E_DIRECTION_LEFT)
		    x = bd->w - bd->client_inset.l - bd->client_inset.r - bd->client.w;
	       }
	     ecore_x_window_shape_window_set_xy(twin2, bd->client.win,
						x, y);
	     ecore_x_window_shape_rectangle_clip(twin2, 0, 0, 
						 bd->w - bd->client_inset.l - bd->client_inset.r,
						 bd->h - bd->client_inset.t - bd->client_inset.b);
	     ecore_x_window_shape_window_add_xy(twin, twin2,
						bd->client_inset.l, 
						bd->client_inset.t);
	     ecore_x_window_del(twin2);
	     ecore_x_window_shape_window_set(bd->win, twin);
	     ecore_x_window_del(twin);
	  }
	else
	  {
	     ecore_x_window_shape_mask_set(bd->win, 0);
	  }
//	bd->need_shape_export = 1;
	bd->need_shape_merge = 0;
     }
   
   if (bd->need_shape_export)
     {
	Ecore_X_Rectangle *rects, *orects;
	int num;
	
	rects = ecore_x_window_shape_rectangles_get(bd->win, &num);
	if (rects)
	  {
	     int changed;
	     
	     changed = 1;
	     if ((num == bd->shape_rects_num) && (bd->shape_rects))
	       {
		  int i;
		  
		  orects = bd->shape_rects;
		  changed = 0;
		  for (i = 0; i < num; i++)
		    {
		       if (rects[i].x < 0)
			 {
			    rects[i].width -= rects[i].x;
			    rects[i].x = 0;
			 }
		       if ((rects[i].x + rects[i].width) > bd->w)
			 rects[i].width = rects[i].width - rects[i].x;
		       if (rects[i].y < 0)
			 {
			    rects[i].height -= rects[i].y;
			    rects[i].y = 0;
			 }
		       if ((rects[i].y + rects[i].height) > bd->h)
			 rects[i].height = rects[i].height - rects[i].y;
		       
		       if ((orects[i].x != rects[i].x) ||
			   (orects[i].y != rects[i].y) ||
			   (orects[i].width != rects[i].width) ||
			   (orects[i].height != rects[i].height))
			 {
			    changed = 1;
			    break;
			 }
		    }
	       }
	     if (changed)
	       {
		  if (bd->client.shaped)
		    e_container_shape_solid_rect_set(bd->shape, 0, 0, 0, 0);
		  else
		    e_container_shape_solid_rect_set(bd->shape, bd->client_inset.l, bd->client_inset.t, bd->client.w, bd->client.h);
		  E_FREE(bd->shape_rects);
		  bd->shape_rects = rects;
		  bd->shape_rects_num = num;
		  e_container_shape_rects_set(bd->shape, rects, num);
	       }
	     else
	       free(rects);
	  }
	else
	  {
	     E_FREE(bd->shape_rects);
	     bd->shape_rects = NULL;
	     bd->shape_rects_num = 0;
	     e_container_shape_rects_set(bd->shape, NULL, 0);
	  }
	bd->need_shape_export = 0;
     }

   if ((bd->changes.visible) && (bd->visible) && (bd->new_client))
     {
	int x, y;

	if ((!bd->placed) && (!bd->re_manage) &&
	    (e_config->window_placement_policy == E_WINDOW_PLACEMENT_MANUAL) &&
	    (!((bd->client.icccm.transient_for != 0) ||
	       (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG))) &&
	    (!move) && (!resize))
	  {
	     /* Set this window into moving state */

	     bd->cur_mouse_action = e_action_find("window_move");
	     if (bd->cur_mouse_action)
	       {
		  if ((!bd->cur_mouse_action->func.end_mouse) &&
		      (!bd->cur_mouse_action->func.end))
		    bd->cur_mouse_action = NULL;
		  if (bd->cur_mouse_action)
		    {
		       ecore_x_pointer_xy_get(bd->zone->container->win, &x, &y);
		       bd->x = x - (bd->w >> 1);
		       bd->y = y - (bd->client_inset.t >> 1);
		       bd->changed = 1;
		       bd->changes.pos = 1;
		       if (bd->internal_ecore_evas)
			 ecore_evas_managed_move(bd->internal_ecore_evas,
						 bd->x + bd->client_inset.l,
						 bd->y + bd->client_inset.t);
		       ecore_x_icccm_move_resize_send(bd->client.win,
						      bd->x + bd->client_inset.l,
						      bd->y + bd->client_inset.t,
						      bd->client.w, bd->client.h);
		    }
	       }
	  }
	ecore_evas_show(bd->bg_ecore_evas);
	ecore_x_window_show(bd->win);
	if (bd->cur_mouse_action)
	  {
	     bd->moveinfo.down.x = bd->x;
	     bd->moveinfo.down.y = bd->y;
	     bd->moveinfo.down.w = bd->w;
	     bd->moveinfo.down.h = bd->h;
	     bd->mouse.current.mx = x;
	     bd->mouse.current.my = y;
	     bd->moveinfo.down.button = 0;
	     bd->moveinfo.down.mx = x;
	     bd->moveinfo.down.my = y;

	     grabbed = 1;
	     e_object_ref(E_OBJECT(bd->cur_mouse_action));
	     bd->cur_mouse_action->func.go(E_OBJECT(bd), NULL); 
	     if (e_config->border_raise_on_mouse_action)
	       e_border_raise(bd);
	     e_border_focus_set(bd, 1, 1);
	  }
	bd->changes.visible = 0;
     }

   if (bd->changes.icon)
     {
	if (bd->icon_object)
	  {
	     evas_object_del(bd->icon_object);
	     bd->icon_object = NULL;
	  }
	bd->icon_object = e_border_icon_add(bd, bd->bg_evas);
	if ((bd->focused) && (bd->icon_object))
	  edje_object_signal_emit(bd->icon_object, "active", "");
	if (bd->bg_object)
	  {
	     evas_object_show(bd->icon_object);
	     edje_object_part_swallow(bd->bg_object, "icon_swallow", bd->icon_object);
	  }
	else
	  {
	     evas_object_hide(bd->icon_object);
	  }

	  {
	     E_Event_Border_Icon_Change *ev;

	     ev = calloc(1, sizeof(E_Event_Border_Icon_Change));
	     ev->border = bd;
	     e_object_ref(E_OBJECT(bd));
//	     e_object_breadcrumb_add(E_OBJECT(bd), "border_icon_change_event");
	     ecore_event_add(E_EVENT_BORDER_ICON_CHANGE, ev,
			     _e_border_event_border_icon_change_free, NULL);
	  }
	bd->changes.icon = 0;
     }

   if (change_urgent)
     {
	if (bd->client.icccm.urgent)
	  edje_object_signal_emit(bd->bg_object, "urgent", "");
	else
	  edje_object_signal_emit(bd->bg_object, "not_urgent", "");
	/* FIXME: we should probably do something with the pager or
	 * maybe raising the window if it becomes urgent
	 */
     }
   
   bd->new_client = 0;
   
   bd->changed = 0;

   bd->changes.stack = 0;
   bd->changes.prop = 0;
   bd->changes.border = 0;
   
   if ((bd->take_focus) || (bd->want_focus))
     {
	bd->take_focus = 0;
	if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) ||
	    (bd->want_focus))
	  {
	     bd->want_focus = 0;
	     if (!bd->lock_focus_out)
	       e_border_focus_set(bd, 1, 1);
	  }
	else
	  {
	     if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG)
	       {
		  if ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) ||
		      ((e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED) &&
		       (e_border_find_by_client_window(bd->client.icccm.transient_for) ==
			e_border_focused_get())))
		    {
		       if (!bd->lock_focus_out)
			 {
			    e_border_focus_set(bd, 1, 1);
			 }
		    }
	       }
	  }
     }

   if (bd->need_maximize)
     {
	E_Maximize max;
	max = bd->maximized;
	bd->maximized = E_MAXIMIZE_NONE;
	e_border_maximize(bd, max);
	bd->need_maximize = 0;
     }
 
   if (bd->need_fullscreen)
     {
	e_border_fullscreen(bd, e_config->fullscreen_policy);
	bd->need_fullscreen = 0;
     }
 
   if (bd->remember)
     e_remember_update(bd->remember, bd);
}

static void
_e_border_moveinfo_gather(E_Border *bd, const char *source)
{
   if (e_util_glob_match(source, "mouse,*,1")) bd->moveinfo.down.button = 1;
   else if (e_util_glob_match(source, "mouse,*,2")) bd->moveinfo.down.button = 2;
   else if (e_util_glob_match(source, "mouse,*,3")) bd->moveinfo.down.button = 3;
   else bd->moveinfo.down.button = 0;
   if ((bd->moveinfo.down.button >= 1) && (bd->moveinfo.down.button <= 3))
     {
	bd->moveinfo.down.mx = bd->mouse.last_down[bd->moveinfo.down.button - 1].mx;
	bd->moveinfo.down.my = bd->mouse.last_down[bd->moveinfo.down.button - 1].my;
     }
   else
     {
	bd->moveinfo.down.mx = bd->mouse.current.mx;
	bd->moveinfo.down.my = bd->mouse.current.my;
     }
}

static void
_e_border_resize_handle(E_Border *bd)
{
   int x, y, w, h;
   int new_x, new_y, new_w, new_h;
   int tw, th;
   Evas_List *skiplist = NULL;

   x = bd->x;
   y = bd->y;
   w = bd->w;
   h = bd->h;

   if ((bd->resize_mode == RESIZE_TR) ||
       (bd->resize_mode == RESIZE_R) ||
       (bd->resize_mode == RESIZE_BR))
     {
	if ((bd->moveinfo.down.button >= 1) &&
	    (bd->moveinfo.down.button <= 3))
	  w = bd->mouse.last_down[bd->moveinfo.down.button - 1].w +
	  (bd->mouse.current.mx - bd->moveinfo.down.mx);
	else
	  w = bd->moveinfo.down.w + (bd->mouse.current.mx - bd->moveinfo.down.mx);
     }
   else if ((bd->resize_mode == RESIZE_TL) ||
	    (bd->resize_mode == RESIZE_L) ||
	    (bd->resize_mode == RESIZE_BL))
     {
	if ((bd->moveinfo.down.button >= 1) &&
	    (bd->moveinfo.down.button <= 3))
	  w = bd->mouse.last_down[bd->moveinfo.down.button - 1].w -
	  (bd->mouse.current.mx - bd->moveinfo.down.mx);
	else
	  w = bd->moveinfo.down.w - (bd->mouse.current.mx - bd->moveinfo.down.mx);
     }

   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_T) ||
       (bd->resize_mode == RESIZE_TR))
     {
	if ((bd->moveinfo.down.button >= 1) &&
	    (bd->moveinfo.down.button <= 3))
	  h = bd->mouse.last_down[bd->moveinfo.down.button - 1].h -
	  (bd->mouse.current.my - bd->moveinfo.down.my);
	else
	  h = bd->moveinfo.down.h - (bd->mouse.current.my - bd->moveinfo.down.my);
     }
   else if ((bd->resize_mode == RESIZE_BL) ||
	    (bd->resize_mode == RESIZE_B) ||
	    (bd->resize_mode == RESIZE_BR))
     {
	if ((bd->moveinfo.down.button >= 1) &&
	    (bd->moveinfo.down.button <= 3))
	  h = bd->mouse.last_down[bd->moveinfo.down.button - 1].h +
	  (bd->mouse.current.my - bd->moveinfo.down.my);
	else
	  h = bd->moveinfo.down.h + (bd->mouse.current.my - bd->moveinfo.down.my);
     }

   tw = bd->w;
   th = bd->h;

   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_L) ||
       (bd->resize_mode == RESIZE_BL))
     x += (tw - w);
   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_T) ||
       (bd->resize_mode == RESIZE_TR))
     y += (th - h);

   skiplist = evas_list_append(skiplist, bd);
   e_resist_container_border_position(bd->zone->container, skiplist,
				      bd->x, bd->y, bd->w, bd->h,
				      x, y, w, h,
				      &new_x, &new_y, &new_w, &new_h);
   evas_list_free(skiplist);

   w = new_w;
   h = new_h;
   e_border_resize_limit(bd, &new_w, &new_h);
   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_L) ||
       (bd->resize_mode == RESIZE_BL))
     new_x += (w - new_w);
   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_T) ||
       (bd->resize_mode == RESIZE_TR))
     new_y += (h - new_h);

   e_border_move_resize(bd, new_x, new_y, new_w, new_h);
}

static int
_e_border_shade_animator(void *data)
{
   E_Border *bd = data;
   double dt, val;
   double dur = bd->client.h / e_config->border_shade_speed;

   dt = ecore_time_get() - bd->shade.start;
   val = dt / dur;

   if (val < 0.0) val = 0.0;
   else if (val > 1.0) val = 1.0;

   if (e_config->border_shade_transition == E_TRANSITION_SINUSOIDAL)
     {
	if (bd->shaded)
	  bd->shade.val = (1 - cos(val * M_PI)) / 2.0;
	else
	  bd->shade.val = 0.5 + (cos(val * M_PI) / 2.0);
     }
   else if (e_config->border_shade_transition == E_TRANSITION_DECELERATE)
     {
	if (bd->shaded)
	  bd->shade.val = sin(val * M_PI / 2.0);
	else
	  bd->shade.val = 1 - sin(val * M_PI / 2.0);
     }
   else if (e_config->border_shade_transition == E_TRANSITION_ACCELERATE)
     {
	if (bd->shaded)
	  bd->shade.val = 1 - cos(val * M_PI / 2.0);
	else
	  bd->shade.val = cos(val * M_PI / 2.0);
     }
   else /* LINEAR if none of the others */
     {
	if (bd->shaded)
	  bd->shade.val = val;
	else
	  bd->shade.val = 1 - val;
     }

   /* due to M_PI's innacuracy, cos(M_PI/2) != 0.0, so we need this */
   if (bd->shade.val < 0.001) bd->shade.val = 0.0;
   else if (bd->shade.val > .999) bd->shade.val = 1.0;

   if (bd->shade.dir == E_DIRECTION_UP)
     {
	bd->h = bd->client_inset.t + bd->client_inset.b + bd->client.h * bd->shade.val;
     }
   else if (bd->shade.dir == E_DIRECTION_DOWN)
     {
	bd->h = bd->client_inset.t + bd->client_inset.b + bd->client.h * bd->shade.val;
	bd->y = bd->shade.y + bd->client.h * (1 - bd->shade.val);
	bd->changes.pos = 1;
     }
   else if (bd->shade.dir == E_DIRECTION_LEFT)
     {
	bd->w = bd->client_inset.l + bd->client_inset.r + bd->client.w * bd->shade.val;
     }
   else if (bd->shade.dir == E_DIRECTION_RIGHT)
     {
	bd->w = bd->client_inset.l + bd->client_inset.r + bd->client.w * bd->shade.val;
	bd->x = bd->shade.x + bd->client.w * (1 - bd->shade.val);
	bd->changes.pos = 1;
     }

   if ((bd->shaped) || (bd->client.shaped))
     {
	bd->need_shape_merge = 1;
	bd->need_shape_export = 1;
     }
   bd->changes.size = 1;
   bd->changed = 1;

   /* we're done */
   if ((bd->shaded && (bd->shade.val == 1)) ||
       ((!bd->shaded) && (bd->shade.val == 0)) )
     {
	E_Event_Border_Resize *ev;

	bd->shading = 0;
	bd->shaded = !(bd->shaded);
	bd->changes.size = 1;
	bd->changes.shaded = 1;
	bd->changes.shading = 1;
	bd->changed = 1;
	bd->shade.anim = NULL;

	if (bd->shaded)
	  {
	     edje_object_signal_emit(bd->bg_object, "shaded", "");
	  }
	else
	  {
	     edje_object_signal_emit(bd->bg_object, "unshaded", "");
	  }
	ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_NW);
	ev = calloc(1, sizeof(E_Event_Border_Resize));
	ev->border = bd;
	e_object_ref(E_OBJECT(bd));
//	e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
	ecore_event_add(E_EVENT_BORDER_RESIZE, ev, _e_border_event_border_resize_free, NULL);
	return 0;
     }

   return 1;
}

static void
_e_border_event_border_resize_free(void *data, void *ev)
{
   E_Event_Border_Resize *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_resize_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_move_free(void *data, void *ev)
{
   E_Event_Border_Move *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_move_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_add_free(void *data, void *ev)
{
   E_Event_Border_Add *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_add_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_remove_free(void *data, void *ev)
{
   E_Event_Border_Remove *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_remove_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_show_free(void *data, void *ev)
{
   E_Event_Border_Show *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_show_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_hide_free(void *data, void *ev)
{
   E_Event_Border_Hide *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_hide_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_iconify_free(void *data, void *ev)
{
   E_Event_Border_Iconify *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_iconify_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_uniconify_free(void *data, void *ev)
{
   E_Event_Border_Uniconify *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_uniconify_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_stick_free(void *data, void *ev)
{
   E_Event_Border_Stick *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_stick_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_unstick_free(void *data, void *ev)
{
   E_Event_Border_Unstick *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_unstick_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_zone_set_free(void *data, void *ev)
{
   E_Event_Border_Zone_Set *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_zone_set_event");
   e_object_unref(E_OBJECT(e->border));
   e_object_unref(E_OBJECT(e->zone));
   free(e);
}

static void
_e_border_event_border_desk_set_free(void *data, void *ev)
{
   E_Event_Border_Desk_Set *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_desk_set_event");
   e_object_unref(E_OBJECT(e->border));
   e_object_unref(E_OBJECT(e->desk));
   free(e);
}

static void
_e_border_event_border_stack_free(void *data, void *ev)
{
   E_Event_Border_Stack *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_raise_event");
   e_object_unref(E_OBJECT(e->border));
   if (e->stack)
     {
//	e_object_breadcrumb_del(E_OBJECT(e->above), "border_raise_event.above");
	e_object_unref(E_OBJECT(e->stack));
     }
   free(e);
}

static void
_e_border_event_border_icon_change_free(void *data, void *ev)
{
   E_Event_Border_Icon_Change *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_icon_change_event");
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_focus_in_free(void *data, void *ev)
{
   E_Event_Border_Focus_In *e;

   e = ev;
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_event_border_focus_out_free(void *data, void *ev)
{
   E_Event_Border_Focus_Out *e;

   e = ev;
   e_object_unref(E_OBJECT(e->border));
   free(e);
}

static void
_e_border_zone_update(E_Border *bd)
{
   E_Container *con;
   Evas_List *l;

   /* still within old zone - leave it there */
   if (E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
		    bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
     return;
   /* find a new zone */
   con = bd->zone->container;
   for (l = con->zones; l; l = l->next)
     {
	E_Zone *zone;

	zone = l->data;
	if (E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
			 zone->x, zone->y, zone->w, zone->h))
	  {
	     e_border_zone_set(bd, zone);
	     return;
	  }
     }
}

static int
_e_border_resize_begin(E_Border *bd)
{
   int w, h;

   if (!bd->lock_user_stacking)
     {
	if (e_config->border_raise_on_mouse_action)
	  e_border_raise(bd);
     }
   if ((bd->shaded) || (bd->shading) ||
       (((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN) && (!e_config->allow_manip)) ||
       (bd->fullscreen) || (bd->lock_user_size))
     return 0;

   if ((bd->client.icccm.base_w >= 0) &&
       (bd->client.icccm.base_h >= 0))
     {
	if (bd->client.icccm.step_w > 0)
	  w = (bd->client.w - bd->client.icccm.base_w) / bd->client.icccm.step_w;
	else
	  w = bd->client.w;
	if (bd->client.icccm.step_h > 0)
	  h = (bd->client.h - bd->client.icccm.base_h) / bd->client.icccm.step_h;
	else
	  h = bd->client.h;
     }
   else
     {
	if (bd->client.icccm.step_w > 0)
	  w = (bd->client.w - bd->client.icccm.min_w) / bd->client.icccm.step_w;
	else
	  w = bd->client.w;
	if (bd->client.icccm.step_h > 0)
	  h = (bd->client.h - bd->client.icccm.min_h) / bd->client.icccm.step_h;
	else
	  h = bd->client.h;
     }
   if (grabbed)
     e_grabinput_get(bd->win, 0, bd->win);
   if (bd->client.netwm.sync.request)
     {
	bd->client.netwm.sync.alarm = ecore_x_sync_alarm_new(bd->client.netwm.sync.counter);
	bd->client.netwm.sync.serial = 0;
	bd->client.netwm.sync.wait = 0;
	bd->client.netwm.sync.send_time = ecore_time_get();
     }
   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(bd->x, bd->y, bd->w, bd->h); 
   else
     e_move_resize_object_coords_set(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
   e_resize_begin(bd->zone, w, h);
   resize = bd;
   return 1;
}

static int
_e_border_resize_end(E_Border *bd)
{
   if (grabbed)
     e_grabinput_release(bd->win, bd->win);
   grabbed = 0;
   if (bd->client.netwm.sync.alarm)
     {
	ecore_x_sync_alarm_free(bd->client.netwm.sync.alarm);
	bd->client.netwm.sync.alarm = 0;
     }
   e_resize_end();
   resize = NULL;
   return 1;
}

static void
_e_border_resize_update(E_Border *bd)
{
   int w, h;

   if ((bd->client.icccm.base_w >= 0) &&
       (bd->client.icccm.base_h >= 0))
     {
	if (bd->client.icccm.step_w > 0)
	  w = (bd->client.w - bd->client.icccm.base_w) / bd->client.icccm.step_w;
	else
	  w = bd->client.w;
	if (bd->client.icccm.step_h > 0)
	  h = (bd->client.h - bd->client.icccm.base_h) / bd->client.icccm.step_h;
	else
	  h = bd->client.h;
     }
   else
     {
	if (bd->client.icccm.step_w > 0)
	  w = (bd->client.w - bd->client.icccm.min_w) / bd->client.icccm.step_w;
	else
	  w = bd->client.w;
	if (bd->client.icccm.step_h > 0)
	  h = (bd->client.h - bd->client.icccm.min_h) / bd->client.icccm.step_h;
	else
	  h = bd->client.h;
     }
   if (e_config->resize_info_follows)
     e_move_resize_object_coords_set(bd->x, bd->y, bd->w, bd->h);
   else
     e_move_resize_object_coords_set(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
   e_resize_update(w, h);
}

static int
_e_border_move_begin(E_Border *bd)
{
   if (!bd->lock_user_stacking)
     {
	if (e_config->border_raise_on_mouse_action)
	  e_border_raise(bd);
     }
   if ((((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN) && (!e_config->allow_manip)) ||
       (bd->fullscreen) || (bd->lock_user_location))
     return 0;

   if (grabbed)
     e_grabinput_get(bd->win, 0, bd->win);
#if 0
   if (bd->client.netwm.sync.request)
     {
	bd->client.netwm.sync.alarm = ecore_x_sync_alarm_new(bd->client.netwm.sync.counter);
	bd->client.netwm.sync.serial = 0;
	bd->client.netwm.sync.wait = 0;
	bd->client.netwm.sync.time = ecore_time_get();
     }
#endif
   if (e_config->move_info_follows)
     e_move_resize_object_coords_set(bd->x, bd->y, bd->w, bd->h); 
   else
     e_move_resize_object_coords_set(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
   e_move_begin(bd->zone, bd->x, bd->y);
   move = bd;
   return 1;
}

static int
_e_border_move_end(E_Border *bd)
{
   if (grabbed)
     e_grabinput_release(bd->win, bd->win);
   grabbed = 0;
#if 0
   if (bd->client.netwm.sync.alarm)
     {
	ecore_x_sync_alarm_free(bd->client.netwm.sync.alarm);
	bd->client.netwm.sync.alarm = 0;
     }
#endif
   e_move_end();
   move = NULL;
   return 1;
}

static void
_e_border_move_update(E_Border *bd)
{
   if (e_config->move_info_follows)
     e_move_resize_object_coords_set(bd->x, bd->y, bd->w, bd->h); 
   else
     e_move_resize_object_coords_set(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
   e_move_update(bd->x, bd->y);
}

static int
_e_border_cb_ping_timer(void *data)
{
   E_Border *bd;
   
   bd = data;
   if (bd->ping_ok)
     {
	if (bd->hung)
	  {
	     bd->hung = 0;
	     edje_object_signal_emit(bd->bg_object, "unhung", "");
	     if (bd->kill_timer)
	       {
		  ecore_timer_del(bd->kill_timer);
		  bd->kill_timer = NULL;
	       }
	  }
     }
   else
     {
	if (!bd->hung)
	  {
	     bd->hung = 1;
	     edje_object_signal_emit(bd->bg_object, "hung", "");
	     /* FIXME: if below dialog is up - hide it now */
	  }
	if (bd->delete_requested)
	  {
	     /* FIXME: pop up dialog saying app is hung - kill client, or pid */
	     e_border_act_kill_begin(bd);
	  }
     }
   bd->ping_timer = NULL;
   e_border_ping(bd);
   return 0;
}

static int
_e_border_cb_kill_timer(void *data)
{
   E_Border *bd;
   
   bd = data;
   if (bd->hung)
     {
	if (bd->client.netwm.pid > 1)
	  kill(bd->client.netwm.pid, SIGKILL);
     }
   bd->kill_timer = NULL;
   return 0;
}

static char *
_e_border_winid_str_get(Ecore_X_Window win)
{
   const char *vals = "qWeRtYuIoP5-$&<~";
   static char id[9];
   unsigned int val;
   
   val = (unsigned int)win;
   id[0] = vals[(val >> 28) & 0xf];
   id[1] = vals[(val >> 24) & 0xf];
   id[2] = vals[(val >> 20) & 0xf];
   id[3] = vals[(val >> 16) & 0xf];
   id[4] = vals[(val >> 12) & 0xf];
   id[5] = vals[(val >>  8) & 0xf];
   id[6] = vals[(val >>  4) & 0xf];
   id[7] = vals[(val      ) & 0xf];
   id[8] = 0;
   return id;
}

static void
_e_border_app_change(void *data, E_App *app, E_App_Change change)
{
   Evas_List *l;

   switch (change)
     {
      case E_APP_ADD:
      case E_APP_DEL:
      case E_APP_CHANGE:
	for (l = borders; l; l = l->next)
	  {
	     E_Border *bd;
	     
	     bd = l->data;
//	     if (e_app_equals(bd->app, app))
	       {
		  if (bd->app)
		    {
		       e_object_unref(E_OBJECT(bd->app));
		       bd->app = NULL;
		    }
		  
		  bd->changes.icon = 1;
		  bd->changed = 1;
	       }
	  }
	break;
      case E_APP_EXEC:
      case E_APP_READY:
      case E_APP_READY_EXPIRE:
      case E_APP_EXIT:
      default:
	break;
     }
}

static void
_e_border_pointer_resize_begin(E_Border *bd)
{
   switch (bd->resize_mode)
     {
      case RESIZE_TL:
	 e_pointer_type_push(bd->zone->container->manager->pointer, bd, "resize_tl");
	 break;
      case RESIZE_T:
	 e_pointer_type_push(bd->zone->container->manager->pointer, bd, "resize_t");
	 break;
      case RESIZE_TR:
	 e_pointer_type_push(bd->zone->container->manager->pointer, bd, "resize_tr");
	 break;
      case RESIZE_R:
	 e_pointer_type_push(bd->zone->container->manager->pointer, bd, "resize_r");
	 break;
      case RESIZE_BR:
	 e_pointer_type_push(bd->zone->container->manager->pointer, bd, "resize_br");
	 break;
      case RESIZE_B:
	 e_pointer_type_push(bd->zone->container->manager->pointer, bd, "resize_b");
	 break;
      case RESIZE_BL:
	 e_pointer_type_push(bd->zone->container->manager->pointer, bd, "resize_bl");
	 break;
      case RESIZE_L:
	 e_pointer_type_push(bd->zone->container->manager->pointer, bd, "resize_l");
	 break;
     }
}

static void
_e_border_pointer_resize_end(E_Border *bd)
{
   switch (bd->resize_mode)
     {
      case RESIZE_TL:
	 e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "resize_tl");
	 break;
      case RESIZE_T:
	 e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "resize_t");
	 break;
      case RESIZE_TR:
	 e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "resize_tr");
	 break;
      case RESIZE_R:
	 e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "resize_r");
	 break;
      case RESIZE_BR:
	 e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "resize_br");
	 break;
      case RESIZE_B:
	 e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "resize_b");
	 break;
      case RESIZE_BL:
	 e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "resize_bl");
	 break;
      case RESIZE_L:
	 e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "resize_l");
	 break;
     }
}

static void
_e_border_pointer_move_begin(E_Border *bd)
{
   e_pointer_type_push(bd->zone->container->manager->pointer, bd, "move");
}

static void
_e_border_pointer_move_end(E_Border *bd)
{
   e_pointer_type_pop(bd->zone->container->manager->pointer, bd, "move");
}
