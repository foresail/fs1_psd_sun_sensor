#!/usr/bin/env python3
import sys
import csv
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import minimize


def calc_calib(name, angles, x_points, y_points, x_intensities, y_intensities):

    def fit_position(x):
        x_calculated = x[2] * np.tan(np.radians(angles)) + x[0]
        y_calculated = x[2] * np.tan(np.radians(angles)) + x[1]

        return sum(( x_intensities * (x_points - x_calculated) )**2.0)  + \
               sum(( y_intensities * (y_points - y_calculated) )**2.0)

    def fit_angle(x):
        x_calculated = np.degrees(np.arctan((x_points + x[0]) / x[2]))
        y_calculated = np.degrees(np.arctan((y_points + x[1]) / x[2]))

        return sum(( x_intensities * (angles - x_calculated) )**2.0)  + \
               sum(( y_intensities * (angles - y_calculated) )**2.0)


    res = minimize(fit_angle,
        x0=[0, 0, 700],
        method='Nelder-Mead',
        options={'xatol': 1e-8, 'maxfev': 2000, 'disp': True},
        bounds=(
            (-200, 200),
            (-200, 200),
            (500, 1000)
        ),
    )
    offset_x, offset_y, height = res.x
    print(res)
    print()

    #offset_x, offset_y, height = 0,-200,1000

    print(f"{offset_x=}")
    print(f"{offset_y=}")
    print(f"{height=}")
    print("\n")

    x_calculated = height * np.tan(np.radians(angles)) - offset_x
    y_calculated = height * np.tan(np.radians(angles)) - offset_y

    # X measured points and calculated points
    fig, axes = plt.subplots(3, 2, num=name, figsize=(16,9))
    axes[0,0].scatter(angles, x_points, marker = 'o')
    axes[0,0].plot(angles, x_calculated, "r")
    axes[0,0].set_ylabel('Position X')
    axes[0,0].set_xlabel('Angle [deg]')
    axes[0,0].set_ylim([-1024, 1024])
    axes[0,0].set_title("X-axis")

    # Y measured points and calculated points
    axes[0,1].scatter(angles, y_points, marker = 'o')
    axes[0,1].plot(angles, y_calculated, "r")
    axes[0,1].set_ylabel('Position Y')
    axes[0,1].set_xlabel('Angle [deg]')
    axes[0,1].set_ylim([-1024, 1024])
    axes[0,1].set_title("Y-axis")

    # X error graph
    calculated_angle = np.degrees(np.arctan((x_points + offset_x) / height))
    axes[1,0].scatter(angles, angles - calculated_angle, marker = 'o')
    axes[1,0].set_ylabel('X Angle error')
    axes[1,0].set_xlabel('Angle [deg]')
    axes[1,0].set_ylim([-10, 10])
    axes[1,0].grid(True)

    # Y error graph
    calculated_angle = np.degrees(np.arctan((y_points + offset_y) / height))
    axes[1,1].scatter(angles, angles - calculated_angle, marker = 'o')
    axes[1,1].set_ylabel('Y Angle error')
    axes[1,1].set_xlabel('Angle [deg]')
    axes[1,1].set_ylim([-10, 10])
    axes[1,1].grid(True)

    # X intensity
    axes[2,0].plot(angles, x_intensities, marker = 'o')
    axes[2,0].set_ylabel('X Intensity')
    axes[2,0].set_xlabel('Angle [deg]')
    axes[2,0].set_ylim([0, 1024])

    # Y intensity
    axes[2,1].plot(angles, y_intensities, marker = 'o')
    axes[2,1].set_ylabel('Y Intensity')
    axes[2,1].set_xlabel('Angle [deg]')
    axes[2,1].set_ylim([0, 1024])

    plt.savefig(name + ".png", dpi=300)
    plt.show()


def read_sunsensor_csv(fname):
    angles = []
    pointsx = []
    pointsy = []
    intensity = []

    with open(fname, mode ='r') as file:
        # reading the CSV file
        csvFile = csv.reader(file)
        next(csvFile)
        next(csvFile)
        next(csvFile)
        next(csvFile)
        # displaying the contents of the CSV file
        for lines in csvFile:
            angles.append(int(lines[0]))
            pointsx.append(int(lines[1]))
            pointsy.append(int(lines[2]))
            intensity.append(int(lines[3]))

    return np.array(angles), np.array(pointsx), np.array(pointsy), np.array(intensity)


if __name__ == "__main__":
    name = sys.argv[1]
    angles, points_xx, points_xy, intensity_x = read_sunsensor_csv(name)
    angles, points_yx, points_yy, intensity_y = read_sunsensor_csv(name.replace("X_calib", "Y_calib"))

    # flipping the y data so that fitting is easier
    #points_xx = points_xx[::-1]
    #points_xy = points_xy[::-1]
    #intensity_x = intensity_x[::-1]

    points_yx = points_yx[::-1]
    points_yy = points_yy[::-1]
    intensity_y = intensity_y[::-1]

    calc_calib(name, angles, points_xx, points_yy, intensity_x, intensity_y)

