#!/usr/bin/env python3
"""
    Control PSS Sun Sensor over FTDI 232H USB interface cable.
"""

import sys
import time
import struct
from typing import List, NamedTuple, Tuple, Union

from pyftdi.i2c import I2cController, I2cPort, I2cNackError


__all__ = [
    "RawMeasurement",
    "PointMeasurement",
    "VectorMeasurement"
    "AngleMeasurement",
    "Calibration",
]


# Command codes:
PSS_CMD_STATUS          = 0x01
PSS_CMD_GET_RAW         = 0x03
PSS_CMD_GET_POINT       = 0x04
PSS_CMD_GET_VECTOR      = 0x05
PSS_CMD_GET_ANGLES      = 0x06
PSS_CMD_GET_ALL         = 0x07
PSS_CMD_GET_TEMPERATURE = 0x08
PSS_CMD_SET_CALIBRATION = 0x10
PSS_CMD_GET_CALIBRATION = 0x11
PSS_CMD_SET_I2C_ADDRESS = 0xE8

# Response codes:
PSS_RSP_OK              = 0xF0
PSS_RSP_SLEEP           = 0xF1
PSS_RSP_RAW             = 0xF3
PSS_RSP_POINT           = 0xF4
PSS_RSP_VECTOR          = 0xF5
PSS_RSP_ANGLES          = 0xF6
PSS_RSP_ALL             = 0xF7
PSS_RSP_TEMPERATURE     = 0xF8
PSS_RSP_CALIBRATION     = 0xFA
PSS_RSP_UNKNOWN_COMMAND = 0xFD
PSS_RSP_INVALID_PARAM   = 0xFE
PSS_RSP_ERROR           = 0xFF


# Structures for data
class RawMeasurement(NamedTuple):
    """
    A raw PSS diode current measurement.
    Measured current are between 0 and 1024
    """
    x1: int
    x2: int
    y1: int
    y2: int

class PointMeasurement(NamedTuple):
    """
    Calculated light point position measurement
    """
    x: int
    y: int
    intensity: int

class VectorMeasurement(NamedTuple):
    """
    Calculated sun direction vector point position measurement
    """
    x: int
    y: int
    z: int
    intensity: int

class AngleMeasurement(NamedTuple):
    """
    Calculated sun direction as angles.
    """
    rx: float
    ry: float
    intensity: int

class Calibration(NamedTuple):
    """
    PSS Calibration value struct
    """
    offset_x: int
    offset_y: int
    height: int
    samples: int
    temp_offset: int



class PSSSunSensor:
    """
    Class to communicate to single PSS Sun Sensor over I2C.
    """

    def __init__(self, addr: int, i2c: I2cController = None):
        """
        Initialize connection to PSS Sun Sensor
        aka opens FTDI 232H I2C controller and and I2C port for the sensor.

        Args:
            addr: Sensor I2C address
            i2c: FTDI I2C Controller object. If not given a new controller
                object will be initialized with default values.
        """
        if i2c is None:
            i2c = I2cController()
            i2c.configure("ftdi://ftdi:232h/1", frequency=50e3)
        self._i2c = i2c
        self._port = i2c.get_port(addr)


    def get_raw(self) -> RawMeasurement:
        """
        Read raw measurements from the sun sensor.

        Returns:
            A RawMeasurement object
        """

        self._port.write(struct.pack("B", PSS_CMD_GET_RAW))
        time.sleep(0.01)

        rsp = self._port.read(9)
        if rsp[0] != PSS_RSP_RAW:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")
        return RawMeasurement(*struct.unpack("<xHHHH", rsp))


    def get_point(self) -> PointMeasurement:
        """
        Read light point measurement from the sun sensor.

        Returns:
            A PointMeasurement object
        """

        self._port.write(struct.pack("B", PSS_CMD_GET_POINT))
        time.sleep(0.01)

        rsp = self._port.read(7)
        if rsp[0] != PSS_RSP_POINT:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")
        return PointMeasurement(*struct.unpack("<xhhH", rsp))


    def get_vector(self) -> VectorMeasurement:
        """
        Read light point measurement from the sun sensor.

        Returns:
            A VectorMeasurement object
        """

        self._port.write(struct.pack("B", PSS_CMD_GET_VECTOR))
        time.sleep(0.01)

        rsp = self._port.read(9)
        if rsp[0] != PSS_RSP_VECTOR:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")
        return VectorMeasurement(*struct.unpack("<xhhhH", rsp))


    def get_angles(self) -> AngleMeasurement:
        """
        Read angle measuremnt from the sun sensor

        Returns:
            A AngleMeasurement object
        """

        self._port.write(struct.pack("B", PSS_CMD_GET_ANGLES))
        time.sleep(0.02)

        rsp = self._port.read(7)
        if rsp[0] != PSS_RSP_ANGLES:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")
        meas = struct.unpack("<xHHH", rsp)
        return AngleMeasurement(meas[0] / 10, meas[1] / 10, meas[2])


    def get_all(self) -> Tuple[RawMeasurement, PointMeasurement, AngleMeasurement]:
        """
        Read all three measurement structs from the sun sensor

        Returns:
            A tuple containing RawMeasurement, PointMeasurement and AngleMeasurement object
        """

        self._port.write(struct.pack("B", PSS_CMD_GET_ALL))
        time.sleep(0.2)

        rsp = self._port.read(21)
        if rsp[0] != PSS_RSP_ALL:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")

        raw   =   RawMeasurement(*struct.unpack("<HHHH", rsp[1:9]))
        point = PointMeasurement(*struct.unpack("<hhH",  rsp[9:15]))
        angle = AngleMeasurement(*struct.unpack("<hhH",  rsp[15:21]))
        return raw, point, angle



    def get_temperature(self) -> float:
        """
        Read sun sensor's temperature

        Returns:
            Temperature reading in Celcius degrees.
        """

        self._port.write(struct.pack("B", PSS_CMD_GET_TEMPERATURE))

        rsp = self._port.read(3)
        if rsp[0] != PSS_RSP_TEMPERATURE:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")
        return struct.unpack("<xh", rsp)[0] / 10.0


    def set_calibration(self, calib: Union[Calibration, Tuple]) -> None:
        """
        Write sun sensor calibration values

        Args:
            calib: A calibration object a tuple containing the calibration values.

        Example:
            set_calibration(sensor, Calibration(offset_x=0, offset_y=0, height=670, samples=1, temp_offset=650))
        """

        self._port.write(struct.pack("<Bhhhhh", PSS_CMD_SET_CALIBRATION, *calib))
        #time.sleep(0.05)

        rsp = self._port.read(1)
        if rsp[0] != PSS_RSP_OK:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")


    def get_calibration(self) -> Calibration:
        """
        Read sunsensor's temperature sensor.

        Returns:
            A Calibration object
        """

        self._port.write(struct.pack("B", PSS_CMD_GET_CALIBRATION))
        #time.sleep(0.05)

        rsp = self._port.read(11)
        if rsp[0] != PSS_RSP_CALIBRATION:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")
        return Calibration(*struct.unpack("<xhhhhh", rsp))


    def set_lut(self, lut: List[int]) -> None:
        """
        Write the Position to angle look-up-table in the memory.

        Args:
            lut: Look-up-table to be uploaded (a list of 256 integers)
        """
        assert len(lut) == 256

        for i in range(0, 256, 8):

            # Send 8 byte segment of the LUT
            self._port.write(struct.pack("BB16h", PSS_CMD_SET_LUT, i, lut[i:i+8]))

            rsp = self._port.read(1)
            if rsp[0] != PSS_RSP_OK:
                raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")


    def set_i2c_address(self, addr: int) -> None:
        """
        Set sensor I2C address (reboot required)

        Args:
            addr: I2C address (from 0x00 to 0x7F)
        """

        self._port.write(struct.pack("BB", PSS_CMD_SET_I2C_ADDRESS, addr))
        time.sleep(0.01)

        rsp = self._port.read(1)
        if rsp[0] != PSS_RSP_OK:
            raise RuntimeError(f"Sensor responded error 0x{rsp[0]:02x}")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="PSS test tool")
    auto_int = lambda x: int(x,0)
    parser.add_argument('--addr', '-a', type=auto_int, default=0x4A, help="Sensor I2C address")
    parser.add_argument('--rate', '-r', type=float, default=0.2, help="Sampling rate")

    # Sampling
    parser.add_argument('--raw', action='store_true', help='Sample raw data')
    parser.add_argument('--point', action='store_true', help='Sample point data')
    parser.add_argument('--vector', action='store_true', help='Sample vector data')
    parser.add_argument('--angles', action='store_true', help='Sample angle data')
    parser.add_argument('--all', action='store_true', help='Sample all data types')
    parser.add_argument('--temp', action='store_true', help='Sample temperature types')
    parser.add_argument('--scan', action='store_true', help='Scan responsive sensors')

    # Calibration
    parser.add_argument('--calib', action='store_true', help='Get calibration values')
    parser.add_argument('--set_offset', type=int, nargs=3, help='Set position offset and height')
    parser.add_argument('--set_temp', type=int, help='Set temperature offset')
    parser.add_argument('--set_addr', type=auto_int, help='Set sensor I2C address')

    args = parser.parse_args()

    if args.all:
        args.raw = args.point = args.angles = True

    # Scan sensors
    if args.scan:
        i2c = I2cController()
        i2c.configure("ftdi://ftdi:232h/1", frequency=10e3)

        for addr in range(0, 127):
            try:
                print(f"0x{addr:02X}: ", end='')
                sensor = i2c.get_port(addr)
                PSSSunSensor(addr, i2c).get_temperature()
                print("Found!")
            except I2cNackError:
                print("NACK")
            except Exception as e:
                print(e)

        sys.exit(0)


    pss = PSSSunSensor(args.addr)

    # Read calibration
    if args.calib:
        print(pss.get_calibration())

    # Set temperature offset
    if args.set_temp is not None:
        calib = pss.get_calibration()
        print("Old calibration:\n", calib)
        calib = calib._replace(temp_offset=args.set_temp)
        pss.set_calibration(calib)
        print("New calibration:\n", calib)

    # Set sensor I2C address
    if args.set_addr:
        print("Setting I2C address to 0x%02x" % args.set_addr)
        pss.set_i2c_address(args.set_addr)

    # Set calibration sensor offset and height
    if args.set_offset:
        calib = pss.get_calibration()
        print("Old calibration:\n", calib)
        calib = Calibration(args.set_offset[0], args.set_offset[1], args.set_offset[2], \
            calib.samples, calib.temp_offset)
        pss.set_calibration(calib)
        print("New calibration:\n", calib)

    # Infinite sampling loop
    if args.raw or args.point or args.vector or args.angles or args.temp:
        while True:
            if args.raw:
                print("%5d %5d %5d %5d" % pss.get_raw())
            if args.point:
                print("%5d %5d %5d" % pss.get_point())
            if args.vector:
                print("%5d %5d %5d %5d" % pss.get_vector())
            if args.angles:
                print("%5.2f %5.2f %5d" % pss.get_angles())
            if args.temp:
                print("%.1f °C" % pss.get_temperature())

            time.sleep(args.rate)
