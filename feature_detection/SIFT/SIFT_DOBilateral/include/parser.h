#ifndef __CMD_PARSER__
#define __CMD_PARSER__

#include <iostream>
#include <string>
/*
    For future versions this will accept proper command line
    arguements and be able to produce a help section
*/

const std::string keys =
    "{help h usage ? |      | print this message   }"
    "{@image1        |      | image1 for compare   }"
    "{@image2        |      | image2 for compare   }"
    "{@repeat        |1     | number               }"
    "{path           |.     | path to file         }"
    "{fps            | -1.0 | fps for output video }"
    "{N count        |100   | count of objects     }"
    "{ts timestamp   |      | use time stamp       }"
    ;
#endif
