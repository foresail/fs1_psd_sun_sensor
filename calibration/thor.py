#!/usr/bin/env python3
"""
    Thor rotator controller driver.
    Tested on Throw CR1/M-Z7

    Manual: https://www.thorlabs.com/Software/Motion%20Control/APT_Communications_Protocol.pdf
"""

import struct
from typing import NamedTuple, Optional

import serial


#
# List of message IDs
#
MGMSG_HW_DISCONNECT = 0x0002
MGMSG_HW_REQ_INFO = 0x0005
MGMSG_HW_GET_INFO = 0x0006

MGMSG_HW_RESPONSE = 0x0080
MGMSG_HW_RICHRESPONSE = 0x0081

MGMSG_MOD_IDENTIFY = 0x0223

MGMSG_MOD_SET_CHANENABLESTATE = 0x0210
MGMSG_MOD_REQ_CHANENABLESTATE = 0x0211
MGMSG_MOD_GET_CHANENABLESTATE = 0x0212

MGMSG_MOT_SET_POSCOUNTER = 0x0410
MGMSG_MOT_REQ_POSCOUNTER = 0x0411
MGMSG_MOT_GET_POSCOUNTER = 0x0412

MGMSG_MOT_SET_ENCCOUNTER = 0x0409
MGMSG_MOT_REQ_ENCCOUNTER = 0x040A
MGMSG_MOT_GET_ENCCOUNTER = 0x040B

MGMSG_MOT_SET_VELPARAMS = 0x0413
MGMSG_MOT_REQ_VELPARAMS = 0x0414
MGMSG_MOT_GET_VELPARAMS = 0x0415

MGMSG_MOT_SET_JOGPARAMS = 0x0416
MGMSG_MOT_REQ_JOGPARAMS = 0x0417
MGMSG_MOT_GET_JOGPARAMS = 0x0418

MGMSG_MOT_REQ_ADCINPUTS = 0x042B
MGMSG_MOT_GET_ADCINPUTS = 0x042C

MGMSG_MOT_SET_POWERPARAMS = 0x0426
MGMSG_MOT_REQ_POWERPARAMS = 0x0427
MGMSG_MOT_GET_POWERPARAMS = 0x0428

MGMSG_MOT_SET_GENMOVEPARAMS = 0x043A
MGMSG_MOT_REQ_GENMOVEPARAMS = 0x043B
MGMSG_MOT_GET_GENMOVEPARAMS = 0x043C

MGMSG_MOT_SET_MOVERELPARAMS = 0x0445
MGMSG_MOT_REQ_MOVERELPARAMS = 0x0446
MGMSG_MOT_GET_MOVERELPARAMS = 0x0447

MGMSG_MOT_SET_MOVEABSPARAMS = 0x0450
MGMSG_MOT_REQ_MOVEABSPARAMS = 0x0451
MGMSG_MOT_GET_MOVEABSPARAMS = 0x0452

MGMSG_MOT_SET_HOMEPARAMS = 0x0440
MGMSG_MOT_REQ_HOMEPARAMS = 0x0441
MGMSG_MOT_GET_HOMEPARAMS = 0x0442

MGMSG_MOT_SET_LIMSWITCHPARAMS = 0x0423
MGMSG_MOT_REQ_LIMSWITCHPARAMS = 0x0424
MGMSG_MOT_GET_LIMSWITCHPARAMS = 0x0425

MGMSG_MOT_MOVE_HOME = 0x0443
MGMSG_MOT_MOVE_HOMED = 0x0444

MGMSG_MOT_MOVE_RELATIVE = 0x0448
MGMSG_MOT_MOVE_COMPLETED = 0x0464

MGMSG_MOT_MOVE_ABSOLUTE = 0x0453

MGMSG_MOT_MOVE_JOG = 0x046A

MGMSG_MOT_MOVE_VELOCITY = 0x0457

MGMSG_MOT_MOVE_STOP = 0x0465
MGMSG_MOT_MOVE_STOPPED = 0x0466

MGMSG_MOT_SET_BOWINDEX = 0x04F4
MGMSG_MOT_REQ_BOWINDEX = 0x04F5
MGMSG_MOT_GET_BOWINDEX = 0x04F6

MGMSG_MOT_SET_DCPIDPARAMS = 0x04A0
MGMSG_MOT_REQ_DCPIDPARAMS = 0x04A1
MGMSG_MOT_GET_DCPIDPARAMS = 0x04A2

MGMSG_MOT_SET_AVMODES = 0x04B3
MGMSG_MOT_REQ_AVMODES = 0x04B4
MGMSG_MOT_GET_AVMODES = 0x04B5

MGMSG_MOT_SET_POTPARAMS = 0x04B0
MGMSG_MOT_REQ_POTPARAMS = 0x04B1
MGMSG_MOT_GET_POTPARAMS = 0x04B2

MGMSG_MOT_SET_BUTTONPARAMS = 0x04B6
MGMSG_MOT_REQ_BUTTONPARAMS = 0x04B7
MGMSG_MOT_GET_BUTTONPARAMS = 0x04B8

MGMSG_MOT_SET_EEPROMPARAMS = 0x04B9

MGMSG_MOT_SET_POSITIONLOOPPARAMS = 0x04D7
MGMSG_MOT_REQ_POSITIONLOOPPARAMS = 0x04D8
MGMSG_MOT_GET_POSITIONLOOPPARAMS = 0x04D9

MGMSG_MOT_SET_MOTOROUTPUTPARAMS = 0x04DA
MGMSG_MOT_REQ_MOTOROUTPUTPARAMS = 0x04DB
MGMSG_MOT_GET_MOTOROUTPUTPARAMS = 0x04DC

MGMSG_MOT_SET_TRACKSETTLEPARAMS = 0x04E0
MGMSG_MOT_REQ_TRACKSETTLEPARAMS = 0x04E1
MGMSG_MOT_GET_TRACKSETTLEPARAMS = 0x04E2


class ThorResponse(NamedTuple):
    message_id: int
    param1: int
    param2: int
    data: bytes

class VelocityParameters(NamedTuple):
    channel: int # The channel being addressed
    min_velocity: int # The minimum (start) velocity in encoder counts / sec
    accleration: int # The acceleration in encoder counts / sec^2
    max_velocity: int # The maximum (final) velocity in encoder counts / sec

class JogParameters(NamedTuple):
    channel: int # The channel being addressed
    mode: int # Jogging mode (TODO)
    step_size: int
    min_velocity: int # minimum velocity
    accleration: int # Acceleration
    max_velocity: int # Maximum velocity
    stop_mode: int # Stopping mode (TODO)

class HomeParameters(NamedTuple):
    channel: int # The channel being addressed
    home_dir: int # The direction sense for a move to Home, either: 1 - forward/Positive or  2 - reverse/negative.
    limit_switch: int # The limit switch associated with the home position: 1 - hardware reverse or 4 - hardware forward
    home_velocity: int # The homing velocity.
    offset_distance: int # The distance of the Home position from the Home Limit Switch.

class PIDParameters(NamedTuple):
    channel: int # The channel being addressed
    p: int # The proportional gain
    i: int # The integral gain
    d: int # The differential gain
    integral_limit: int # The Integral Limit parameter is used to cap the value of the Integrator to prevent runaway of the integral sum
    filter_control: int # Identifies which of the above parameters are applied by

class ButtonParameters(NamedTuple):
    channel: int # The channel being addressed
    mode: int # 1 = the buttons are used to jog the motor (JogParameters). 2 =  each button can be programmed with a different position value
    position_1: int # The position (in encoder counts) to which the motor will move when the top button is pressed.
    position_2: int # The position (in encoder counts) to which the motor will move when the bottom button is pressed.
    timeout_1: int # The time in ms that button 1 must be depressed.
    timeout_2: int # The time in ms that button 2 must be depressed.

class PotentiometerParameters(NamedTuple):
    channel: int # The channel being addressed
    zero_wnd: int # The deflection from the mid position (in ADC counts 0 to 127) before motion can start
    vel1: int # The velocity (in encoder counts /sec) to move when between Wnd0 and PotDef1
    wnd1: int # The deflection from the mid position (in ADC counts, Wnd0 to 127) to apply Vel1
    vel2: int # The velocity (in encoder counts /sec) to move when between PotDef1 and PotDef2
    wnd2: int # The deflection from the mid position (in ADC counts, PotDef1 to 127) to apply Vel2
    vel3: int # The velocity (in encoder counts/sec) to move when between PotDef2 and PotDef3
    wnd3: int # The deflection from the mid position (in ADC counts PotDef2 to 127) to apply Vel3
    vel4: int # The velocity (in encoder counts /sec) to move when beyond PotDef3



class ThorRotator:

    EncCnt = 1638  # counts per degree

    velocity_scale = 36650
    acceleration_scale = 95.276

    print_packets = False


    def __init__(self, device: str="/dev/ttyUSB0", _serial=None):
        """
        Initialize new connection to Thor rotator

        Args:
            device: The rotator serial port
        """

        if _serial is None:
            self._serial = serial.Serial(device, baudrate=115200, timeout=0.1)
        else:
            self._serial = _serial

        self.dest = 0x50
        self.src = 0x01


    def degrees(self, count: int) -> float:
        """
        Convert encoder count to degrees.
        """
        return count / self.EncCnt


    def counts(self, degrees: float) -> int:
        """
        Convert degrees to encoder count.
        """
        return int(degrees * self.EncCnt)


    def _send(self,
            message_id: int,
            param1: Optional[int]=0,
            param2: Optional[int]=0,
            data: Optional[bytes]=None
        ) -> None:
        """
        Contruct a command and send it.

        Args:
            message_id: Message ID
            param1:
            param2:
            data: Arbitrary data
        """

        if data is not None:
            msg = struct.pack("<HHBB", message_id, len(data), self.dest | 0x80, self.src) + data
        else:
            msg = struct.pack("<HBBBB", message_id, param1, param2, self.dest, self.src)

        if self.print_packets:
            print("TX:", msg)
        self._serial.write(msg)


    def _get(self, message_id: int, length: int) -> bytes:
        """
        """
        raise NotImplementedError()
        msg = struct.pack("<HHBB", message_id, length, self.dest | 0x80, self.src)
        if self.print_packets:
            print("TX:", msg)

        self._serial.write(msg)

        data = self._serial.read(length)
        if self.print_packets:
            print("RX:", data)
        return data


    def _request(self, **kwargs) -> ThorResponse:
        """
        Send and receive
        """
        self._send(**kwargs)
        return self._receive()


    def _receive(self, timeout: Optional[int]=None) -> ThorResponse:
        """
        Wait for response from the device.

        Returns:
            ThorResponse object
        """
        t = self._serial.timeout
        try:
            if timeout is not None:
                self._serial.timeout = timeout
            hdr = self._serial.read(6)
        finally:
            self._serial.timeout = t

        if self.print_packets:
            print("RX:", hdr)

        if len(hdr) != 6:
            raise serial.SerialTimeoutException()
        message_id, param1, param2, dest, src = struct.unpack(">HBBBB", hdr)

        if 0x80 & dest:
            # Arbitrary length frame
            data_len = param1 | (param2 << 8)
            data = self._serial.read(data_len)
            if self.print_packets:
                print("   ", data)
            return ThorResponse(message_id, None, None, data)

        return ThorResponse(message_id, param1, param2, None)


    def identify(self, channel: int) -> None:
        """
        Identify the rotator by blinking the LED.
        """
        self._send(message_id=MGMSG_MOD_IDENTIFY)


    def set_channel_state(self, channel: int, state: bool=True) -> None:
        """
        Enable/disable channel

        Args:
            channel: The channel being addressed.
        """
        self._send(
            message_id=MGMSG_MOD_SET_CHANENABLESTATE,
            param1=channel,
            param2=(1 if state else 2)
        )


    def get_channel_state(self, channel: int) -> bool:
        """
        Get channel state

        Args:
            channel: The channel being addressed.

        Returns:
            State as boolean.
        """
        rsp = self._request(
            message_id=MGMSG_MOD_REQ_CHANENABLESTATE,
            data=struct.pack("B", channel),
        )
        assert channel == rsp.param1
        return rsp.param2 == 1


    def get_infos(self) -> bytes:
        """
        Get hardware information

        Returns:
            Hardware information as bytes.
        """
        rsp = self._request(message_id=MGMSG_HW_REQ_INFO)
        return rsp.data


    def set_position(self, channel: int, position: int) -> None:
        """
        Set position counter

        Args:
            channel: The channel being addressed.
            position: Position counter
        """

        self._send(
            message_id=MGMSG_MOT_SET_ENCCOUNTER,
            data=struct.pack("<Hi", channel, position)
        )


    def get_position(self) -> int:
        """
        Get position counter

        Returns:
            Current position as encoder count
        """

        rsp = self._request(message_id=MGMSG_MOT_REQ_ENCCOUNTER)
        channel_, position = struct.unpack("<Hi", rsp.data)
        return position


    def set_velocity(self, channel: int, min_vel: int, accl: int, max_vel: int) -> None:
        """
        Set velocity parameters

        Args:
            channel: The channel being addressed.
        """

        self._send(
            message_id=MGMSG_MOT_SET_VELPARAMS,
            data=struct.pack("<HIII", channel, min_vel, accl, max_vel)
        )


    def get_velocity(self, channel: int) -> VelocityParameters:
        """
        Get velocity paramters

        Args:
            channel: The channel being addressed.

        Returns:

        """
        rsp = self._request(
            message_id=MGMSG_MOT_REQ_VELPARAMS,
            param1=channel
        )
        return VelocityParameters(*struct.unpack("<HIII", rsp.data))


    def set_relative_distance(self, channel: int, distance: int) -> None:
        """
        Set relative relative distance.

        Args:
            channel: The channel being addressed.
            distance: Relative distance
        """

        self._send(
            message_id=MGMSG_MOT_SET_MOVERELPARAMS,
            data=struct.pack("<Hi", channel, distance)
        )


    def get_relative_distance(self, channel: int) -> int:
        """
        Get relative distance.

        Args:
            channel: The channel being addressed.

        Returns:
            relative distance is returned as an integer.
        """

        rsp = self._request(
            message_id=MGMSG_MOT_REQ_MOVERELPARAMS,
            param1=channel
        )
        channel_, rel = struct.unpack("<Hi", rsp.data)
        assert channel == channel_
        return rel


    def set_absolute_postion(self, channel: int, position: int) -> None:
        """
        Set absolute position.

        Args:
            channel: The channel being addressed.
            position: Absolute postion
        """

        self._send(
            message_id=MGMSG_MOT_SET_MOVEABSPARAMS,
            data=struct.pack("<Hi", channel, int(position))
        )


    def get_absolute_position(self, channel: int) -> int:
        """
        Get absolute position

        Args:
            channel: The channel being addressed.

        Returns:
            Absolute position as integer
        """

        rsp = self._request(
            message_id=MGMSG_MOT_REQ_MOVEABSPARAMS,
            param1=channel
        )
        channel_, position = struct.unpack("<Hi", rsp.data)
        assert channel == channel_
        return position


    def set_jog(self, channel: int, param: JogParameters) -> None:
        """
        Set jogging parameters

        Args:
            channel: The channel being addressed.
            param: Jog paramters
        """

        self._send(
            message_id=MGMSG_MOT_SET_JOGPARAMS,
            data=struct.pack("<HHIIIIH", channel, param.jog_mode, param.min_velocity, param.accleration, param.max_velocity, param.stop_mode)
        )


    def get_jog(self, channel: int) -> JogParameters:
        """
        Get Jog parameters

        Args:
            channel: The channel being addressed.

        Returns:
            JogParameters object
        """
        rsp = self._request(
            message_id=MGMSG_MOT_REQ_JOGPARAMS,
            param1=channel
        )
        return JogParameters(*struct.unpack("<HHIIIIH", rsp.data))


    def set_pot(self, channel: int, param: PotentiometerParameters) -> None:
        """
        Set pot parameters

        Args:
            channel: The channel being addressed.
            param: Potentiometer parameters as PotentiometerParameters object
        """

        self._send(
            message_id=MGMSG_MOT_SET_POTPARAMS,
            data=struct.pack("<HHIHIHIHI", channel, param.vel1, param.wnd1, param.vel2, param.wnd2, param.vel3, param.wnd3, param.vel4)
        )


    def get_pot(self, channel: int) -> PotentiometerParameters:
        """
        Get pot parameters

        Args:
            channel: The channel being addressed.

        Returns:
            PotentiometerParameters object
        """
        rsp = self._request(
            message_id=MGMSG_MOT_REQ_POTPARAMS,
            param1=channel
        )
        return PotentiometerParameters(*struct.unpack("<HHIHIHIHI", rsp.data))


    def set_button_parameters(self, channel: int, param: ButtonParameters) -> None:
        """
        Set button parameters

        Args:
            channel: The channel being addressed.
            param: Button paramters
        """

        self._send(
            message_id=MGMSG_MOT_SET_BUTTONPARAMS,
            data=struct.pack("<HHiiHH", param.channel, param.mode, param.position_1, param.position_2, param.timeout_1, param.timeout_2)
        )


    def get_button_parameters(self, channel: int) -> ButtonParameters:
        """
        Get button parameters.

        Args:
            channel: The channel being addressed.

        Returns:
            ButtonParameters object containing the received parameters.
        """

        rsp = self._request(
            message_id=MGMSG_MOT_REQ_BUTTONPARAMS,
            param1=channel
        )
        return ButtonParameters(*struct.unpack("<HHiiHH", rsp.data))


    def set_bow(self, channel: int, bow_index: int) -> None:
        """
        Set bow index

        Args:
            channel: The channel being addressed.
            bow_index: Bow index
        """

        self._send(
            message_id=MGMSG_MOT_SET_BOWINDEX,
            data=struct.pack("<HH", channel, bow_index)
        )


    def get_bow(self, channel: int) -> int:
        """
        Get bow index

        Args:
            channel: The channel being addressed.

        Returns:
            Bow index as integer
        """

        rsp = self._request(
            message_id=MGMSG_MOT_REQ_BOWINDEX,
            param1=channel
        )
        channel, bow_index = struct.unpack("<HH", rsp.data)
        return bow_index


    def set_pid(self, channel: int, param: PIDParameters) -> None:
        """
        Set PID parameters

        Args:
            channel: The channel being addressed.
            param:
        """

        self._send(
            message_id=MGMSG_MOT_SET_DCPIDPARAMS,
            data=struct.pack("<HIIIIH", channel, param.p, param.i, param.d, param.limit, param.filt)
        )


    def get_pid(self, channel: int) -> PIDParameters:
        """
        Get PID parameters

        Args:
            channel: The channel being addressed.

        Returns:
            PIDParameters object
        """

        rsp = self._request(message_id=MGMSG_MOT_REQ_DCPIDPARAMS, param1=channel)
        return PIDParameters(*struct.unpack("<HIIIIH", rsp.data))


    def get_home(self, channel: int) -> HomeParameters:
        """
        Get home parameters.

        Args:
            channel: The channel being addressed.

        Returns:
            HomeParameters object
        """

        rsp = self._request(
            message_id=MGMSG_MOT_REQ_HOMEPARAMS,
            param1=channel
        )
        return HomeParameters(*struct.unpack("<HHHIi", rsp.data))


    def move_home(self, channel: int):
        """
        Move to home.

        Args:
            channel: The channel being addressed.
        """
        self._send(
            message_id=MGMSG_MOT_MOVE_HOME,
            param1=channel
        )
        self._receive(timeout=120) # Wait for move done



    def move_relative(self, channel: int, distance: Optional[int]=None) -> None:
        """
        Move relative distance

        Args:
            channel: The channel being addressed.
            distance: Distance to be moved (Optional)
        """
        if distance is not None:
            self.set_relative_distance(channel, distance)

        self._send(MGMSG_MOT_MOVE_RELATIVE, param1=channel)
        self._receive(timeout=120) # Wait for move done


    def move_absolute(self, channel: int, position: Optional[int]=None) -> None:
        """
        Move to abosule position

        Args:
            channel: The channel being addressed.
            pos: Absolute postion to move.
        """
        if position is not None:
            self.set_absolute_postion(channel, position)

        self._send(MGMSG_MOT_MOVE_ABSOLUTE, param1=channel)
        self._receive(timeout=120) # Wait for move done


    def move_velocity(self, channel: int, direction: int) -> None:
        """
        Move forward/background

        Args:
            channel: The channel being addressed.
            direction: Movement direction
        """
        self._send(
            message_id=MGMSG_MOT_MOVE_VELOCITY,
            param1=channel,
            param2=direction
        )


    def stop(self, channel: int, stop_mode: int=2) -> None:
        """
        Stop any type of motor move (relative, absolute, homing or
        move at velocity) on the specified motor channel.

        Args:
            channel: The channel being addressed.
            stop_mode: either an immediate (abrupt) or profiles tops.
                1 = Stop immediately
                2 = Stop in a controller (profiled) manner
        """

        self._send(
            message_id=MGMSG_MOT_MOVE_STOP,
            param1=channel,
            param2=stop_mode
        )


    def move_jog(self, channel: int, direction: int) -> None:
        """
        Start a jog move on the specified motor channel.

        Args:
            channel: The channel being addressed
            direction: The direction to Jog. 1 = forward, 2 = reverse
        """

        self._send(
            message_id=MGMSG_MOT_MOVE_JOG,
            param1=channel,
            param2=direction,
        )
        self._receive(timeout=120) # Wait for move done

if __name__ == "__main__":

    import argparse
    parser = argparse.ArgumentParser(description="ThorLabs rotator control tool")
    parser.add_argument('--device', '-D', default="/dev/ttyUSB0", help="Serial device")
    parser.add_argument('--channel', '-c', type=int, default=1, help="Channel")
    args = parser.parse_args()

    thor = ThorRotator(device=args.device)

    def print_enum(obj):
        print(f"# {obj.__class__.__name__}:")
        for name, value in obj._asdict().items():
            print(f"{name:>20}: {value!r}")
        print()


    print(f"Channel #{args.channel}")
    print(f"State: {thor.get_channel_state(args.channel)}")
    print(f"Position: {thor.get_position()}")
    print_enum(thor.get_home(args.channel))
    print_enum(thor.get_pid(args.channel))
    print_enum(thor.get_velocity(args.channel))
    print_enum(thor.get_jog(args.channel))
    print_enum(thor.get_pot(args.channel))
    print_enum(thor.get_button_parameters(args.channel))

    #thor.set_position(args.channel, 0)
    #thor.move_home(args.channel)
    thor.move_absolute(args.channel, 50*thor.EncCnt)
    #thor.move_relative(args.channel, 50)
