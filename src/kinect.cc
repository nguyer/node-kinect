#define BUILDING_NODE_EXTENSION

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include "uv.h"

#include "libfreenect.h"

#include <stdio.h>
#include <stdexcept>
#include <string>
//#include <iostream>

using namespace node;
using namespace v8;

namespace kinect {

  static Persistent<String> depthCallbackSymbol;
  static Persistent<String> videoCallbackSymbol;

  class Context : ObjectWrap {
    public:
      static void            Initialize (v8::Handle<v8::Object> target);
      virtual                ~Context   ();
      void                   DepthCallback    ();
      void                   VideoCallback    ();
      v8::Local<v8::Object>  videoBuffer_;
      v8::Local<v8::Object>  depthBuffer_;
      bool                   running_;
      freenect_context*      context_;
      uv_async_t             uv_async_video_callback_;
      uv_async_t             uv_async_depth_callback_;

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

      void                  SetVideoCallback ();
      static Handle<Value>  SetVideoCallback (const Arguments &args);      

      void                  InitProcessEventThread();

      bool                      depthCallback_;
      bool                      videoCallback_;
      freenect_device*          device_;
      uv_thread_t               event_thread_;

  };

  Context *
  Context::GetContext(const Arguments &args) {
    return ObjectWrap::Unwrap<Context>(args.This());
  }

  /********* Video ****************/

  static void
  async_video_callback(uv_async_t *handle, int notUsed) {
    Context * context = (Context *) handle->data;
    assert(context != NULL);
    context->VideoCallback();
  }

  static void
  video_callback(freenect_device *dev, void *video, uint32_t timestamp) {
    Context* context = (Context *) freenect_get_user(dev);
    assert(context != NULL);
    context->uv_async_video_callback_.data = (void *) context;
    uv_async_send(&context->uv_async_video_callback_);
    
  }

  void
  Context::VideoCallback() {
    if (videoCallbackSymbol.IsEmpty()) {
      videoCallbackSymbol = NODE_PSYMBOL("videoCallback");
    }
    Local<Value> callback_v =handle_->Get(videoCallbackSymbol);
    if (!callback_v->IsFunction()) {
      ThrowException(Exception::Error(String::New("VideoCallback should be a function")));
    }
    Local<Function> callback = Local<Function>::Cast(callback_v);
    Local<Value> argv[1] = { videoBuffer_};
    callback->Call(handle_, 1, argv);
  }

  void
  Context::SetVideoCallback() {
    node::Buffer *slowBuffer;
    void * bufferPtr;
    freenect_frame_mode mode;
    
    if (!videoCallback_) {

      printf("Setting the Video callback...\n");
      freenect_set_video_callback(device_, video_callback);

      videoCallback_ = true;
      mode = freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB);
      assert(mode.is_valid);
      if (freenect_set_video_mode(device_, mode) != 0) {
        ThrowException(Exception::Error(String::New("Error setting Video mode")));
        return;
      };

      // Allocate buffer

      int length = mode.bytes;
      printf("Allocating Video buffer...\n");
      slowBuffer = node::Buffer::New(length);
      v8::Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
      v8::Local<v8::Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
      v8::Handle<v8::Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(length), v8::Integer::New(0) };
      videoBuffer_ = bufferConstructor->NewInstance(3, constructorArgs);
      bufferPtr = node::Buffer::Data(slowBuffer);

      if (freenect_set_video_buffer(device_, bufferPtr) != 0) {
        ThrowException(Exception::Error(String::New("Error setting Video buffer")));
        return;
      };
    }

    if (freenect_set_depth_mode(device_, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT)) != 0) {
      ThrowException(Exception::Error(String::New("Error setting depth mode")));
      return;
    };

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
  Context::SetVideoCallback(const Arguments& args) {
    
    GetContext(args)->SetVideoCallback();

    return Undefined();
  }


  /********* Depth ****************/

  static void
  async_depth_callback(uv_async_t *handle, int notUsed) {
    Context * context = (Context *) handle->data;
    assert(context != NULL);
    context->DepthCallback();
  }

  static void
  depth_callback(freenect_device *dev, void *depth, uint32_t timestamp) {
    Context* context = (Context *) freenect_get_user(dev);
    context->uv_async_depth_callback_.data = (void *) context;
    uv_async_send(&context->uv_async_depth_callback_);
  }

  void
  Context::DepthCallback() {
    if (depthCallbackSymbol.IsEmpty()) {
      depthCallbackSymbol = NODE_PSYMBOL("depthCallback");
    }
    Local<Value> callback_v =handle_->Get(depthCallbackSymbol);
    if (!callback_v->IsFunction()) {
      ThrowException(Exception::Error(String::New("depthCallback should be a function")));
    }
    Local<Function> callback = Local<Function>::Cast(callback_v);
    Local<Value> argv[1] = { depthBuffer_ };
    callback->Call(handle_, 1, argv);
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
    NODE_SET_PROTOTYPE_METHOD(t, "setVideoCallback", SetVideoCallback);

    target->Set(String::NewSymbol("Context"), t->GetFunction());
  }

  void
  process_event_thread(void *arg) {
    Context * context = (Context *) arg;
    while(context->running_) {
      freenect_process_events(context->context_);
    }
  }

  void
  Context::InitProcessEventThread() {
    uv_loop_t *loop = uv_default_loop();
    uv_async_init(loop, &uv_async_video_callback_, async_video_callback);
    uv_async_init(loop, &uv_async_depth_callback_, async_depth_callback);
    uv_thread_create(&event_thread_, process_event_thread, this);
  }

  Context::Context(int user_device_number) : ObjectWrap() {
    context_         = NULL;
    device_          = NULL;
    depthCallback_   = false;
    videoCallback_   = false;
    running_         = true;

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

    InitProcessEventThread();
  }

  void
  Context::Close() {

    running_ = false;
    
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