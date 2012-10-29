#define BUILDING_NODE_EXTENSION

#include <v8.h>
#include <node.h>

#include "libfreenect.h"

#include <stdio.h>
#include <stdexcept>

using namespace node;
using namespace v8;

namespace kinect {

  class Context : ObjectWrap {
    public:
      static void   Initialize(v8::Handle<v8::Object> target);
      virtual       ~Context();

    private:
      Context();
      static Handle<Value>  New(const Arguments& args);
      static Context*       GetContext(const Arguments &args);
      void                  Close();
      static Handle<Value>  Close(const Arguments& args);

      freenect_context*     context_;
      freenect_device*      device_;  
  };

  Handle<Value>
  Context::New(const Arguments& args) {
    HandleScope scope;

    assert(args.IsConstructCall());

    Context *context = new Context();
    context->Wrap(args.This());

    return args.This();
  }

  void
  Context::Initialize(v8::Handle<v8::Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);

    target->Set(String::NewSymbol("Context"), t->GetFunction());
  }

  Context::Context() : ObjectWrap() {
    if (freenect_init(&context_, NULL) < 0) {
      throw std::runtime_error("Error initializing freenect context");
    }
    freenect_set_log_level(context_, FREENECT_LOG_DEBUG);
    freenect_select_subdevices(context_, (freenect_device_flags)(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));
    int nr_devices = freenect_num_devices (context_);
    if (nr_devices < 1) {
      throw std::runtime_error("No devices present");
    }

    int user_device_number = 0;

    // TODO: select device in constructor

    if (freenect_open_device(context_, &device_, user_device_number) < 0) {
      std::runtime_error("Could not open device number\n");
      Close();
    }
  }

  Context *
  Context::GetContext(const Arguments &args) {
    return ObjectWrap::Unwrap<Context>(args.This());
  }

  void
  Context::Close() {
    if (context_ != NULL) {
      if (freenect_shutdown(context_) < 0) throw std::runtime_error("Error shutting down");
      context_ = NULL;
    }
  }

  Context::~Context() {
    Close();
  }

  Handle<Value>
  Context::Close(const Arguments& args) {
    HandleScope scope;
    GetContext(args)->Close();
    return Undefined();
   }

  Handle<Value>
  Initialize(Handle<Object> target) {
    Context::Initialize(target);
  }
}

extern "C" void
init(Handle<Object> target)
{
  kinect::Initialize(target);
}