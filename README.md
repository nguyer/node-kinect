# node-kinect

Kinect in Node.js.

# Install

* Install libusb from http://www.libusb.org/
* Install libfreenect from https://github.com/OpenKinect/libfreenect

Then:

```sh
$ git clone 

# Test

Plug your kinect and do:

```bash
$ npm test
```

# Use

## Create a context

```js
var kinect = require('kinect');

var context = kinect();
```

Accepts options like this:

```js
var kinect = require('kinect');

var options = {
  device: 2
};

var context = kinect(options);
```


Options:

* device: integer for device number. Default is 0

## LED

```js
var kinect = require('kinect');

var context = kinect();
context.led("red");
```

Accepts these strings as sole argument:

* "off"
* "green"
* "red"
* "yellow"
* "blink green"
* "blink red yellow"