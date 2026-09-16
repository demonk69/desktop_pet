# Embedded Entry Point

The target SDK and MCU are `HW_VERIFY`, so V0.1 does not provide a fake embedded
`main`. After the board is identified, this directory will own SDK startup, construct
the selected HAL backends, and run the same `pet_app_t` used by the simulator.
