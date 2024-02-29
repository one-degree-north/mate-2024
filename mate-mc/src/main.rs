#![no_std]
#![no_main]

use adafruit_feather_rp2040 as board;
use board::hal;

use embedded_hal::serial::Write;
use hal::gpio::Pins;
use mate::Thruster;
use panic_halt as _;

use serde::{Deserialize, Serialize};

use usb_device::{class_prelude::*, prelude::*};
use usbd_serial::SerialPort;

#[derive(Debug, Deserialize, Serialize)]
pub struct Thruster {
    pub top_left: f64,
    pub top_right: f64,
    pub bottom_left: f64,
    pub bottom_right: f64,
    pub side_left: f64,
    pub side_right: f64,
}

#[board::entry]
fn main() -> ! {
    // Get standard singletons
    let mut pac = hal::pac::Peripherals::take().unwrap();

    // Set up the watchdog driver - needed by the clock setup code
    let mut watchdog = hal::Watchdog::new(pac.WATCHDOG);

    // Configure the clocks
    let clocks = hal::clocks::init_clocks_and_plls(
        board::XOSC_CRYSTAL_FREQ,
        pac.XOSC,
        pac.CLOCKS,
        pac.PLL_SYS,
        pac.PLL_USB,
        &mut pac.RESETS,
        &mut watchdog,
    )
    .ok()
    .unwrap();

    let usb_bus = UsbBusAllocator::new(hal::usb::UsbBus::new(
        pac.USBCTRL_REGS,
        pac.USBCTRL_DPRAM,
        clocks.usb_clock,
        true,
        &mut pac.RESETS,
    ));

    let mut serial = SerialPort::new(&usb_bus);

    let mut usb_dev = UsbDeviceBuilder::new(&usb_bus, UsbVidPid(0x16c0, 0x27dd))
        .manufacturer("mate")
        .product("mate-pi")
        .serial_number("1")
        .device_class(0x02)
        .build();

    loop {
        if usb_dev.poll(&mut [&mut serial]) {
            let mut buf = [0u8; 64];
            match serial.read(&mut buf) {
                Ok(0) => {}
                Err(_) => {}
                Ok(count) => match buf[0] {
                    42 => {
                        serial.write(&[42]).unwrap();
                        serial.flush().unwrap();
                    }
                    49 => {
                        read_exact(&mut serial, &mut buf[count..49]);
                        serial.write(&buf[1..49]);
                    }
                    _ => {}
                },
            }
        }
    }
}

fn write_byte_array<E: UsbBus>(serial: &mut SerialPort<E>, array: &[u8]) {
    let mut offset = 0;
    while offset < array.len() {
        let chunk = &array[offset..];
        let written = serial.write(chunk).unwrap();
        offset += written;
    }
    serial.flush().unwrap();
}

fn read_exact(serial: &mut SerialPort<hal::usb::UsbBus>, mut buf: &mut [u8]) {
    while !buf.is_empty() {
        match serial.read(buf) {
            Ok(0) => break,
            Ok(n) => buf = &mut buf[n..],
            Err(_) => {}
        }
    }
}
