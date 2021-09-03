#!/bin/bash

COMMON_NAME="/difs"
VIDEO_NAME="BigBuckBunny.mp4"

ndngetfile $COMMON_NAME /video/$VIDEO_NAME 2>/dev/null | vlc -