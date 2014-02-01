var kinect       = require('./build/Release/kinect.node');
var EventEmitter = require('events').EventEmitter;
var inherits     = require('util').inherits;
var assert       = require('assert');

function Context(context) {
  this._kContext = context;
  this._activated = {};
  EventEmitter.apply(this, arguments);
}
inherits(Context, EventEmitter);

Context.prototype.activate = function(wat) {
  var self = this;
  if (this._activated[wat]) throw new Error('Already had activated ' + wat)
  switch(wat) {
    case "depth":
      process.nextTick(function() {
        self._kContext.setDepthCallback();
      });
      break;

    case "video":
      //var buf = self._videoBuffer = new Buffer(640 * 480 * 3);
      //self._kContext.setVideoBuffer(buf);
      self._kContext.setVideoCallback();
      break;

    default: throw new Error('Cannot activate ' + wat);
  }
  this._activated[wat] = true;
};

Context.prototype.start = Context.prototype.activate;

kinect.Context.prototype.depthCallback = function depthCallback(depthBuffer) {
  this._context.emit('depth', depthBuffer);
};

kinect.Context.prototype.videoCallback = function videoCallback(videoBuffer) {
  this._context.emit('video', videoBuffer);
};

Context.prototype.led = function lef(color) {
  this._kContext.led(color);
};

Context.prototype.tilt = function tilt(angle) {
  this._kContext.tilt(angle);
};

Context.prototype.close = function close() {
  this._kContext.close();
};

Context.prototype.resume = function() {
  this._kContext.resume();
};

Context.prototype.pause = function() {
  this._kContext.pause();
};

module.exports = function(options) {
  if (! options) options = {};
  if (! options.device) options.device = 0;
  var kContext = new kinect.Context(options.device);
  var context = new Context(kContext);
  kContext._context = context;

  return context;
};
