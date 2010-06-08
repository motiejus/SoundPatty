#!/bin/bash
echo $@ | mail -s "Found pattern, channel: $1" motiejus@jocom.lt
