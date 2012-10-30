var kinect = require('..');
var assert = require('assert');

describe("Initialization", function() {
  it('Initializes and shuts down', function() {
    var context = kinect({device: 0});
    context.close();
  });

  it('Fails on device not present', function() {
    assert.throws(function() {
      kinect({device: 1});
    });
  });

  it("Fails to activate unknown feature", function() {
    var context = kinect();
    assert.throws(function() {
      context.activate('yabayaba');
    });
  });
});

