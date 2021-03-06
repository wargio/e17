#include "e.h"
#include "e_mod_main.h"
#include "e_mod_parse.h"

/* GADCON */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char *_gc_label(E_Gadcon_Client_Class *client_class);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);

/* EVENTS */
static Eina_Bool _xkb_changed(void *data, int type, void *event_info);
static void _e_xkb_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_xkb_cb_menu_post(void *data, E_Menu *menu __UNUSED__);
static void _e_xkb_cb_menu_configure(void *data, E_Menu *mn, E_Menu_Item *mi __UNUSED__);
static void _e_xkb_cb_lmenu_post(void *data, E_Menu *menu __UNUSED__);
static void _e_xkb_cb_lmenu_set(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__);

/* Static variables
 * The static variables specific to the current code unit.
 */

/* GADGET INSTANCE */

typedef struct _Instance 
{
   E_Gadcon_Client *gcc;
   
   Evas_Object *o_xkbswitch;
   Evas_Object *o_xkbflag;
   
   E_Menu *menu;
   E_Menu *lmenu;
} Instance;

/* LIST OF INSTANCES */
static Eina_List *instances = NULL;

/* Global variables
 * Global variables shared across the module.
 */

/* CONFIG STRUCTURE */
Xkb _xkb = { NULL, NULL, NULL };

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION,
     "xkbswitch", 
     { 
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "XKB Switcher"
};

/* Module initializer
 * Initializes the configuration file, checks its versions, populates
 * menus, finds the rules file, initializes gadget icon.
 */
EAPI void *
e_modapi_init(E_Module *m) 
{
   /* Menus and dialogs */
   e_configure_registry_category_add("keyboard_and_mouse", 80, _("Input"), 
                                     NULL, "preferences-behavior");
   e_configure_registry_item_add("keyboard_and_mouse/xkbswitch", 110,
                                 _("XKB Switcher"),  NULL,
                                 "preferences-desktop-locale", 
                                 _xkb_cfg_dialog);
   _xkb.module = m;
   _xkb.evh = ecore_event_handler_add(E_EVENT_XKB_CHANGED, _xkb_changed, NULL);
   /* Gadcon */
   e_gadcon_provider_register(&_gc_class);
   return m;
}

/* Module shutdown
 * Called when the module gets unloaded. Deregisters the menu state
 * and frees up the config.
 */
EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_configure_registry_item_del("keyboard_and_mouse/xkbswitch");
   e_configure_registry_category_del("keyboard_and_mouse");

   if (_xkb.evh) ecore_event_handler_del(_xkb.evh);
   if (_xkb.cfd) e_object_del(E_OBJECT(_xkb.cfd));
   _xkb.cfd    = NULL;
   _xkb.module = NULL;
   e_gadcon_provider_unregister(&_gc_class);
   
   return 1;
}

/* Module state save
 * Used to save the configuration file.
 */
EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

/* Updates icons on all available xkbswitch gadgets to reflect the
 * current layout state.
 */
void
_xkb_update_icon(void)
{
   Instance *inst;
   Eina_List *l;
   const char *name;
   
   if (!e_config->xkb.used_layouts) return;
   name = ((E_Config_XKB_Layout*)
           eina_list_data_get(e_config->xkb.used_layouts))->name;

   if ((name) && (strchr(name, '/'))) name = strchr(name, '/') + 1;
   
   if (e_config->xkb.only_label)
     {
        EINA_LIST_FOREACH(instances, l, inst)
          {
             if (inst->o_xkbflag)
               {
                  evas_object_del(inst->o_xkbflag);
                  inst->o_xkbflag = NULL;
               }
             e_theme_edje_object_set(inst->o_xkbswitch, 
                                     "base/theme/modules/xkbswitch", 
                                     "modules/xkbswitch/noflag");
             edje_object_part_text_set(inst->o_xkbswitch,
                                       "e.text.label", name);
          }
     }
   else
     {
        EINA_LIST_FOREACH(instances, l, inst)
          {
             if (!inst->o_xkbflag)
               inst->o_xkbflag = e_icon_add(inst->gcc->gadcon->evas);
             e_theme_edje_object_set(inst->o_xkbswitch,
                                     "base/theme/modules/xkbswitch", 
                                     "modules/xkbswitch/main");
             e_xkb_e_icon_flag_setup(inst->o_xkbflag, name);
             edje_object_part_swallow(inst->o_xkbswitch, "e.swallow.flag",
                                      inst->o_xkbflag);
             edje_object_part_text_set(inst->o_xkbswitch, "e.text.label",
                                       e_xkb_layout_name_reduce(name));
          }
     }
}

/* LOCAL STATIC FUNCTIONS */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *gcname, const char *id, const char *style) 
{
   Instance   *inst;
   const char *name;
   
   if (e_config->xkb.used_layouts)
     name = ((E_Config_XKB_Layout*)
             eina_list_data_get(e_config->xkb.used_layouts))->name;
   else name = NULL;

   /* The instance */
   inst = E_NEW(Instance, 1);
   /* The gadget */
   inst->o_xkbswitch = edje_object_add(gc->evas);
   if (e_config->xkb.only_label)
     e_theme_edje_object_set(inst->o_xkbswitch, 
                             "base/theme/modules/xkbswitch", 
                             "modules/xkbswitch/noflag");
   else
     e_theme_edje_object_set(inst->o_xkbswitch,
                             "base/theme/modules/xkbswitch", 
                             "modules/xkbswitch/main");
   edje_object_part_text_set(inst->o_xkbswitch, "e.text.label",
                             e_xkb_layout_name_reduce(name));
   /* The gadcon client */
   inst->gcc = e_gadcon_client_new(gc, gcname, id, style, inst->o_xkbswitch);
   inst->gcc->data = inst;
   /* The flag icon */
   if (!e_config->xkb.only_label)
     {
        inst->o_xkbflag = e_icon_add(gc->evas);
        e_xkb_e_icon_flag_setup(inst->o_xkbflag, name);
        /* The icon is part of the gadget. */
        edje_object_part_swallow(inst->o_xkbswitch, "e.swallow.flag",
                                 inst->o_xkbflag);
     }
   else inst->o_xkbflag = NULL;
   
   /* Hook some menus */
   evas_object_event_callback_add(inst->o_xkbswitch, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_xkb_cb_mouse_down, inst);
   /* Make the list know about the instance */
   instances = eina_list_append(instances, inst);
   
   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst;
   
   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   
   if (inst->menu) 
     {
        e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
     }
   if (inst->lmenu) 
     {
        e_menu_post_deactivate_callback_set(inst->lmenu, NULL, NULL);
        e_object_del(E_OBJECT(inst->lmenu));
        inst->lmenu = NULL;
     }
   if (inst->o_xkbswitch) 
     {
        evas_object_event_callback_del(inst->o_xkbswitch,
                                       EVAS_CALLBACK_MOUSE_DOWN, 
                                       _e_xkb_cb_mouse_down);
        evas_object_del(inst->o_xkbswitch);
        evas_object_del(inst->o_xkbflag);
     }
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__) 
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("XKB Switcher");
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__) 
{
   return _gc_class.name;
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas) 
{
   Evas_Object *o;
   char buf[PATH_MAX];
   
   snprintf(buf, sizeof(buf), "%s/e-module-xkbswitch.edj", _xkb.module->dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buf, "icon");
   return o;
}

static Eina_Bool
_xkb_changed(void *data __UNUSED__, int type __UNUSED__, void *event_info __UNUSED__)
{
   _xkb_update_icon();
   return ECORE_CALLBACK_PASS_ON;
}

static int
_xkb_menu_items_sort(const void *data1, const void *data2)
{
   const E_Config_XKB_Layout *cl1 = data1;
   const E_Config_XKB_Layout *cl2 = data2;
   int v;
   
   v = strcmp(cl1->name, cl2->name);
   if (!v) v = strcmp(cl1->model, cl2->model);
   if (!v) v = strcmp(cl1->variant, cl2->variant);
   return v;
}

static void
_e_xkb_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event) 
{
   Evas_Event_Mouse_Down *ev = event;
   Instance *inst = data;
   
   if (!inst) return;
   
   if ((ev->button == 3) && (!inst->menu)) /* Right-click utility menu */
     {
        int x, y;
        E_Menu_Item *mi;
        
        /* The menu and menu item */
        inst->menu = e_menu_new();
        mi = e_menu_item_new(inst->menu);
        /* Menu item specifics */
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "preferences-system");
        e_menu_item_callback_set(mi, _e_xkb_cb_menu_configure, NULL);
        /* Append into the util menu */
        inst->menu = e_gadcon_client_util_menu_items_append(inst->gcc, 
                                                            inst->menu, 0);
        /* Callback */
        e_menu_post_deactivate_callback_set(inst->menu, _e_xkb_cb_menu_post,
                                            inst);
        /* Coords */
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y,
                                          NULL, NULL);
        /* Activate - we show the menu relative to the gadget */
        e_menu_activate_mouse(inst->menu,
                              e_util_zone_current_get(e_manager_current_get()),
                              (x + ev->output.x), (y + ev->output.y), 1, 1, 
                              E_MENU_POP_DIRECTION_AUTO, ev->timestamp);

        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
   else if ((ev->button == 1) && (!inst->lmenu)) /* Left-click layout menu */
     {
        Evas_Coord x, y, w, h;
        int cx, cy;
        
        /* Coordinates and sizing */
        evas_object_geometry_get(inst->o_xkbswitch, &x, &y, &w, &h);
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy,
                                          NULL, NULL);
        x += cx;
        y += cy;

        if (!inst->lmenu) inst->lmenu = e_menu_new();

        if (inst->lmenu)
          {
             E_Config_XKB_Layout *cl;
             E_Menu_Item *mi;
             Eina_List *l;
             int dir;
             char buf[PATH_MAX];
             Eina_List *tlist = NULL;
             
             mi = e_menu_item_new(inst->lmenu);
             
             e_menu_item_label_set(mi, _("Settings"));
             e_util_menu_item_theme_icon_set(mi, "preferences-system");
             e_menu_item_callback_set(mi, _e_xkb_cb_menu_configure, NULL);

             mi = e_menu_item_new(inst->lmenu);
             e_menu_item_separator_set(mi, 1);
             
             EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
               {
                  tlist = eina_list_append(tlist, cl);
               }
             tlist = eina_list_sort(tlist, eina_list_count(tlist),
                                    _xkb_menu_items_sort);
             
             /* Append all the layouts */
             EINA_LIST_FOREACH(tlist, l, cl)
               {
                  const char *name = cl->name;
                  
                  mi = e_menu_item_new(inst->lmenu);
                  
                  e_menu_item_radio_set(mi, 1);
                  e_menu_item_radio_group_set(mi, 1);
                  e_menu_item_toggle_set
                    (mi, (cl == e_config->xkb.used_layouts->data) ? 1 : 0);
                  e_xkb_flag_file_get(buf, sizeof(buf), name);
                  e_menu_item_icon_file_set(mi, buf);
                  snprintf(buf, sizeof(buf), "%s (%s, %s)", cl->name, 
                           cl->model, cl->variant);
                  e_menu_item_label_set(mi, buf);
                  e_menu_item_callback_set(mi, _e_xkb_cb_lmenu_set, cl);
               }
             
             if (tlist) eina_list_free(tlist);
             
             /* Deactivate callback */
             e_menu_post_deactivate_callback_set(inst->lmenu,
                                                 _e_xkb_cb_lmenu_post, inst);
            /* Proper menu orientation */
            switch (inst->gcc->gadcon->orient)
               {
                case E_GADCON_ORIENT_TOP:
                  dir = E_MENU_POP_DIRECTION_DOWN;
                  break;
                case E_GADCON_ORIENT_BOTTOM:
                  dir = E_MENU_POP_DIRECTION_UP;
                  break;
                case E_GADCON_ORIENT_LEFT:
                  dir = E_MENU_POP_DIRECTION_RIGHT;
                  break;
                case E_GADCON_ORIENT_RIGHT:
                  dir = E_MENU_POP_DIRECTION_LEFT;
                  break;
                case E_GADCON_ORIENT_CORNER_TL:
                  dir = E_MENU_POP_DIRECTION_DOWN;
                  break;
                case E_GADCON_ORIENT_CORNER_TR:
                  dir = E_MENU_POP_DIRECTION_DOWN;
                  break;
                case E_GADCON_ORIENT_CORNER_BL:
                  dir = E_MENU_POP_DIRECTION_UP;
                  break;
                case E_GADCON_ORIENT_CORNER_BR:
                  dir = E_MENU_POP_DIRECTION_UP;
                  break;
                case E_GADCON_ORIENT_CORNER_LT:
                  dir = E_MENU_POP_DIRECTION_RIGHT;
                  break;
                case E_GADCON_ORIENT_CORNER_RT:
                  dir = E_MENU_POP_DIRECTION_LEFT;
                  break;
                case E_GADCON_ORIENT_CORNER_LB:
                  dir = E_MENU_POP_DIRECTION_RIGHT;
                  break;
                case E_GADCON_ORIENT_CORNER_RB:
                  dir = E_MENU_POP_DIRECTION_LEFT;
                  break;
                case E_GADCON_ORIENT_FLOAT:
                case E_GADCON_ORIENT_HORIZ:
                case E_GADCON_ORIENT_VERT:
                default:
                    dir = E_MENU_POP_DIRECTION_AUTO;
                  break;
               }
             
             e_gadcon_locked_set(inst->gcc->gadcon, 1);
             
             /* We display not relatively to the gadget, but similarly to
              * the start menu - thus the need for direction etc.
              */
             e_menu_activate_mouse(inst->lmenu,
                                   e_util_zone_current_get
                                   (e_manager_current_get()),
                                   x, y, w, h, dir, ev->timestamp);
        }
     }
   else if (ev->button == 2) /* Middle click */
     e_xkb_layout_next();
}

static void
_e_xkb_cb_menu_post(void *data, E_Menu *menu __UNUSED__) 
{
   Instance *inst = data;
   
   if (!(inst) || !inst->menu) return;
   inst->menu = NULL;
}

static void
_e_xkb_cb_lmenu_post(void *data, E_Menu *menu __UNUSED__) 
{
   Instance *inst = data;

   if (!(inst) || !inst->lmenu) return;
   inst->lmenu = NULL;
}

static void
_e_xkb_cb_menu_configure(void *data __UNUSED__, E_Menu *mn, E_Menu_Item *mi __UNUSED__)
{
   if (_xkb.cfd) return;
   _xkb_cfg_dialog(mn->zone->container, NULL);
}

static void
_e_xkb_cb_lmenu_set(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__) 
{
   Eina_List *l;
   void *ndata;
   void *odata = eina_list_data_get(e_config->xkb.used_layouts);

   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, ndata)
     {
        if (ndata == data) eina_list_data_set(l, odata);
     }
   eina_list_data_set(e_config->xkb.used_layouts, data);
   e_xkb_update();
   _xkb_update_icon();
}
