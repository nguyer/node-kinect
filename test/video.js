var Kinect = require('..');
var assert = require('assert');

describe("Video", function() {

  var context;

  beforeEach(function() {
    context = Kinect();
  });

  afterEach(function() {
    context.close();
  });

  it("should allow to pass in a depth callback", function(done) {
    this.timeout(60000);
    context.start('video');
    context.resume();
    var missingFrames = 1000;

    function handleVideo(buf) {
      if (-- missingFrames == 0) {
        context.removeListener('video', handleVideo);
      }
      if (! (missingFrames % 10))
        process.stdout.write('.');
      assert(buf instanceof Buffer, 'buf is not an instance of Buffer');
      assert(buf.length > 0, 'Buffer length is zero');
      assert.equal(buf.length, 640 * 480 * 3, 'Buffer length is ' + buf.length);

      if (missingFrames == 0) done();
    }
    context.on('video', handleVideo);
  });
});