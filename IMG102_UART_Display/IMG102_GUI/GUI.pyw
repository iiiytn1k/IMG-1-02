#!/usr/bin/python3
import dearpygui.dearpygui as ui
import serial.tools.list_ports
import json
import threading
import time
from enum import Enum
from img102_UART import *

baudrate_list = ('500000', '1000000', '2000000', '4000000')
dithering_list = ('None', 'Floyd-Steinberg', 'Atkinson', 'Burkes', 'Sierra', 'Two-Row Sierra', 'Sierra-lite', '8x8 Bayer')


def write_Settings():
    config_dict['capture_x'] = tr.capture_x
    config_dict['capture_y'] = tr.capture_y
    config_dict['capture_size'] = tr.capture_size
    config_dict['brightness'] = tr.brightness
    config_dict['contrast'] = tr.contrast
    config_dict['gamma'] = tr.gamma
    config_dict['clip_limit'] = tr.clip_limit
    config_dict['dithering_method'] = tr.dithering_method
    with open('conf.json', 'w') as outfile:
        json.dump(config_dict, outfile)


# Get a list of available COM ports
def get_port_list():
    portlist = serial.tools.list_ports.comports()
    comport_list = []
    for element in portlist:
        comport_list.append(element.device)
    if len(comport_list) == 0:
        comport_list.append('None')
    return comport_list


def open_port_button_callback():
    if UART.is_open:
        tr.startflag = False
        time.sleep(0.15)  # Wait until last frame is sent
        UART.flush()
        UART.close()
        ui.configure_item('start_button', label="Start")
        ui.configure_item('open_port_button', label="Open port")
        ui.configure_item('port_message', default_value="")
    else:
        UART.port = ui.get_value('port_combo')
        UART.baudrate = ui.get_value('baudselect_combo')
        UART.write_timeout = 0
        UART.rtscts = False
        UART.dsrdtr = False
        try:
            UART.open()
        except:
            ui.configure_item('port_message', default_value="Can't open port", color=[255, 0, 0])
        else:
            config_dict['default_port'] = UART.port
            ui.configure_item('open_port_button', label="Close port")
            ui.configure_item('port_message', default_value="Opened", color=[0, 255, 0])


def startbutton_callback():
    if tr.startflag is False:
        if UART.is_open or DEBUG_FLAG:
            tr.startflag = True
            ui.configure_item('start_button', label="Stop")
        else:
            ui.configure_item('no_port_selected_popup', show=True)
    else:
        tr.startflag = False
        ui.configure_item('start_button', label="Start")


def dithering_combo_callback(sender, app_data):
    tr.dithering_method = app_data


def x_pos_callback(sender, app_data):
    tr.capture_x = app_data


def y_pos_callback(sender, app_data):
    tr.capture_y = app_data


def image_size_callback(sender, app_data):
    tr.capture_size = app_data


def bcg_tab_callcack(sender, app_data):
    if app_data == 'manual_tab':
        tr.manual_flag = True
    elif app_data == 'auto_tab':
        tr.manual_flag = False


def brightness_callback(sender, app_data):
    tr.brightness = app_data


def contrast_callback(sender, app_data):
    tr.contrast = app_data


def gamma_callback(sender, app_data):
    tr.gamma = app_data


def he_radio_callback(sender, app_data):
    print(app_data)
    if app_data == "Histogram equalization":
        tr.hist_eq_flag = True
        tr.clahe_flag = False
    else:
        tr.hist_eq_flag = False
        tr.clahe_flag = True


def clip_limit_callback(sender, app_data):
    tr.clip_limit = app_data


def bcg_zero_callback(sender):
    if sender == 'reset_brightness_button':
        ui.set_value('brightness_slider', 0)
        brightness_callback(0, 0)
    elif sender == 'reset_contrast_button':
        ui.set_value('contrast_slider', 0)
        contrast_callback(0, 0)
    elif sender == 'reset_gamma_button':
        ui.set_value('gamma_slider', 1.0)
        gamma_callback(0, 1.0)


def exit_callback():
    tr.startflag = False
    write_Settings()
    time.sleep(0.15)  # Wait until last frame is sent
    if UART.is_open:
        UART.flush()
        UART.close()
    print("Exit")


if __name__ == "__main__":
    # Read settings from conf.json
    try:
        with open('conf.json') as config_file:
            config_dict = json.load(config_file)
    except Exception:
        config_dict = {'capture_x': 100, 'capture_y': 100, 'capture_size': 100,
                       'brightness': 0, 'contrast': 0, 'gamma': 1.0, 'clip_limit': 3.0,
                       'dithering_method': 'Atkinson', 'default_port': 'COM1'}

    tr = UART_send_data_thread(config_dict['capture_x'], config_dict['capture_y'], config_dict['capture_size'], True, True, False,
                               config_dict['brightness'], config_dict['contrast'], config_dict['gamma'], config_dict['clip_limit'],
                               config_dict['dithering_method'])
    t1 = threading.Thread(target=tr.main_loop, daemon=True)
    t1.start()

    ui.create_context()
    ui.create_viewport(title="IMG1-02 Demo", width=340, height=620, resizable=False, always_on_top=True)

    with ui.font_registry():
        default_font = ui.add_font('FreeSans.otf', 22)

    with ui.window(id='no_port_selected_popup', pos=[25, 200], width=270, no_title_bar=True, no_resize=True, modal=True, popup=True):
        ui.add_text("Please select and open COM-port", pos=[10, 15])
        ui.add_button(label="OK", indent=80, width=75, pos=[0, 57], callback=lambda: ui.configure_item('no_port_selected_popup', show=False))

    with ui.window(id='main_window', label="IMG1-02 UART Graphical Display", width=340, height=620, no_move=True, no_resize=True, no_close=True, no_collapse=True):
        ui.bind_font(default_font)
        ui.add_text("COM-port:")
        ui.add_combo(items=get_port_list(), tag='port_combo', width=200, default_value=config_dict['default_port'])
        ui.add_text("Baudrate:")
        ui.add_combo(tag='baudselect_combo', items=baudrate_list, width=200, default_value='2000000')
        with ui.group(horizontal=True):
            ui.add_button(tag='open_port_button', width=100, label="Open port", callback=open_port_button_callback)
            ui.add_text("", tag='port_message')
        ui.add_separator()

        ui.add_text("Capture settings:")
        with ui.group(horizontal=True):
            ui.add_text("Top:")
            ui.add_input_int(indent=50, max_value=2000, min_value=1, min_clamped=True, max_clamped=True, step=1, step_fast=100, default_value=config_dict['capture_y'], width=120, callback=y_pos_callback)
        with ui.group(horizontal=True):
            ui.add_text("Left:")
            ui.add_input_int(indent=50, max_value=2000, min_value=1, min_clamped=True, max_clamped=True, default_value=config_dict['capture_x'], width=120, callback=x_pos_callback)
        with ui.group(horizontal=True):
            ui.add_text("Size:")
            ui.add_input_int(indent=50, max_value=2000, min_value=1, min_clamped=True, max_clamped=True, default_value=config_dict['capture_size'], width=120, callback=image_size_callback)

        ui.add_text("Dithering algorithm:")
        ui.add_combo(items=dithering_list, width=200, default_value=config_dict['dithering_method'], callback=dithering_combo_callback)

        with ui.tab_bar(callback=bcg_tab_callcack):
            with ui.tab(tag='manual_tab', label="Manual"):
                with ui.group(horizontal=True, horizontal_spacing=10):
                    ui.add_text("Brightness:")
                    ui.add_slider_int(tag='brightness_slider', indent=90, max_value=127, min_value=-127, width=180, callback=brightness_callback, default_value=config_dict['brightness'])
                    ui.add_button(tag='reset_brightness_button', label=" R ", width=28, callback=bcg_zero_callback)
                with ui.group(horizontal=True, horizontal_spacing=10):
                    ui.add_text("Contrast:")
                    ui.add_slider_int(tag='contrast_slider', indent=90, max_value=127, min_value=-127, width=180, callback=contrast_callback, default_value=config_dict['contrast'])
                    ui.add_button(tag='reset_contrast_button', label=" R ", width=28, callback=bcg_zero_callback)
                with ui.group(horizontal=True, horizontal_spacing=10):
                    ui.add_text("Gamma:")
                    ui.add_slider_float(tag='gamma_slider', indent=90, min_value=0.1, max_value=7.0, width=180, format='%.1f', callback=gamma_callback, default_value=config_dict['gamma'])
                    ui.add_button(tag='reset_gamma_button', label=" R ", width=28, callback=bcg_zero_callback)
            with ui.tab(tag='auto_tab', label="Auto"):
                ui.add_radio_button(("Histogram equalization", "Contrast Limited AHE:"), callback=he_radio_callback)
                with ui.group(horizontal=True, horizontal_spacing=10):
                    ui.add_text("Clip limit: ")
                    ui.add_slider_float(min_value=0.01, max_value=4.0, indent=90, width=200, format='%.2f', callback=clip_limit_callback, default_value=config_dict['clip_limit'])

        ui.add_button(tag='start_button', label="Start", width=200, pos=[60, 530], callback=startbutton_callback, enabled=True)

    ui.setup_dearpygui()
    ui.set_exit_callback(exit_callback)
    ui.show_viewport()
    ui.start_dearpygui()
    ui.destroy_context()
