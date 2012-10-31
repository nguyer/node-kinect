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
    context.activate('depth');
    context.once('depth', function(buf) {
      assert(buf instanceof Buffer);
      assert.equal(buf.length, 640 * 480 * 2);
      done(); 
    });
  });
  
});
