#!/bin/bash
#
# Fetch latest images.
#
echo "latest_512_0193.jpg"
curl https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0193.jpg --output "AIA 193 Å.jpg"
echo "latest_512_211193171.jpg"
curl https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_211193171.jpg --output "AIA 211 Å, 193 Å, 171 Å.jpg"
curl https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_HMIB.jpg --output "HMI Magnetogram.jpg"
curl https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_HMIIC.jpg --output "HMI Intensitygram.jpg"
