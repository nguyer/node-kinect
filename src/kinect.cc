#define BUILDING_NODE_EXTENSION

#include <v8.h>
#include <node.h>

#include "libfreenect.h"

#include <stdio.h>
#include <stdexcept>

#include <string>
//#include <iostream>

using namespace node;
using namespace v8;

namespace kinect {

  class Context : ObjectWrap {
    public:
      static void   Initialize (v8::Handle<v8::Object> target);
      virtual       ~Context   ();

    private:
      Context(int user_device_number);
      static Handle<Value>  New        (const Arguments& args);
      static Context*       GetContext (const Arguments &args);
      void                  Close      ();
      static Handle<Value>  Close      (const Arguments &args);
      void                  Led        (const std::string option);
      static Handle<Value>  Led        (const Arguments &args);

      freenect_context*     context_;
      freenect_device*      device_;  
  };

  Context *
  Context::GetContext(const Arguments &args) {
    return ObjectWrap::Unwrap<Context>(args.This());
  }


  /**** LED Control ******/

  void
  Context::Led(const std::string option) {
    freenect_led_options ledCode;

    if (option.compare("off") == 0) {
      ledCode = LED_OFF;
    } else if (option.compare("green") == 0) {
      ledCode = LED_GREEN;
    } else if (option.compare("red") == 0) {
      ledCode = LED_RED;
    } else if (option.compare("yellow") == 0) {
      ledCode = LED_YELLOW;
    } else if (option.compare("blink green") == 0) {
      ledCode = LED_BLINK_GREEN;
    } else if (option.compare("blink red yellow") == 0) {
      ledCode = LED_BLINK_RED_YELLOW;
    } else {
      ThrowException(Exception::Error(String::New("Did not recognize given led code")));
      return;
    }

    if (freenect_set_led(device_, ledCode) < 0) {
      ThrowException(Exception::Error(String::New("Error setting led")));
      return;
    }
  }

  Handle<Value>
  Context::Led(const Arguments& args) {
    HandleScope scope;

    if (args.Length() == 1) {
      if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(
          String::New("led argument must be a string")));
      }
    } else {
      return ThrowException(Exception::Error(String::New("Expecting at least one argument with the led status")));
    }

    String::AsciiValue val(args[0]->ToString());
    GetContext(args)->Led(std::string(*val));
    //GetContext(args)->Led(std::string("green"));
    return Undefined();
   }


  /********* Life Cycle ***********/

  Handle<Value>
  Context::New(const Arguments& args) {
    HandleScope scope;

    assert(args.IsConstructCall());

    int user_device_number = 0;
    if (args.Length() == 1) {
      if (!args[0]->IsNumber()) {
        return ThrowException(Exception::TypeError(
          String::New("user_device_number must be an integer")));
      }
      user_device_number = (int) args[0]->ToInteger()->Value();
      if (user_device_number < 0) {
        return ThrowException(Exception::RangeError(
          String::New("user_device_number must be a natural number")));
      }
    }

    Context *context = new Context(user_device_number);
    context->Wrap(args.This());

    return args.This();
  }

  void
  Context::Initialize(v8::Handle<v8::Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(t, "led",   Led);

    target->Set(String::NewSymbol("Context"), t->GetFunction());
  }

  Context::Context(int user_device_number) : ObjectWrap() {
    context_ = NULL;
    device_  = NULL;

    if (freenect_init(&context_, NULL) < 0) {
      ThrowException(Exception::Error(String::New("Error initializing freenect context")));
      return;
    }
    freenect_set_log_level(context_, FREENECT_LOG_DEBUG);
    freenect_select_subdevices(context_, (freenect_device_flags)(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));
    int nr_devices = freenect_num_devices (context_);
    if (nr_devices < 1) {
      Close();
      ThrowException(Exception::Error(String::New("No kinect devices present")));
      return; 
    }

    // TODO: select device in constructor

    if (freenect_open_device(context_, &device_, user_device_number) < 0) {
      Close();
      ThrowException(Exception::Error(String::New("Could not open device number\n")));
      return;
    }
  }

  void
  Context::Close() {
    
    if (device_ != NULL) {
      if (freenect_close_device(device_) < 0) {
        ThrowException(Exception::Error(String::New(("Error closing device"))));
        return;
      }

      device_ = NULL;
    }

    if (context_ != NULL) {
      if (freenect_shutdown(context_) < 0) {
        ThrowException(Exception::Error(String::New(("Error shutting down"))));
        return;
      }
        
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

  void
  Initialize(Handle<Object> target) {
    Context::Initialize(target);
  }

}

extern "C" void
init(Handle<Object> target)
{
  kinect::Initialize(target);
}