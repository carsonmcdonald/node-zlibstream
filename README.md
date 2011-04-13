# NAME

node-zlibstream - ZLib buffered streaming for Node.js with dictionary support.

# BUILD/INSTALL

To build you will need to have zlib installed on your machine.

The easy way to install is to use [npm](https://github.com/isaacs/npm):

    npm install zlibstream

To build from source:

    git clone git://github.com/carsonmcdonald/node-zlibstream.git
    cd node-zlibstream
    ./configure
    make

# USAGE

To use without a dictionary:

    var Buffer = require('buffer').Buffer;
    var ZLibStream = require('zlibstream').ZLibStream;
    
    var zstream = new ZLibStream();

    var comp1 = zstream.deflate(new Buffer('First chunk of input data'));
    var out1 = zstream.inflate(comp1);
    console.log(out1.toString());

    var comp2 = zstream.deflate(new Buffer('Second chunk of input data'));
    var out2 = zstream.inflate(comp2);
    console.log(out2.toString());

    zstream.resetDeflate();
    zstream.resetInflate();

To use with a dictionary:

    var Buffer = require('buffer').Buffer;
    var ZLibStream = require('zlibstream').ZLibStream;
    
    var dictionary = new Buffer("chunkinputdataof", 'binary');

    var zstream = new ZLibStream();

    zstream.setInflateDictionary(dictionary);
    zstream.setDeflateDictionary(dictionary);

    var comp1 = zstream.deflate(new Buffer('First chunk of input data. More chunks and chunks of data for input.'));
    console.log(comp1.length);
    var out1 = zstream.inflate(comp1);
    console.log(out1.toString());

    var comp2 = zstream.deflate(new Buffer('Second chunk of input data. Lots more chunks and chunks of data.'));
    console.log(comp2.length);
    var out2 = zstream.inflate(comp2);
    console.log(out2.toString());

    zstream.resetDeflate();
    zstream.resetInflate();

In the dictionary example you should try commenting out the dictionary and notice the difference in the length of the compressed data. The dictionary should be made up of commonly repeated data found in the input.

# LICENSE

node-zlib released with an MIT license
