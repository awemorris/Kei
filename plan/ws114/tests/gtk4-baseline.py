#!/usr/bin/env python3
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Supplement gtk4-demo with observable GTK4 API operations in an owned guest."""
import json
import os
import gi

gi.require_version('Gtk', '4.0')
from gi.repository import Gio, GLib, Gtk


def report(event, **fields):
    print(json.dumps({'event': event, 'pid': os.getpid(), **fields}, ensure_ascii=False), flush=True)


class Baseline(Gtk.Application):
    def __init__(self):
        super().__init__(application_id=os.environ.get('WS114_PROBE_ID', 'org.zedbsd.WS114Baseline'))
        self.connect('activate', self.activate)

    def activate(self, app):
        self.window = Gtk.ApplicationWindow(application=app, title=os.environ.get('WS114_PROBE_TITLE', 'WS114 GTK4 baseline'))
        self.window.set_default_size(700, 460)
        header = Gtk.HeaderBar()
        self.window.set_titlebar(header)
        menu = Gio.Menu()
        menu.append('First action', 'app.first')
        menu.append('Second action', 'app.second')
        for name in ('first', 'second'):
            action = Gio.SimpleAction.new(name, None)
            action.connect('activate', lambda action, value: report('menu-action', name=action.get_name()))
            self.add_action(action)
        menu_button = Gtk.MenuButton(label='Menu', menu_model=menu)
        menu_button.set_tooltip_text('WS114 menu tooltip')
        header.pack_start(menu_button)
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)
        box.set_margin_start(22)
        box.set_margin_end(22)
        box.set_margin_top(20)
        box.set_margin_bottom(20)
        self.window.set_child(box)
        box.append(Gtk.Label(label='Standard Debian GTK4 — QMP input and lifecycle probe'))
        self.source = Gtk.Entry(text=os.environ.get('WS114_PROBE_TEXT', 'q581 clipboard Ω 日本語'))
        self.source.set_placeholder_text('Copy source')
        self.source.connect('changed', lambda entry: report('source-text', text=entry.get_text()))
        box.append(self.source)
        self.target = Gtk.Entry()
        self.target.set_placeholder_text('Paste target / keyboard input')
        self.target.connect('changed', lambda entry: report('target-text', text=entry.get_text()))
        box.append(self.target)
        row = Gtk.Box(spacing=12)
        box.append(row)
        for label, callback in (('Dialog', self.dialog), ('File dialog', self.file_dialog),
                                ('Maximize', self.maximize), ('Fullscreen', self.fullscreen)):
            button = Gtk.Button(label=label)
            button.connect('clicked', callback)
            row.append(button)
        text = Gtk.TextView()
        text.get_buffer().set_text('Scroll and select text.\n' + '\n'.join('GTK4 line %02d' % i for i in range(1, 30)))
        scroll = Gtk.ScrolledWindow(vexpand=True)
        scroll.set_child(text)
        adjustment = scroll.get_vadjustment()
        adjustment.connect('value-changed', lambda value: report('scroll', value=value.get_value(), upper=value.get_upper(), page=value.get_page_size()))
        box.append(scroll)
        self.window.connect('close-request', lambda window: report('close-request'))
        self.window.connect('notify::maximized', lambda window, prop: report('maximized', value=window.is_maximized()))
        self.window.connect('notify::fullscreened', lambda window, prop: report('fullscreen', value=window.is_fullscreen()))
        self.window.connect('notify::is-active', lambda window, prop: report('focus', active=window.is_active()))
        self.window.present()
        GLib.timeout_add(800, self.snapshot)
        report('gtk-version', version='%d.%d.%d' % (Gtk.get_major_version(), Gtk.get_minor_version(), Gtk.get_micro_version()))

    def snapshot(self):
        native = self.window.get_native()
        renderer = native.get_renderer()
        surface = native.get_surface()
        report('snapshot', renderer=renderer.__gtype__.name, width=surface.get_width(),
               height=surface.get_height(), scale=surface.get_scale_factor(),
               display=surface.get_display().__gtype__.name)
        return False

    def dialog(self, button):
        child = Gtk.Window(title='WS114 modal child', transient_for=self.window, modal=True)
        child.set_default_size(360, 160)
        content = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=20)
        content.set_margin_top(24)
        content.set_margin_start(24)
        content.set_margin_end(24)
        content.set_margin_bottom(24)
        child.set_child(content)
        content.append(Gtk.Label(label='Modal dialog with transient parent'))
        close = Gtk.Button(label='Close dialog')
        close.connect('clicked', lambda button: (report('dialog-closed'), child.close()))
        content.append(close)
        child.present()
        report('dialog-opened', modal=child.get_modal(), transient=child.get_transient_for() is self.window)

    def file_dialog(self, button):
        dialog = Gtk.FileDialog(title='WS114 choose file')
        dialog.open(self.window, None, self.file_response)
        report('file-dialog-requested')

    def file_response(self, dialog, result):
        try:
            file = dialog.open_finish(result)
            report('file-dialog-response', path=file.get_path())
        except GLib.Error as error:
            report('file-dialog-response', error=error.message, domain=error.domain, code=error.code)

    def maximize(self, button):
        self.window.maximize()
        GLib.timeout_add(1200, self.snapshot)
        GLib.timeout_add(4000, lambda: (self.window.unmaximize(), False)[1])

    def fullscreen(self, button):
        self.window.fullscreen()
        GLib.timeout_add(1200, self.snapshot)
        GLib.timeout_add(4000, lambda: (self.window.unfullscreen(), False)[1])


app = Baseline()
app.run(None)
