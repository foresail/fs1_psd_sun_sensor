#!/usr/bin/env python3
"""
    Run a calibration measurement
"""

import time, datetime
import os, csv

from thor import ThorRotator
from pss import PSSSunSensor, Calibration
import numpy as np
import matplotlib.pyplot as plt

import argparse

parser = argparse.ArgumentParser(description="PSS measurement tool")
auto_int = lambda x: int(x,0)
parser.add_argument('--addr', '-a', type=auto_int, default=0x4A, help="Sensor I2C address")
parser.add_argument('--device', '-D', default="/dev/ttyUSB0", help="Stepper motor Serial device")
parser.add_argument('--constant', '-c', action="store_true", help="Only do constant measurement at 0 deg")
parser.add_argument('--degree', '-d', type=int, default=80, help="Degree measurement max")

parser.add_argument('-w', '--write', dest="save_path",
                    default="meas",
                    type=str, 
                    help="save files to folder")

args = parser.parse_args()

info = "_pss_%x" % args.addr
if args.constant:
    info += "_constant"
fname = datetime.datetime.now().isoformat("_") + info

angles = []
pointsx = []
pointsy = []
intensity = []

# Start writing to CSV file
csv_fname = f"{args.save_path}/{fname}.csv"
if not os.path.exists(args.save_path):
    os.makedirs(args.save_path)

print(f"Outputting to {csv_fname!r}")
with open(csv_fname, 'w') as csvfile: 
    # creating a csv writer object 
    csvwriter = csv.writer(csvfile) 

    pss = PSSSunSensor(args.addr)
    pss.set_calibration(Calibration(0, 0, 670, 1, 639))
    thor = ThorRotator(device=args.device)

    # Read the current calibration and write it to measurement file
    calib = pss.get_calibration()
    print(calib)
    
    csvwriter.writerow(["Calibaration:"])
    csvwriter.writerow(["Offset X", "Offset Y", "Height", "Samples", "Temp Offset"])
    csvwriter.writerow([calib[0], calib[1], calib[2], calib[3], calib[4]])

    fields = ['Angle', 'Position X', 'Position Y', 'Intensity']
    # writing header 
    csvwriter.writerow(fields) 
    if args.constant:
        print("Moving platform to 0 deg. Please wait.")
        angle = 0
        # Rotate
        thor.move_absolute(1, angle * thor.EncCnt)
        time.sleep(0.1)
        
        for _ in range(250):
            # Measure
            while True:
                try:
                    pos = pss.get_point()
                except:
                    continue
                break
                
            print(f"Angle: {angle:>5} deg, X: {pos.x:<5}, Y: {pos.y:<5}, Intensity: {pos.intensity:<5}")
            # write measurement
            csvwriter.writerow([angle, pos.x, pos.y, pos.intensity])

            # Store for plottin
            angles.append(angle)
            pointsx.append(pos.x)
            pointsy.append(pos.y)
            intensity.append(pos.intensity)
            time.sleep(0.1)
    else:
        print("Moving platform to", -args.degree, "deg. Please wait.")
        for angle in np.arange(-args.degree, args.degree+1, 1):

            # Rotate
            thor.move_absolute(1, angle * thor.EncCnt)
            for _ in range(10):
                time.sleep(0.1)

                # Measure
                while True:
                    try:
                        pos = pss.get_point()
                    except:
                        continue
                    break
                
                print(f"Angle: {angle:>5} deg, X: {pos.x:<5}, Y: {pos.y:<5}, Intensity: {pos.intensity:<5}")
                # write measurement
                csvwriter.writerow([angle, pos.x, pos.y, pos.intensity])

                # Store for plottin
                angles.append(angle)
                pointsx.append(pos.x)
                pointsy.append(pos.y)
                intensity.append(pos.intensity)

#np.savez_compressed(f"{args.save_path}/{fname}", angles=angles, pointsx=pointsx, pointsy=pointsy, intensity=intensity)

fig, axes = plt.subplots(3)
axes[0].plot(angles, pointsx, marker = 'o')
axes[0].set_ylabel('Position X')
axes[0].set_xlabel('Angle')

axes[0].set_ylim([-1024, 1024])

axes[1].plot(angles, pointsy, marker = 'o')
axes[1].set_ylabel('Position Y')
axes[1].set_xlabel('Angle')

axes[1].set_ylim([-1024, 1024])

axes[2].plot(angles, intensity, marker = 'o')
axes[2].set_ylabel('Intensity')
axes[2].set_xlabel('Angle')

axes[2].set_ylim([0, 1024])

fig.suptitle("PSS %x" % args.addr)

plt.show()

#thor.move_absolute(1, -50 * thor.EncCnt)