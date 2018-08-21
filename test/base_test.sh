#!/bin/bash


g++ -O3 -DNDEBUG -DCALC_BATCH base_test.cpp -o base_test -I../distribution -I../lib/extras -L../distribution -lBoascript 

