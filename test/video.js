var Kinect = require('..');
var assert = require('assert');

describe("Depth", function() {

  var context;

  beforeEach(function() {
    context = Kinect();
  });

  afterEach(function() {
    context.close();
  });

  it("should allow to pass in a depth callback", function(done) {
    this.timeout(5000);
    context.activate('video');
    context.once('video', function(buf) {
      assert(!! buf);
      assert(buf instanceof Buffer);
      assert(buf.length > 0);
      assert.equal(buf.length, 640 * 480 * 3);
      done(); 
    });
  });
});