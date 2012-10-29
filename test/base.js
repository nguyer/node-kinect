var test = require('tap').test;
var kinect = require('..');

test('Initializes and shuts down', function(t) {
  var context = kinect({device: 0});
  context.close();
  t.end();
});

test('Fails on device not present', function(t) {
  t.throws(function() {
    kinect({device: 1});
  });
  t.end();
});