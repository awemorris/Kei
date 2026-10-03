#!/usr/bin/env python3
"""Capture one fresh zedBSD Venus frame on the QEMU host.

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import subprocess
import sys
import time

from venus_rfb import capture as capture_rfb


def venus_hostmem():
    """The Venus guests' hostmem: VENUS_HOSTMEM, else plan/tools/guest/venus-hostmem.sh's default (q647)."""
    value = os.environ.get('VENUS_HOSTMEM')
    if value:
        return value
    text = (Path(__file__).resolve().parents[3] / 'plan/tools/guest/venus-hostmem.sh').read_text()
    match = re.search(r'VENUS_HOSTMEM=\$\{VENUS_HOSTMEM:-([0-9]+[KMG]?)\}', text)
    return match.group(1) if match else '256M'


def digest(path):
    result = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1048576), b''):
            result.update(chunk)
    return result.hexdigest()


def verify_blob_scanout_trace(data, width, height):
    """Prove BLOB request selection; display pixels and clean completion prove success separately."""
    pattern = re.compile(r'\b(virtio_gpu_cmd_set_scanout(?:_blob)?) id (\d+), '
                         r'res 0x([0-9a-fA-F]+), w (\d+), h (\d+), x (\d+), y (\d+)\r?$',
                         re.MULTILINE)
    counts = {'virtio_gpu_cmd_set_scanout_blob': 0, 'virtio_gpu_cmd_set_scanout': 0}
    resources = set()
    for match in pattern.finditer(data.decode('utf-8', errors='replace')):
        event, head, resource, frame_width, frame_height, x, y = match.groups()
        resource = int(resource, 16)
        if resource == 0 or (int(frame_width), int(frame_height)) != (width, height):
            continue
        counts[event] += 1
        if event == 'virtio_gpu_cmd_set_scanout_blob':
            resources.add(resource)
    if counts['virtio_gpu_cmd_set_scanout_blob'] == 0:
        raise RuntimeError('direct display trace has no nonzero BLOB scanout at the application extent')
    if counts['virtio_gpu_cmd_set_scanout'] != 0:
        raise RuntimeError('direct display trace used legacy scanout at the application extent')
    return {'scope': 'whole direct acceptance run', 'width': width, 'height': height,
            'blob_requests': counts['virtio_gpu_cmd_set_scanout_blob'],
            'legacy_requests': counts['virtio_gpu_cmd_set_scanout'],
            'blob_resources': sorted(resources), 'proves': 'request selection only'}


def interrupt_termination(signum, frame):
    # Convert catchable termination into normal stack unwinding, so a suspended
    # owned renderer is resumed before the disposable QEMU is retired.
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    raise RuntimeError('harness interrupted by SIGTERM')


class QMP:
    def __init__(self, path, log):
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.settimeout(5)
        deadline = time.monotonic() + 20
        while True:
            try:
                self.socket.connect(str(path))
                break
            except (FileNotFoundError, ConnectionRefusedError):
                if time.monotonic() >= deadline:
                    raise TimeoutError('QMP startup')
                time.sleep(0.05)
        self.stream = self.socket.makefile('rwb', buffering=0)
        self.log = log
        self.sequence = 0
        self.receive()
        self.call('qmp_capabilities')

    def receive(self):
        raw = self.stream.readline()
        if not raw:
            raise RuntimeError('QMP disconnected')
        reply = json.loads(raw)
        self.log.write(json.dumps({'receive': reply}) + '\n')
        self.log.flush()
        return reply

    def call(self, name, arguments=None):
        self.sequence += 1
        request = {'execute': name, 'id': self.sequence}
        if arguments is not None:
            request['arguments'] = arguments
        self.log.write(json.dumps({'send': request}) + '\n')
        self.log.flush()
        self.stream.write((json.dumps(request) + '\n').encode())
        deadline = time.monotonic() + 10
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(f'QMP command {name}')
            self.socket.settimeout(remaining)
            reply = self.receive()
            if reply.get('id') != self.sequence:
                continue
            if 'error' in reply:
                raise RuntimeError(f'{name}: {reply["error"]}')
            return reply.get('return')

    def text(self, text):
        punctuation = {' ': 'spc', '-': 'minus', '=': 'equal', '/': 'slash',
                       '.': 'dot', '&': 'shift-7', '\n': 'ret'}
        for char in text:
            key = punctuation.get(char, char)
            if key != 'shift-7' and not re.fullmatch('[a-z0-9]+', key):
                raise ValueError(f'unsupported console character {char!r}')
            self.call('human-monitor-command', {'command-line': f'sendkey {key} 1'})
            time.sleep(0.025)

    def close(self):
        self.stream.close()
        self.socket.close()


def verify_frame(path, left, right):
    data = path.read_bytes()
    match = re.match(rb'P6\s+(\d+)\s+(\d+)\s+255\s', data)
    if not match:
        raise ValueError('unsupported QMP PPM header')
    width, height = map(int, match.groups())
    if (width, height) != (256, 192):
        return False, f'geometry {width}x{height}'
    pixels = data[match.end():]
    wanted = (bytes(left) * 128 + bytes(right) * 128) * height
    if pixels != wanted:
        return False, 'RGB pixels differ from independent expectation'
    return True, 'all 49152 RGB pixels match'


def guest_text(path, offset=0):
    if not path.exists():
        return ''
    with path.open('rb') as stream:
        stream.seek(offset)
        return stream.read().decode(errors='replace')


def console_text(qmp, output, args):
    if args.console_address is None:
        return ''
    dump = output / 'console.bin'
    qmp.call('pmemsave', {'val': args.console_address,
                         'size': args.console_size, 'filename': str(dump)})
    text = dump.read_bytes().decode(errors='replace').replace('\x00', '\n')
    (output / 'console.log').write_text(text)
    return text


def run(args, exercise=None, harness_path=None):
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    image = args.image.resolve()
    run_image = output / 'guest.img'
    subprocess.run(['cp', '--reflink=auto', '--sparse=always', str(image), str(run_image)], check=True)
    variables = output / 'OVMF_VARS.fd'
    shutil.copyfile(args.variables, variables)
    debug = output / 'guest.log'
    renderer = output / 'qemu-renderer.log'
    qmp_path = output / 'qmp.sock'
    vnc_path = output / 'vnc.sock'
    command = [args.qemu, '-machine', 'pc,accel=kvm,memory-backend=memory',
               '-cpu', 'host', '-m', '1024', '-smp', '2',
               '-object', 'memory-backend-memfd,id=memory,size=1G,share=on',
               '-drive', f'if=pflash,format=raw,readonly=on,file={args.firmware}',
               '-drive', f'if=pflash,format=raw,file={variables}',
               '-drive', f'file={run_image},format=raw,if=ide,index=0',
               '-vga', 'none', '-device',
               'virtio-vga-gl,id=venus,venus=on,blob=on,hostmem=%s,max_outputs=1' % venus_hostmem(),
               '-display', 'egl-headless,rendernode=/dev/dri/renderD128',
               '-qmp', f'unix:{qmp_path},server=on,wait=off',
               '-vnc', f'unix:{vnc_path}',
               '-monitor', 'none', '-serial', 'none', '-nic', 'none',
               '-debugcon', f'file:{debug}', '-no-reboot',
               '-trace', 'enable=virtio_gpu_cmd_set_scanout_blob',
               '-trace', 'enable=virtio_gpu_cmd_set_scanout']
    # A profile may add devices (WS031 p013: the model viewer's USB tablet).
    command += list(getattr(args, 'extra_qemu', []))
    environment = os.environ.copy()
    environment['VK_DRIVER_FILES'] = args.icd
    environment['VIRGL_LOG_LEVEL'] = 'debug'
    environment['RENDER_SERVER_EXEC_PATH'] = str(args.render_server)
    if getattr(args, 'fault_test', None) in ('completion-delay', 'context-timeout', 'producer-stop', 'producer-exit-delayed'):
        environment['Q312_COMPLETION_GATE'] = str(output / 'completion.gate')
    library_directory = getattr(args, 'renderer_library_dir', None)
    library = None
    if library_directory is not None:
        library_directory = library_directory.resolve(strict=True)
        library = (library_directory / 'libvirglrenderer.so.1').resolve(strict=True)
        if not library.is_file() or not library_directory.is_dir():
            raise RuntimeError('selected renderer library directory has no shared library')
        environment['LD_LIBRARY_PATH'] = str(library_directory)
    if not args.render_server.is_file() or not os.access(args.render_server, os.X_OK):
        raise RuntimeError(f'Venus render server is not executable: {args.render_server}')
    report = {'started': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'image_sha256': digest(run_image), 'source_image_sha256': digest(image), 'argv': command,
              'environment': {k: environment[k] for k in ['VK_DRIVER_FILES', 'VIRGL_LOG_LEVEL', 'RENDER_SERVER_EXEC_PATH']},
              'phase': args.phase, 'frame': args.frame, 'status': 'running',
              'harness_sha256': digest(harness_path or __file__),
              'transport_harness_sha256': digest(__file__),
              'firmware_sha256': digest(args.firmware),
              'render_server_sha256': digest(args.render_server),
              'capture_method': 'QEMU egl-headless readback via VNC Unix RAW',
              'rfb_client_sha256': digest(Path(__file__).with_name('venus_rfb.py')),
              'console_address': args.console_address, 'console_size': args.console_size}
    if 'Q312_COMPLETION_GATE' in environment:
        report['environment']['Q312_COMPLETION_GATE'] = environment['Q312_COMPLETION_GATE']
        report['test_renderer_delay'] = True
    report_path = output / 'result.json'
    if library is not None:
        report['renderer_library'] = {'directory': str(library_directory),
                                      'path': str(library), 'sha256': digest(library)}
        report['environment']['LD_LIBRARY_PATH'] = str(library_directory)
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    if report['image_sha256'] != report['source_image_sha256']:
        raise RuntimeError('source image changed while copying')
    started = time.monotonic()
    qmp = None
    process = None
    previous_term = signal.signal(signal.SIGTERM, interrupt_termination)
    try:
        with renderer.open('wb') as log, (output / 'qmp.jsonl').open('w') as qlog:
            process = subprocess.Popen(command, env=environment, stdout=log, stderr=subprocess.STDOUT)
            qmp = QMP(qmp_path, qlog)
            report['pci'] = qmp.call('query-pci')
            report['qemu_version'] = qmp.call('query-version')
            if library is not None:
                mappings = Path(f'/proc/{process.pid}/maps').read_text()
                if str(library) not in mappings:
                    raise RuntimeError('QEMU did not load the selected isolated renderer library')
                report['renderer_library']['mapped_by_qemu'] = True
            deadline = time.monotonic() + args.timeout
            while time.monotonic() < deadline:
                text = guest_text(debug)
                if 'boot: starting init' in text:
                    text += console_text(qmp, output, args)
                if re.search(r'root@[^\r\n]*\$ ', text):
                    break
                if process.poll() is not None:
                    raise RuntimeError('QEMU exited during boot')
                if re.search(r'kernel panic|amd64 fault v=|VFS initialization failed|Open configured kernel:', text):
                    raise RuntimeError('guest boot failure (see guest.log)')
                time.sleep(0.1)
            else:
                raise TimeoutError('guest shell prompt')
            # Driver reset may retire the firmware surface before first present.
            try:
                qmp.call('screendump', {'filename': str(output / 'boot.ppm'), 'device': 'venus', 'head': 0})
                report['boot_surface_available'] = True
            except RuntimeError as error:
                if not str(error).startswith('screendump:') or "'desc': 'no surface'" not in str(error):
                    raise
                report['boot_surface_available'] = False
                report['boot_surface_note'] = 'GPU reset removed the firmware surface; require a fresh frame after present.'
            if exercise is not None and not args.boot_only:
                exercise(args, qmp, output, debug, vnc_path, process, report)
            elif args.boot_only:
                report['status'] = 'boot-pass'
            else:
                baseline = debug.stat().st_size
                guest_command = (f'/bin/venus-frame --phase={args.phase} --frame={args.frame} --hold=120\n')
                report['guest_command'] = guest_command.rstrip()
                qmp.text(guest_command)
                deadline = time.monotonic() + args.timeout
                pattern = (r'VENUS-FRAME PRESENT phase=' + re.escape(args.phase) + r' frame=' + str(args.frame) +
                           r' left=(\d+),(\d+),(\d+) right=(\d+),(\d+),(\d+)')
                while time.monotonic() < deadline:
                    text = guest_text(debug, baseline)
                    text += console_text(qmp, output, args)
                    marker = re.search(pattern, text)
                    if marker:
                        observed = list(map(int, marker.groups()))
                        if observed != args.left + args.right:
                            raise RuntimeError(f'guest pattern {observed} differs from independent expectation')
                        break
                    if 'VENUS-FRAME FAIL' in text or 'venus-frame:' in text:
                        raise RuntimeError('guest Vulkan/frame failure (see guest.log)')
                    if process.poll() is not None:
                        raise RuntimeError('QEMU exited during guest rendering')
                    time.sleep(0.05)
                else:
                    raise TimeoutError('fresh guest frame marker')
                deadline = time.monotonic() + 20
                mismatch = 'no VNC framebuffer yet'
                while time.monotonic() < deadline:
                    shot = output / 'frame.ppm'
                    remaining = deadline - time.monotonic()
                    capture_info = capture_rfb(vnc_path, shot, min(10, remaining))
                    report['rfb_capture'] = capture_info
                    passed, mismatch = verify_frame(shot, args.left, args.right)
                    if passed:
                        report.update(status='pass', pixel_check=mismatch,
                                      frame_sha256=digest(shot), expected=[args.left, args.right])
                        break
                    time.sleep(0.1)
                else:
                    raise TimeoutError(f'fresh expected VNC frame: {mismatch}')
            qmp.call('quit')
            qmp.close()
            qmp = None
            report['qemu_exit_code'] = process.wait(timeout=10)
            if report['qemu_exit_code'] != 0:
                raise RuntimeError('QEMU failed during shutdown')
            if library is not None:
                final_digest = digest(library)
                report['renderer_library']['final_sha256'] = final_digest
                if final_digest != report['renderer_library']['sha256']:
                    raise RuntimeError('selected renderer library changed during the attempt')
            if digest(args.render_server) != report['render_server_sha256']:
                raise RuntimeError('selected renderer server changed during the attempt')
    except (Exception, KeyboardInterrupt) as error:
        report.update(status='fail', error=str(error))
    finally:
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
        if qmp is not None:
            try:
                qmp.close()
            except OSError:
                pass
        if process is not None:
            try:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5)
                report['qemu_exit_code'] = process.returncode
            except Exception as cleanup_error:
                report['cleanup_error'] = str(cleanup_error)
                report['status'] = 'fail'
        report['elapsed_seconds'] = round(time.monotonic() - started, 3)
        report['guest_log_sha256'] = digest(debug) if debug.exists() else None
        report['renderer_log_sha256'] = digest(renderer)
        report_path.write_text(json.dumps(report, indent=2) + '\n')
        signal.signal(signal.SIGTERM, previous_term)
    print(json.dumps({k: report.get(k) for k in ['status', 'phase', 'frame', 'elapsed_seconds', 'error']}, indent=2))
    return 0 if report['status'] in ('pass', 'boot-pass') else 1


def rgb(text):
    values = [int(value) for value in text.split(',')]
    if len(values) != 3 or any(value < 0 or value > 255 for value in values):
        raise argparse.ArgumentTypeError('RGB must be three bytes')
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--render-server', type=Path, default=Path('/usr/libexec/virgl_render_server'))
    parser.add_argument('--renderer-library-dir', type=Path)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--phase', choices=['2d', 'venus'], default='venus')
    parser.add_argument('--frame', type=int, default=1)
    parser.add_argument('--left', type=rgb)
    parser.add_argument('--right', type=rgb)
    parser.add_argument('--timeout', type=int, default=120)
    parser.add_argument('--boot-only', action='store_true')
    parser.add_argument('--console-address', type=lambda text: int(text, 0))
    parser.add_argument('--console-size', type=int, default=32768)
    parser.add_argument('--qemu', default='qemu-system-x86_64')
    parser.add_argument('--icd', default='/usr/share/vulkan/icd.d/intel_icd.json')
    parser.add_argument('--firmware', default='/usr/share/OVMF/OVMF_CODE_4M.fd')
    parser.add_argument('--variables', default='/usr/share/OVMF/OVMF_VARS_4M.fd')
    args = parser.parse_args()
    if not 0 < args.timeout <= 600 or not 0 <= args.frame <= 1000000:
        parser.error('timeout must be 1..600 and frame 0..1000000')
    if not 0 < args.console_size <= 1048576:
        parser.error('console-size must be 1..1048576')
    if not args.boot_only and (args.left is None or args.right is None):
        parser.error('acceptance requires independent --left and --right expectations')
    return run(args)


if __name__ == '__main__':
    sys.exit(main())
