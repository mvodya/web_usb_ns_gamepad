# HID class realization for Nintendo Switch Gamepad

idf_component_register(SRCS "hid.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES "esp_tinyusb" "console")
