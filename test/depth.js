var kinect = require('..');
var assert = require('assert');

describe("After initialized", function() {
  var context = kinect({device: 0});

  describe("Depth", function() {
    it("should allow to pass in a depth callback", function(done) {
      this.timeout(5000);
      context.activate('depth');
      context.once('depth', function(buf, time) {
        console.log('DEPTH cb');
        assert(!! buf);
        assert(buf instanceof Buffer);
        assert.equal(typeof time, 'number');
        done(); 
      });
      console.log('context:', context);
    });
    
  });

});
