var Kinect = require('..');
var assert = require('assert');

describe("Led", function() {

  var context;

  beforeEach(function() {
    context = Kinect();
  });

  afterEach(function() {
    context.close();
  });

  it("should throw when no string is specified", function() {
    assert.throws(function() {
      context.led();
    });
  });

  it("should throw when a non-supported string is specified", function() {
    assert.throws(function() {
      context.led('pink');
    });
  });

  ['off',
   'green',
   'red',
   'yellow',
   'blink green',
   'blink red yellow',
   'off'
  ].forEach(function(color) {
    it("gets " + color, function(done) {
      this.timeout(5000);
      context.led(color);
      console.log('Should be ' + color + '...');
      setTimeout(done, 2000);
    });
  });

});