#!/bin/bash

COMMON_NAME="/difs"
VIDEO_URL="http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"

VIDEO_NAME=`echo $VIDEO_URL | rev | cut -d "/" -f1  | rev`

echo "Upload $VIDEO_NAME to DIFS"

if [ ! -f "./$VIDEO_NAME" ];then
	echo "Download $VIDOE_NAME"
	wget $VIDEO_URL
fi

echo "Uploading $VIDEO_NAME to DIFS"
ndnputfile $COMMON_NAME /video/$VIDEO_NAME $VIDEO_NAME
echo "Done"