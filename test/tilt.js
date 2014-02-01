var Kinect = require('..');
var assert = require('assert');

describe("Tilt", function() {

  var context;

  beforeEach(function() {
    context = Kinect();
  });

  afterEach(function() {
    context.close();
  });

  it("should throw when no angle is specified", function() {
    assert.throws(function() {
      context.tilt();
    });
  });

  [-15,
   -12,
   -9,
   -3,
   0,
   3,
   8,
   12,
   0
  ].forEach(function(angle) {
    it("to " + angle, function(done) {
      this.timeout(5000);
      context.tilt(angle);
      console.log('Should be ' + angle + '...');
      setTimeout(done, 2000);
    });
  });

});
