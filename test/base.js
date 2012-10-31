var Kinect = require('..');
var assert = require('assert');

describe("Initialization", function() {
  var context;

  afterEach(function() {
    if (context) {
      context.close();
      context = null;
    }
  });

  it('Initializes and shuts down', function() {
    context = Kinect();
  });

  it('Fails on device not present', function() {
    assert.throws(function() {
      context = Kinect({device: 100});
    });
  });

  it("Fails to activate unknown feature", function() {
    context = Kinect();
    assert.throws(function() {
      context.activate('yabayaba');
    });
  });
});

