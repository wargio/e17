#include "Eo.h"
#include "mixin.h"
#include "simple5.h"

#include "config.h"

#define MY_CLASS SIMPLE5_CLASS

static void
_destructor(Eo *obj, void *class_data EINA_UNUSED)
{
   (void) obj;
}

static const Eo_Class_Description class_desc = {
     "Simple5",
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(NULL, NULL, 0),
     NULL,
     0,
     NULL,
     _destructor,
     NULL,
     NULL
};

EO_DEFINE_CLASS(simple5_class_get, &class_desc, EO_BASE_CLASS, NULL);

