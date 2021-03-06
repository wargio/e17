#include "CElmGrid.h"

namespace elm {

using namespace v8;

GENERATE_PROPERTY_CALLBACKS(CElmGrid, size);
GENERATE_METHOD_CALLBACKS(CElmGrid, clear);
GENERATE_METHOD_CALLBACKS(CElmGrid, pack);

GENERATE_TEMPLATE(CElmGrid,
                  PROPERTY(size),
                  METHOD(pack),
                  METHOD(clear));

CElmGrid::CElmGrid(Local<Object> _jsObject, CElmObject *parent)
   : CElmObject(_jsObject, elm_grid_add(parent->GetEvasObject()))
{
}

void CElmGrid::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Grid"), GetTemplate()->GetFunction());
}

Handle<Value> CElmGrid::clear(const Arguments&)
{
   return Undefined();
}

void CElmGrid::DidRealiseElement(Local<Value> val)
{
   pack(val->ToObject());
}

void CElmGrid::pack(Handle<Object> obj)
{
   int x, y, w, h;
   Local<Object> element = obj->Get(String::NewSymbol("element"))->ToObject();

   x = obj->Get(String::NewSymbol("x"))->IntegerValue();
   y = obj->Get(String::NewSymbol("y"))->IntegerValue();
   w = obj->Get(String::NewSymbol("w"))->IntegerValue();
   h = obj->Get(String::NewSymbol("h"))->IntegerValue();

   elm_grid_pack(GetEvasObject(), GetEvasObjectFromJavascript(element), x, y, w, h);
}

Handle<Value> CElmGrid::pack(const Arguments &args)
{
   if (!args[0]->IsObject())
     return Undefined();

   Local<Object> desc = args[0]->ToObject();

   if (!desc->Has(String::NewSymbol("element")))
     {
        ELM_ERR("Need an elm element to be packed");
        return Undefined();
     }

   Local<Object> obj = desc->Clone();
   obj->Set(String::NewSymbol("element"),
            Realise(desc->Get(String::New("element")), jsObject));
   pack(obj);

   return Undefined();
}

void CElmGrid::size_set(Handle<Value> val)
{
   Local<Object> obj = val->ToObject();
   Local<Value> x = obj->Get(String::NewSymbol("x"));
   Local<Value> y = obj->Get(String::NewSymbol("y"));

   if (x.IsEmpty() || !x->IsNumber())
     return;

   if (y.IsEmpty() || !y->IsNumber())
     return;

   elm_grid_size_set(eo, x->IntegerValue(), y->IntegerValue());
}

Handle<Value> CElmGrid::size_get() const
{
   HandleScope scope;

   int x, y;
   elm_grid_size_get (eo, &x, &y);

   Local<Object> obj = Object::New();
   obj->Set(String::New("x"), Number::New(x));
   obj->Set(String::New("y"), Number::New(y));

   return scope.Close(obj);
}

}
