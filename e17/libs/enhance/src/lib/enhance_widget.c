#include "enhance_private.h"

static E_Widget *_e_widget_new(Enhance *en, EXML_Node *node, Etk_Widget *etk_widget, char *id);
static E_Widget *_e_widget_window_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_scrolled_view_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_viewport_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_vbox_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_hbox_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_alignment_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_table_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_tree_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_notebook_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_frame_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_hpaned_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_vpaned_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_label_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_image_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_button_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_check_button_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_toggle_button_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_radio_button_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_entry_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_progress_bar_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_statusbar_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_menu_bar_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_menu_item_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_menu_image_item_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_menu_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_hseparator_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_vseparator_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_hslider_handle(Enhance *en, EXML_Node *node);
static E_Widget *_e_widget_vslider_handle(Enhance *en, EXML_Node *node);

static E_Widget *
_e_widget_new(Enhance *en, EXML_Node *node, Etk_Widget *etk_widget, char *id)
{
   E_Widget  *widget;
   
   widget = E_NEW(1, E_Widget);
   widget->wid = etk_widget;
   widget->node = node;
   widget->packing = NULL;
   en->widgets = evas_hash_add(en->widgets, id, widget);

   return widget;
}

static E_Widget *
_e_widget_window_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *win;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;

   win = _e_widget_new(en, node, etk_window_new(), id);
   
   if(!strcmp(en->main_window, id))   
     etk_widget_show_all(win->wid);
   
   return win;
}

static E_Widget *
_e_widget_scrolled_view_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *view;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   view = _e_widget_new(en, node, etk_scrolled_view_new(), id);
   
   return view;
}

static E_Widget *
_e_widget_viewport_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *port;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   port = _e_widget_new(en, node, etk_viewport_new(), id);
   
   return port;
}

static E_Widget *
_e_widget_vbox_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *vbox;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   vbox = _e_widget_new(en, node, etk_vbox_new(ETK_TRUE, 0), id);
   
   return vbox;
}

static E_Widget *
_e_widget_hbox_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *hbox;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   hbox = _e_widget_new(en, node, etk_hbox_new(ETK_TRUE, 0), id);

   return hbox;
}

static E_Widget *
_e_widget_alignment_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *align;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   align = _e_widget_new(en, node, etk_alignment_new(0.0, 0.0, 0.0, 0.0), id);
   
   return align;
}

static E_Widget *
_e_widget_table_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *table;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   table = _e_widget_new(en, node, etk_table_new(1, 1, ETK_FALSE), id);
   
   return table;
}

static E_Widget *
_e_widget_tree_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *tree;
   char       *id;

   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   tree = _e_widget_new(en, node, etk_tree_new(), id);
   etk_tree_build(ETK_TREE(tree->wid));

   return tree;
}

static E_Widget *
_e_widget_notebook_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *notebook;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
    
   notebook = _e_widget_new(en, node, etk_notebook_new(), id);
   
   return notebook;
}

static E_Widget *
_e_widget_frame_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *frame;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   frame = _e_widget_new(en, node, etk_frame_new(NULL), id);
   
   return frame;
}

static E_Widget *
_e_widget_hpaned_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *paned;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   paned = _e_widget_new(en, node, etk_hpaned_new(), id);
   
   return paned;
}

static E_Widget *
_e_widget_vpaned_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *paned;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   paned = _e_widget_new(en, node, etk_vpaned_new(), id);
   
   return paned;
}

static E_Widget *
_e_widget_image_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *img;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   img = _e_widget_new(en, node, etk_image_new(), id);
   
   return img;
}

static E_Widget *
_e_widget_label_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *label;
   char       *id;   
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   label = _e_widget_new(en, node, etk_label_new(NULL), id);
   
   return label;
}

static E_Widget *
_e_widget_button_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *button;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   button = _e_widget_new(en, node, etk_button_new(), id);

   return button;
}

static E_Widget *
_e_widget_check_button_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *button;
   char      *id;   
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   button = _e_widget_new(en, node, etk_check_button_new(), id);
   
   return button;
}

static E_Widget *
_e_widget_toggle_button_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *button;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   button = _e_widget_new(en, node, etk_toggle_button_new(), id);
   
   return button;
}

static E_Widget *
_e_widget_radio_button_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *button;
   Etk_Widget *w = NULL;
   char       *label = NULL;
   char       *group = NULL;   
   char       *id;
   Ecore_List *props;
   EXML_Node  *prop;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   props = node->children;
   ecore_list_goto_first(props);
   prop = ecore_list_current(props);
   while(prop != NULL)
     {
	if(!strcmp(prop->tag, "property"))
	  {
	     char *name;
	     
	     name = ecore_hash_get(prop->attributes, "name");
	     if(!name) { prop = ecore_list_next(props); continue; }
	     
	     if(!strcmp(name, "label"))
	       {
		  if(prop->value)
		    label = strdup(prop->value);
	       }
	     else if(!strcmp(name, "group"))
	       {
		  if(prop->value)
		    group = strdup(prop->value);
	       }
	  }
	prop = ecore_list_next(props);	
     }
   
   ecore_list_goto_first(props);   
      
   if(group)     	
     w = evas_hash_find(en->radio_groups, group);   
   
   if(label)
     {
	if(w)     
	  button = _e_widget_new(en, node, 
			   etk_radio_button_new_with_label_from_widget(label, 
			   ETK_RADIO_BUTTON(w)),
			   id);
	else
	  button = _e_widget_new(en, node, 
				 etk_radio_button_new_with_label(label, NULL),
				 id);
     }
   else
     {
	if(w)     
	  button = _e_widget_new(en, node, 
			etk_radio_button_new_from_widget(ETK_RADIO_BUTTON(w)), 
			id);
	else
	  button = _e_widget_new(en, node, etk_radio_button_new(NULL), id);
     }
   
   if(!group)
     en->radio_groups = evas_hash_add(en->radio_groups, id, button->wid);   

   E_FREE(label);
   E_FREE(group);
   
   return button;
}

static E_Widget *
_e_widget_entry_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *entry;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   entry = _e_widget_new(en, node, etk_entry_new(), id);
         
   return entry;
}

static E_Widget *
_e_widget_progress_bar_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *bar;
   char      *id;

   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   bar = _e_widget_new(en, node, etk_progress_bar_new(), id);
   
   return bar;
}

static E_Widget *
_e_widget_statusbar_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *bar;
   char       *id;
      
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   bar = _e_widget_new(en, node, etk_statusbar_new(), id);
      
   return bar;
}

static E_Widget *
_e_widget_menu_bar_handle(Enhance *en, EXML_Node *node)
{
   E_Widget  *bar;
   char      *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   bar = _e_widget_new(en, node, etk_menu_bar_new(), id);
   
   return bar;
}

static E_Widget *
_e_widget_menu_item_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *item;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   item = _e_widget_new(en, node, etk_menu_item_new(), id);
      
   return item;
}

static E_Widget *
_e_widget_menu_image_item_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *item;
   char       *id;

   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;

   item = _e_widget_new(en, node, etk_menu_item_image_new(), id);

   return item;
}

static E_Widget *
_e_widget_menu_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *menu;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   menu = _e_widget_new(en, node, etk_menu_new(), id);
   
   return menu;
}

static E_Widget *
_e_widget_combo_box_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *combo;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   combo = _e_widget_new(en, node, etk_combobox_new(), id);
   
   return combo;
}

static E_Widget *
_e_widget_hseparator_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *sep;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   sep = _e_widget_new(en, node, etk_hseparator_new(), id);
   
   return sep;
}

static E_Widget *
_e_widget_vseparator_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *sep;
   char       *id;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
      
   sep = _e_widget_new(en, node, etk_vseparator_new(), id);
   
   return sep;
}

static E_Widget *
_e_widget_hslider_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *slider;
   char       *id;
   Ecore_List *props;
   EXML_Node  *prop;
   int         value;
   int         min       = 0;
   int         max       = 0;
   int         step_inc  = 0;
   int         page_inc  = 0;
   int         page_size = 0;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   props = node->children;
   ecore_list_goto_first(props);
   prop = ecore_list_current(props);
   while(prop != NULL)
     {
	if(!strcmp(prop->tag, "property"))
	  {
	     char *name;
	     
	     name = ecore_hash_get(prop->attributes, "name");
	     if(!name) { prop = ecore_list_next(props); continue; }
	     
	     if(!strcmp(name, "adjustment"))
	       {
		  if(prop->value)
		    {
		       char *adj;
		       
		       adj = strdup(prop->value);
		       sscanf(adj, "%d %d %d %d %d %d", &value, &min, &max,
			      &step_inc, &page_inc, &page_size);
		       E_FREE(adj);
		    }
	       }
	  }
	prop = ecore_list_next(props);
     }

   ecore_list_goto_first(props);
   
   slider = _e_widget_new(en, node, etk_hslider_new((double)min, (double)max, 
						    (double)value,
						    (double)step_inc,
						    (double)page_inc), id);   
   return slider;
}

static E_Widget *
_e_widget_vslider_handle(Enhance *en, EXML_Node *node)
{
   E_Widget   *slider;
   char       *id;
   Ecore_List *props;
   EXML_Node  *prop;
   int         value;
   int         min       = 0;
   int         max       = 0;
   int         step_inc  = 0;
   int         page_inc  = 0;
   int         page_size = 0;
   
   id = ecore_hash_get(node->attributes, "id");
   if(!id) return NULL;
   
   props = node->children;
   ecore_list_goto_first(props);
   prop = ecore_list_current(props);
   while(prop != NULL)
     {
	if(!strcmp(prop->tag, "property"))
	  {
	     char *name;
	     
	     name = ecore_hash_get(prop->attributes, "name");
	     if(!name) { prop = ecore_list_next(props); continue; }
	     
	     if(!strcmp(name, "adjustment"))
	       {
		  if(prop->value)
		    {
		       char *adj;

		       adj = strdup(prop->value);
		       sscanf(adj, "%d %d %d %d %d %d", &value, &min, &max,
			      &step_inc, &page_inc, &page_size);
		       E_FREE(adj);		       
		    }
	       }
	  }
	prop = ecore_list_next(props);
     }
   
   ecore_list_goto_first(props);
      
   slider = _e_widget_new(en, node, etk_vslider_new((double)min, (double)max, 
						    (double)value,
						    (double) step_inc, 
						    (double)page_inc), id);   
   return slider;
}

E_Widget *
_e_widget_handle(Enhance *en, EXML_Node *node)
{
   char *class;
   
   class = ecore_hash_get(node->attributes, "class");
   if(!class) return NULL;
   
#if DEBUG  
   printf("Handling widget: %s\n", class);		    
#endif
   
   if(!strcmp(class, "GtkWindow"))     
     return _e_widget_window_handle(en, node);        
   else if(!strcmp(class, "GtkVBox"))
     return _e_widget_vbox_handle(en, node);
   else if(!strcmp(class, "GtkVButtonBox"))
     return _e_widget_vbox_handle(en, node);   
   else if(!strcmp(class, "GtkHBox"))
     return _e_widget_hbox_handle(en, node);
   else if(!strcmp(class, "GtkHButtonBox"))
     return _e_widget_hbox_handle(en, node);   
   else if(!strcmp(class, "GtkFrame"))
     return _e_widget_frame_handle(en, node);
   else if(!strcmp(class, "GtkScrolledWindow"))
     return _e_widget_scrolled_view_handle(en, node);
   else if(!strcmp(class, "GtkViewport"))
     return _e_widget_viewport_handle(en, node);   
   else if(!strcmp(class, "GtkHPaned"))
     return _e_widget_hpaned_handle(en, node);
   else if(!strcmp(class, "GtkVPaned"))
     return _e_widget_vpaned_handle(en, node);   
   else if(!strcmp(class, "GtkButton"))
     return _e_widget_button_handle(en, node);
   else if(!strcmp(class, "GtkCheckButton"))
     return _e_widget_check_button_handle(en, node);
   else if(!strcmp(class, "GtkRadioButton"))
     return _e_widget_radio_button_handle(en, node);
   else if(!strcmp(class, "GtkToggleButton"))
     return _e_widget_toggle_button_handle(en, node);
   else if(!strcmp(class, "GtkLabel"))
     return _e_widget_label_handle(en, node);
   else if(!strcmp(class, "GtkImage"))
     return _e_widget_image_handle(en, node);   
   else if(!strcmp(class, "GtkEntry"))
     return _e_widget_entry_handle(en, node);
   else if(!strcmp(class, "GtkProgressBar"))
     return _e_widget_progress_bar_handle(en, node);   
   else if(!strcmp(class, "GtkAlignment"))
     return _e_widget_alignment_handle(en, node);
   else if(!strcmp(class, "GtkTable"))
     return _e_widget_table_handle(en, node);
   else if(!strcmp(class, "GtkTreeView"))
     return _e_widget_tree_handle(en, node);
   else if(!strcmp(class, "GtkNotebook"))
     return _e_widget_notebook_handle(en, node);   
   else if(!strcmp(class, "GtkStatusbar"))
     return _e_widget_statusbar_handle(en, node);
   else if(!strcmp(class, "GtkMenuBar"))
     return _e_widget_menu_bar_handle(en, node);
   else if(!strcmp(class, "GtkMenuItem"))
     return _e_widget_menu_item_handle(en, node);
   else if(!strcmp(class, "GtkImageMenuItem"))
     return _e_widget_menu_image_item_handle(en, node);
   else if(!strcmp(class, "GtkMenu"))
     return _e_widget_menu_handle(en, node);   
   else if(!strcmp(class, "GtkComboBox"))
     return _e_widget_combo_box_handle(en, node);   
   else if(!strcmp(class, "GtkHSeparator"))
     return _e_widget_hseparator_handle(en, node);
   else if(!strcmp(class, "GtkVSeparator"))
     return _e_widget_vseparator_handle(en, node);
   else if(!strcmp(class, "GtkHScale"))
     return _e_widget_hslider_handle(en, node);
   else if(!strcmp(class, "GtkVScale"))
     return _e_widget_vslider_handle(en, node);   
   return NULL;
}

void
_e_widget_parent_add(E_Widget *parent, E_Widget *child)
{
   char *parent_class;

   parent_class = ecore_hash_get(parent->node->attributes, "class");
   if(!parent_class) return;

#if DEBUG   
   printf("packing %s into %s\n", (char *)ecore_hash_get(child->node->attributes, "class"), parent_class);
#endif
   
   if(!strcmp(parent_class, "GtkWindow"))
     {
	etk_container_add(ETK_CONTAINER(parent->wid), child->wid);
     }
   if(!strcmp(parent_class, "GtkFrame"))
     {
	if(child->packing)
	  {
	     if(child->packing->type)
	       {
		  if(!strcmp(child->packing->type, "label_item"))
		    etk_frame_label_set(ETK_FRAME(parent->wid),
					etk_label_get(ETK_LABEL(child->wid)));
	       }
	  }
	else
	  etk_container_add(ETK_CONTAINER(parent->wid), child->wid);
     }
   if(!strcmp(parent_class, "GtkScrolledWindow"))
     {
	etk_container_add(ETK_CONTAINER(parent->wid), child->wid);
     }
   if(!strcmp(parent_class, "GtkViewport"))
     {
	etk_container_add(ETK_CONTAINER(parent->wid), child->wid);
     }   
   if(!strcmp(parent_class, "GtkHPaned") || !strcmp(parent_class, "GtkVPaned"))
     {
	Etk_Widget *w = NULL;
	Etk_Bool   expand = ETK_TRUE;
	
	if(child->packing)
	  {
	     if(child->packing->shrink == ETK_TRUE)
	       expand = ETK_FALSE;
	  }
	
	if((w = etk_paned_child1_get(ETK_PANED(parent->wid))) != NULL)	  
	  etk_paned_child2_set(ETK_PANED(parent->wid), child->wid, expand);
	else
	  etk_paned_child1_set(ETK_PANED(parent->wid), child->wid, expand);	
     }
   if(!strcmp(parent_class, "GtkNotebook"))
     {
	if(child->packing)
	  {
	     if(child->packing->type)
	       {
		  if(!strcmp(child->packing->type, "tab"))
		    {
		       etk_notebook_page_tab_label_set(ETK_NOTEBOOK(parent->wid),
 		        etk_notebook_current_page_get(ETK_NOTEBOOK(parent->wid)),
		        etk_label_get(ETK_LABEL(child->wid)));
		       etk_notebook_current_page_set(ETK_NOTEBOOK(parent->wid), 0);
		    }		       		  
	       }
	     else
	       {
		  int num;
		  
		  num = etk_notebook_page_append(ETK_NOTEBOOK(parent->wid), "ChangeMe", child->wid);
		  etk_notebook_current_page_set(ETK_NOTEBOOK(parent->wid), num);		  
	       }
	  }
	else
	  {
	     etk_notebook_page_append(ETK_NOTEBOOK(parent->wid), "ChangeMe", child->wid);
	  }
     }   
   if(!strcmp(parent_class, "GtkAlignment"))
     {
	etk_container_add(ETK_CONTAINER(parent->wid), child->wid);
     }   
   else if(!strcmp(parent_class, "GtkMenuItem"))
     {
	etk_menu_item_submenu_set(ETK_MENU_ITEM(parent->wid), 
				  ETK_MENU(child->wid));
     }
   else if(!strcmp(parent_class, "GtkMenuBar"))
     {
	etk_menu_shell_append(ETK_MENU_SHELL(parent->wid), 
			      ETK_MENU_ITEM(child->wid));
     }
   else if(!strcmp(parent_class, "GtkMenu"))
     {
	etk_menu_shell_append(ETK_MENU_SHELL(parent->wid), 
			      ETK_MENU_ITEM(child->wid));
     }   
   else if(!strcmp(parent_class, "GtkVBox") || !strcmp(parent_class, "GtkHBox") ||
	   !strcmp(parent_class, "GtkVButtonBox") || !strcmp(parent_class, "GtkButtonHBox"))
     {
	Etk_Bool expand  = ETK_TRUE;
	Etk_Bool fill    = ETK_TRUE;
	int      padding = 0;
	
	if(child->packing)
	  {
	     expand = child->packing->expand;
	     fill = child->packing->fill;	     
	     padding = child->packing->padding;
	  }
	
	etk_box_pack_start(ETK_BOX(parent->wid), child->wid, 
			   expand, fill, padding);
     }
   
   else if(!strcmp(parent_class, "GtkTable"))
     {
	int left_attach   = 0;
	int right_attach  = 0;
	int top_attach    = 0;
	int bottom_attach = 0;
	int x_padding     = 0;
	int y_padding     = 0;
	int flags_set     = 0;	
	Etk_Fill_Policy_Flags fill_policy;
	
	if(child->packing)
	  {
	     if(child->packing->left_attach > 0)
	       left_attach = child->packing->left_attach;
	     if(child->packing->right_attach > 0)
	       right_attach = child->packing->right_attach;
	     if(child->packing->top_attach > 0)
	       top_attach = child->packing->top_attach;
	     if(child->packing->bottom_attach > 0)
	       bottom_attach = child->packing->bottom_attach;
	     if(child->packing->y_padding > 0)
	       y_padding = child->packing->y_padding;
	     if(child->packing->y_padding > 0)
	       y_padding = child->packing->y_padding;
	     if(child->packing->x_options)
	       {
		  if(strstr(child->packing->x_options, "fill"))
		    {
		       fill_policy = ETK_FILL_POLICY_HFILL;
		       flags_set = 1;
		    }
		  else if(strstr(child->packing->x_options, "expand"))
		    {
		       if(flags_set)
			 fill_policy |= ETK_FILL_POLICY_HEXPAND;
		       else
			 fill_policy = ETK_FILL_POLICY_HEXPAND;
		       flags_set = 1;
		    }
		  else if(strstr(child->packing->y_options, "fill"))
		    {
		       if(flags_set)
			 fill_policy |= ETK_FILL_POLICY_VFILL;
		       else
			 fill_policy = ETK_FILL_POLICY_VFILL;
		       flags_set = 1;
		    }
		  else if(strstr(child->packing->y_options, "expand"))
		    {
		       if(flags_set)
			 fill_policy |= ETK_FILL_POLICY_VEXPAND;
		       else
			 fill_policy = ETK_FILL_POLICY_VEXPAND;
		       flags_set = 1;
		    }
	       }
	     
	     if(!flags_set)
	       fill_policy = ETK_FILL_POLICY_VFILL|ETK_FILL_POLICY_HFILL|
	                     ETK_FILL_POLICY_VEXPAND|ETK_FILL_POLICY_HEXPAND;
	  }
	
	/* NOTE: we have a problem here:
	 * in GTK, we do: table_attach(table, widget, 0, 1, ...)
	 * to attach to col 0 row 0. in etk, we need to do:
	 * table_attach(table, widget, 0, 0, ...)
	 */
	if(right_attach - left_attach == 1)
	  right_attach = left_attach;
	if(bottom_attach - top_attach == 1)
	  bottom_attach = top_attach;	
		       
	etk_table_attach(ETK_TABLE(parent->wid), child->wid, left_attach,
			 right_attach, top_attach, bottom_attach, 
			 x_padding, y_padding, fill_policy);
     }
}
