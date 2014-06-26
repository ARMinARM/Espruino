/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * This file is designed to be parsed during the build process
 *
 * JavaScript Serial Port Functions
 * ----------------------------------------------------------------------------
 */
#include "jswrap_serial.h"
#include "jsdevices.h"
#include "jsinteractive.h"

/*JSON{ "type":"class",
        "class" : "Serial",
        "description" : ["This class allows use of the built-in USARTs",
                         "Methods may be called on the USB, Serial1, Serial2, Serial3, Serial4, Serial5 and Serial6 objects. While different processors provide different numbers of USARTs, you can always rely on at least Serial1 and Serial2" ]
}*/
/*JSON{ "type":"event", "class" : "Serial", "name" : "data",
        "description" : ["The 'data' event is called when data is received. If a handler is defined with `X.on('data', function(data) { ... })` then it will be called, otherwise data will be stored in an internal buffer, where it can be retrieved with `X.read()`" ],
        "params" : [ [ "data", "JsVar", "A string containing one or more characters of received data"] ]
}*/

/*JSON{ "type":"object", "name":"USB", "instanceof" : "Serial",
        "description" : ["The USB Serial port" ],
        "#if" : "defined(USB)"
}*/
/*JSON{ "type":"object", "name":"Serial1", "instanceof" : "Serial",
        "description" : ["The first Serial (USART) port" ],
        "#if" : "USARTS>=1"
}*/
/*JSON{ "type":"object", "name":"Serial2", "instanceof" : "Serial",
        "description" : ["The second Serial (USART) port" ],
        "#if" : "USARTS>=2"
}*/
/*JSON{ "type":"object", "name":"Serial3", "instanceof" : "Serial",
        "description" : ["The third Serial (USART) port" ],
        "#if" : "USARTS>=3"
}*/
/*JSON{ "type":"object", "name":"Serial4", "instanceof" : "Serial",
        "description" : ["The fourth Serial (USART) port" ],
        "#if" : "USARTS>=4"
}*/
/*JSON{ "type":"object", "name":"Serial5", "instanceof" : "Serial",
        "description" : ["The fifth Serial (USART) port" ],
        "#if" : "USARTS>=5"
}*/
/*JSON{ "type":"object", "name":"Serial6", "instanceof" : "Serial",
        "description" : ["The sixth Serial (USART) port" ],
        "#if" : "USARTS>=6"
}*/

/*JSON{ "type":"object", "name":"LoopbackA", "instanceof" : "Serial",
        "description" : [ "A loopback serial device. Data sent to LoopbackA comes out of LoopbackB and vice versa" ]
}*/
/*JSON{ "type":"object", "name":"LoopbackB", "instanceof" : "Serial",
        "description" : ["A loopback serial device. Data sent to LoopbackA comes out of LoopbackB and vice versa" ]
}*/



/*JSON{ "type":"method", "class": "Serial", "name" : "setConsole",
         "description" : "Set this Serial port as the port for the console",
         "generate_full" : "jsiSetConsoleDevice(jsiGetDeviceFromClass(parent))"
}*/

/*JSON{ "type":"method", "class": "Serial", "name" : "setup",
         "description" : ["Setup this Serial port with the given baud rate and options.",
                          "If not specified in options, the default pins are used (usually the lowest numbered pins on the lowest port that supports this peripheral)"],
         "generate" : "jswrap_serial_setup",
         "params" : [ [ "baudrate", "JsVar", "The baud rate - the default is 9600"],
                      [ "options", "JsVar", ["An optional structure containing extra information on initialising the serial port.",
                                             "```{rx:pin,tx:pin,bytesize:8,parity:null/'none'/'o'/'odd'/'e'/'even',stopbits:1}```",
                                             "You can find out which pins to use by looking at [your board's reference page](#boards) and searching for pins with the `UART`/`USART` markers.",
                                             "Note that even after changing the RX and TX pins, if you have called setup before then the previous RX and TX pins will still be connected to the Serial port as well - until you set them to something else using digitalWrite" ] ] ]
}*/
void jswrap_serial_setup(JsVar *parent, JsVar *baud, JsVar *options) {
  IOEventFlags device = jsiGetDeviceFromClass(parent);
  if (!DEVICE_IS_USART(device)) return;

  JshUSARTInfo inf;
  jshUSARTInitInfo(&inf);

  if (!jsvIsUndefined(baud)) {
    int b = (int)jsvGetInteger(baud);
    if (b<=100 || b > 10000000)
      jsExceptionHere(JSET_ERROR, "Invalid baud rate specified");
    else
      inf.baudRate = b;
  }


  if (jsvIsObject(options)) {
    inf.pinRX = jshGetPinFromVarAndUnLock(jsvObjectGetChild(options, "rx", 0));
    inf.pinTX = jshGetPinFromVarAndUnLock(jsvObjectGetChild(options, "tx", 0));    

    JsVar *v;
    v = jsvObjectGetChild(options, "bytesize", 0);
    if (jsvIsInt(v)) 
      inf.bytesize = (unsigned char)jsvGetInteger(v);
    jsvUnLock(v);
    
    inf.parity = 0;
    v = jsvObjectGetChild(options, "parity", 0);
    if(jsvIsString(v)) {
      if(jsvIsStringEqual(v, "o") || jsvIsStringEqual(v, "odd"))
        inf.parity = 1;
      else if(jsvIsStringEqual(v, "e") || jsvIsStringEqual(v, "even"))
        inf.parity = 2;
    } else if(jsvIsInt(v)) {
      inf.parity = (unsigned char)jsvGetInteger(v);
    }
    jsvUnLock(v);
    if (inf.parity>2) {
      jsExceptionHere(JSET_ERROR, "Invalid parity %d", inf.parity);
      return;
    }

    v = jsvObjectGetChild(options, "stopbits", 0);
    if (jsvIsInt(v)) 
      inf.stopbits = (unsigned char)jsvGetInteger(v);
    jsvUnLock(v);
  }

  jshUSARTSetup(device, &inf);
  // Set baud rate in object, so we can initialise it on startup
  if (inf.baudRate != DEFAULT_BAUD_RATE) {
    jsvUnLock(jsvObjectSetChild(parent, USART_BAUDRATE_NAME, jsvNewFromInteger(inf.baudRate)));
  } else
    jsvRemoveNamedChild(parent, USART_BAUDRATE_NAME);
  // Do the same for options
  if (options)
    jsvUnLock(jsvSetNamedChild(parent, options, DEVICE_OPTIONS_NAME));
  else
    jsvRemoveNamedChild(parent, DEVICE_OPTIONS_NAME);
}

/*JSON{ "type":"method", "class": "Serial", "name" : "print",
         "description" : "Print a string to the serial port - without a line feed",
         "generate" : "jswrap_serial_print",
         "params" : [ [ "string", "JsVar", "A String to print"] ]
}*/
/*JSON{  "type":"method", "class": "Serial", "name" : "println",
         "description" : "Print a line to the serial port (newline character sent are '\r\n')",
         "generate" : "jswrap_serial_println",
         "params" : [ [ "string", "JsVar", "A String to print"] ]
}*/
void _jswrap_serial_print(JsVar *parent, JsVar *str, bool newLine) {
  NOT_USED(parent);
  IOEventFlags device = jsiGetDeviceFromClass(parent);
  if (!DEVICE_IS_USART(device)) return;

  str = jsvAsString(str, false);
  jsiTransmitStringVar(device,str);
  jsvUnLock(str);
  if (newLine) {
    jshTransmit(device, (unsigned char)'\r');
    jshTransmit(device, (unsigned char)'\n');
  }
}
void jswrap_serial_print(JsVar *parent, JsVar *str) {
  _jswrap_serial_print(parent, str, false);
}
void jswrap_serial_println(JsVar *parent,  JsVar *str) {
  _jswrap_serial_print(parent, str, true);
}
/*JSON{ "type":"method", "class": "Serial", "name" : "write",
         "description" : "Write a character or array of characters to the serial port - without a line feed",
         "generate" : "jswrap_serial_write",
         "params" : [ [ "data", "JsVarArray", "One or more items to write. May be ints, strings, arrays, or objects of the form `{data: ..., count:#}`."] ]
}*/
static void jswrap_serial_write_cb(int data, void *userData) {
  IOEventFlags device = *(IOEventFlags*)userData;
  jshTransmit(device, (unsigned char)data);
}
void jswrap_serial_write(JsVar *parent, JsVar *args) {
  NOT_USED(parent);
  IOEventFlags device = jsiGetDeviceFromClass(parent);
  if (!DEVICE_IS_USART(device)) return;

  jsvIterateCallback(args, jswrap_serial_write_cb, (void*)&device);
}

/*JSON{ "type":"method", "class": "Serial", "name" : "onData",
         "description" : ["Serial.onData(func) has now been replaced with the event Serial.on(`data`, func)"],
         "generate" : "jswrap_serial_onData",
         "params" : [ [ "function", "JsVar", ""] ]
}*/
void jswrap_serial_onData(JsVar *parent, JsVar *func) {
  NOT_USED(parent);
  NOT_USED(func);
  jsWarn("Serial.onData(func) has now been replaced with Serial.on(`data`, func).");
}

/*JSON{ "type":"method", "class": "Serial", "name" : "available",
         "description" : ["Return how many bytes are available to read. If there is already a listener for data, this will always return 0."],
         "generate" : "jswrap_stream_available",
         "return" : ["int", "How many bytes are available"]
}*/

/*JSON{ "type":"method", "class": "Serial", "name" : "read",
         "description" : ["Return a string containing characters that have been received"],
         "generate" : "jswrap_stream_read",
         "params" : [ [ "chars", "int", "The number of characters to read, or undefined/0 for all available"] ],
         "return" : ["JsVar", "A string containing the required bytes."]
}*/

/*JSON{  "type" : "method", "class" : "Serial", "name" : "pipe", "ifndef" : "SAVE_ON_FLASH",
         "generate" : "jswrap_pipe",
         "description" : [ "Pipe this USART to a stream (an object with a 'write' method)"],
         "params" : [ ["destination", "JsVar", "The destination file/stream that will receive content from the source."],
                      ["options", "JsVar", [ "An optional object `{ chunkSize : int=32, end : bool=true, complete : function }`",
                                             "chunkSize : The amount of data to pipe from source to destination at a time",
                                             "complete : a function to call when the pipe activity is complete",
                                             "end : call the 'end' function on the destination when the source is finished"] ] ]
}*/
