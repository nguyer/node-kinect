#define BUILDING_NODE_EXTENSION

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "libfreenect.h"

#include <stdio.h>
#include <stdexcept>

#include <string>
//#include <iostream>

using namespace node;
using namespace v8;

namespace kinect {

  static Persistent<String> depthCallbackSymbol;

  class Context : ObjectWrap {
    public:
      static void   Initialize (v8::Handle<v8::Object> target);
      virtual       ~Context   ();
      void          DepthCallback    (void *depth, uint32_t timestamp);

    private:
      Context(int user_device_number);
      static Handle<Value>  New              (const Arguments& args);
      static Context*       GetContext       (const Arguments &args);
      
      void                  Close            ();
      static Handle<Value>  Close            (const Arguments &args);
      
      void                  Led              (const std::string option);
      static Handle<Value>  Led              (const Arguments &args);

      void                  SetDepthCallback ();
      static Handle<Value>  SetDepthCallback (const Arguments &args);      

      bool                      depthCallback_;
      v8::Local<v8::Object>     depthBuffer_;
      freenect_context*         context_;
      freenect_device*          device_;

  };

  Context *
  Context::GetContext(const Arguments &args) {
    return ObjectWrap::Unwrap<Context>(args.This());
  }


  /********* Depth ****************/

  static void
  depth_callback(freenect_device *dev, void *depth, uint32_t timestamp) {
    Context* context = (Context *) freenect_get_user(dev);
    assert(context != NULL);
    context->DepthCallback(depth, timestamp);
  }

  void
  Context::DepthCallback(void *depth, uint32_t timestamp) {
    if (depthCallbackSymbol.IsEmpty()) {
      depthCallbackSymbol = NODE_PSYMBOL("depthCallback");
    }
    Local<Value> callback_v =handle_->Get(depthCallbackSymbol);
    if (!callback_v->IsFunction()) {
      ThrowException(Exception::Error(String::New("depthCallback should be a function")));
    }
    Local<Function> callback = Local<Function>::Cast(callback_v);
    Local<Value> argv[2] = { depthBuffer_, Integer::New(timestamp) };
    callback->Call(handle_, 2, argv);
  }

  void
  Context::SetDepthCallback() {
    node::Buffer *slowBuffer;
    void * bufferPtr;
    freenect_frame_mode mode;
    
    if (!depthCallback_) {
      depthCallback_ = true;
      printf("Setting the depth callback...\n");
      freenect_set_depth_callback(device_, depth_callback);
      mode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT);
      assert(mode.is_valid);
      if (freenect_set_depth_mode(device_, mode) != 0) {
        ThrowException(Exception::Error(String::New("Error setting depth mode")));
        return;
      };

      // Allocate buffer

      int length = mode.bytes;
      printf("Allocating depth buffer...\n");
      slowBuffer = node::Buffer::New(length);
      v8::Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
      v8::Local<v8::Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
      v8::Handle<v8::Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(length), v8::Integer::New(0) };
      depthBuffer_ = bufferConstructor->NewInstance(3, constructorArgs);
      bufferPtr = node::Buffer::Data(slowBuffer);

      if (freenect_set_depth_buffer(device_, bufferPtr) != 0) {
        ThrowException(Exception::Error(String::New("Error setting depth buffer")));
        return;
      };

    }

    if (freenect_set_video_mode(device_, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB)) != 0) {
      ThrowException(Exception::Error(String::New("Error setting video mode")));
      return;
    }

    if (freenect_start_depth(device_) != 0) {
      ThrowException(Exception::Error(String::New("Error starting depth")));
      return;
    }

    if (freenect_start_video(device_) != 0) {
      ThrowException(Exception::Error(String::New("Error starting video")));
      return;
    }
  }

  Handle<Value>
  Context::SetDepthCallback(const Arguments& args) {
    
    GetContext(args)->SetDepthCallback();

    return Undefined();
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
    NODE_SET_PROTOTYPE_METHOD(t, "setDepthCallback", SetDepthCallback);

    target->Set(String::NewSymbol("Context"), t->GetFunction());
  }

  Context::Context(int user_device_number) : ObjectWrap() {
    context_         = NULL;
    device_          = NULL;
    depthCallback_   = false;

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

    freenect_set_user(device_, this);

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