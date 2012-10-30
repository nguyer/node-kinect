var kinect = require('..');
var assert = require('assert');

describe("Initialization", function() {
  it('Initializes and shuts down', function(done) {
    var context = kinect({device: 0});
    context.close();
    done();
  });

  it('Fails on device not present', function(done) {
    assert.throws(function() {
      kinect({device: 1});
    });
    done();
  });
});

