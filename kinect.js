var kinect       = require('./build/Release/node-kinect.node');
var EventEmitter = require('events').EventEmitter;
var inherits     = require('util').inherits;

function Context(context) {
  this._kContext = context;
  EventEmitter.apply(this, arguments);
}
inherits(Context, EventEmitter);

Context.prototype.activate = function(wat) {
  var self = this;
  switch(wat) {
    case "depth":
      process.nextTick(function() {
        self._kContext.setDepthCallback();
      });
      break;

    case "video":
      process.nextTick(function() {
        self._kContext.setVideoCallback();
      });
      break;

    default: throw new Error('Cannot activate ' + wat);
  }
};

kinect.Context.prototype.depthCallback = function depthCallback(depthBuffer, time) {
  this._context.emit('depth', depthBuffer, time);
};

kinect.Context.prototype.videoCallback = function videoCallback(videoBuffer, time) {
  this._context.emit('video', videoBuffer, time);
};

Context.prototype.led = function lef(color) {
  this._kContext.led(color);
};

Context.prototype.close = function close() {
  this._kContext.close();
};

module.exports = function(options) {
  if (! options) options = {};
  if (! options.device) options.device = 0;
  var kContext = new kinect.Context(options.device);
  var context = new Context(kContext);
  kContext._context = context;
  
  return context;
};