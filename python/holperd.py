#!/usr/bin/env python3

import subprocess
import io
import argparse
import sys
import asyncio
import os
import json
import traceback
import pulsectl
import datetime
import systemd.daemon
import click


class UserError(Exception):
    def __init__(self, msg):
        self.msg = msg

    def __str__(self):
        return self.msg


class Logger:

    def __init__(self, filename=None):
        if filename == None:
            filename = f'/run/user/{os.getuid()}/holper.log'
        self.filename = filename
        self.file = open(filename, 'a')

    def __call__(self, *args):
        self.log(*args)

    def log(self, *args):
        logstr = ''.join([str(i) for i in args])
        time = datetime.datetime.now().strftime('%c')
        fulllog = f'{time} [{os.getpid()}]: {logstr}\n'
        sys.stdout.write(fulllog)
        self.file.write(fulllog)
        self.file.flush()

    def error(self, *args):
        self.log('\033[91m', *args, '\033[0m\n')

    def exception(self, *args):
        self.log('\033[91m', *args, '\033[0m\n', traceback.format_exc())


default_logger = None


def Log():
    global default_logger
    if not default_logger:
        default_logger = Logger()
    return default_logger


class Server:
    def __init__(self):
        self.root = click.Group()
        self.register_group(Sound)
        self.register_group(Clipboard)
        self.register_group(Timer)
        self.register_group(Info)

    def register_group(self, task):
        for name, group in task.groups().items():
            self.root.add_command(group, name)

    async def _send_and_close(self, reader, writer, msg, code):
        message = json.dumps({'response': msg, 'code': code})
        Log()(f'Response: {message}')
        writer.write(message.encode())
        await writer.drain()
        writer.close()
        await writer.wait_closed()

    async def _connected(self, reader, writer):
        command = await reader.read()
        Log()(f'Command: {command}')
        try:
            command = json.loads(command)
        except:
            Log().exception('Command failed to parse')
            await self._send_and_close(reader, writer, 'Command failed to parse', -1)
            return
        if len(command) == 0 or not isinstance(command, list):
            Log().error('Invalid command')
            await self._send_and_close(reader, writer, 'Invalid command', -1)
            return
        try:
            # TODO: fork?
            so = sys.stdout
            se = sys.stderr
            sys.stdout = io.StringIO()
            sys.stderr = io.StringIO()
            ctx = self.root.make_context(args=command, info_name='holper')
            ret = self.root.invoke(ctx)
            if not isinstance(ret, Response):
                ret = Response(ret)
            Log()(f'Return value: {ret.val}')
            await self._send_and_close(reader, writer, ret.val, ret.code)
            if ret.prt:
                try:
                    await ret.prt(ret, self)
                except:
                    Log().exception(f'post response task failed.')
            return
        except click.exceptions.Exit:
            await self._send_and_close(reader, writer, sys.stdout.getvalue(), 0)
            return
        except UserError as e:
            Log().exception(f'Command failed with user error {e}')
            await self._send_and_close(reader, writer, e.msg, -1)
            return
        except:
            Log().exception(f'Command failed:')
            await self._send_and_close(reader, writer, 'Command failed', -1)
            return
        finally:
            sys.stdout = so
            sys.stderr = se
        return

    async def start(self, location=None):
        if location == None:
            location = f'/run/user/{os.getuid()}/holper.sock'
        try:
            os.stat(location)
            Log()(f'{location} already exists.')
            try:
                os.unlink(location)
            except:
                Log().exception(f'Cannot remove {location}')
                return
        except:
            pass
        Log()(f'Starting server at {location}')
        self.server = await asyncio.start_unix_server(self._connected, location)
        systemd.daemon.notify('READY=1')
        await self.server.serve_forever()


def run_subprocess(args, stdin=None):
    Log()('subprocess: ', args)
    p = subprocess.Popen(args, stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if stdin and isinstance(stdin, str):
        stdin = stdin.encode('UTF-8')
    o, e = p.communicate(stdin)
    o = o.decode('UTF-8')
    e = e.decode('UTF-8')
    Log()(f'output {o}, {e}')
    return o, e


class Response:
    def __init__(self, val, code=0, post_response_task=None, extra=None):
        self.val = val
        self.code = code
        self.prt = post_response_task
        self.extra = extra


class Info:

    @staticmethod
    def groups():
        cls = Info
        info = click.Group()
        info.add_command(cls.log_location, 'log_location')
        return {'info': info}

    @staticmethod
    @click.command()
    def log_location():
        return Log().filename


class Timer:
    timers = {}

    @staticmethod
    def groups():
        cls = Timer
        timer = click.Group()
        timer.add_command(cls.add, 'add')
        timer.add_command(cls.cancel, 'cancel')
        return {'timer': timer}

    @staticmethod
    @click.command()
    @click.argument('timer_id')
    def cancel(timer_id):
        timer_id = int(timer_id)
        if timer_id not in Timer.timers:
            raise UserError(f'Timer {timer_id} not found')
        Timer.timers[timer_id] = False
        return True

    @staticmethod
    @click.command()
    @click.argument('seconds')
    @click.argument('say')
    def add(seconds, say):
        newid = None
        for i in range(10000):
            if i not in Timer.timers:
                newid = i
                break
        if newid == None:
            raise UserError(f'Too many active timers ({len(Timer.timers)})!')
        Timer.timers[newid] = True
        return Response(newid, 0, Timer._run_timer,
                        {'duration': seconds, 'say': say, 'id': newid})

    @staticmethod
    async def _run_timer(resp, server):
        myid = resp.extra['id']
        Log()(f'Timer {myid} starting')
        await asyncio.sleep(float(resp.extra['duration']))
        if not Timer.timers[myid]:
            del Timer.timers[myid]
            Log()(f'Timer {myid} was cancelled. aborting action.')
            return
        say = resp.extra['say']
        Log()(f'Timer {myid} is up, saying {say}')
        run_subprocess(['/usr/bin/espeak-ng', say])
        del Timer.timers[myid]


class Clipboard:
    clip_map = {'primary': 'p',
                'clipboard': 'b'}

    @staticmethod
    def groups():
        cls = Clipboard
        clip = click.Group()
        clip.add_command(cls.set, 'set')
        clip.add_command(cls.sync, 'sync')
        clip.add_command(cls.get, 'get')
        return {'clip': clip}

    @staticmethod
    def get_clip(clip):
        if clip not in Clipboard.clip_map:
            raise UserError(f'{clip} is not a valid clipboard. ' +\
                f'Valid values: {Clipboard.clip_map.keys()}')
        return Clipboard.clip_map[clip]

    @staticmethod
    @click.command()
    @click.argument('data')
    def sync(data):
        for name, clip in Clipboard.clip_map.items():
            Log()(f'Setting {data} to {name}')
            run_subprocess(['/usr/bin/xsel', f'-i{clip}'], data)
        return data

    @staticmethod
    @click.command()
    @click.argument('clipboard', default='primary', required=False)
    @click.argument('data')
    def set(clipboard, data):
        clip = Clipboard.get_clip(clipboard)
        Log()(f'Setting {data} to {clipboard}')
        run_subprocess(['/usr/bin/xsel', f'-i{clip}'], data)
        return data

    @staticmethod
    @click.command()
    @click.argument('clipboard', default='primary')
    def get(clipboard):
        clip = Clipboard.get_clip(clipboard)
        return run_subprocess(['/usr/bin/xsel', f'-o{clip}'], None)[0]


class Sound:
    @staticmethod
    def groups():
        cls = Sound
        vol = click.Group()
        vol.add_command(cls.volup, 'up')
        vol.add_command(cls.voldown, 'down')
        vol.add_command(cls.voltoggle, 'toggle')
        return {'vol': vol}

    @staticmethod
    @click.command()
    def voltoggle():
        cls = Sound
        with pulsectl.Pulse() as p:
            p = pulsectl.Pulse()
            sink = cls._get_sink(p)
            p.mute(sink, not sink.mute)
        return not sink.mute

    @staticmethod
    @click.command()
    @click.argument('amount', default=5)
    def volup(amount):
        return Sound.volchange(amount)

    @staticmethod
    @click.command()
    @click.argument('amount', default=5)
    def voldown(amount):
        return Sound.volchange(-amount)

    def _normalize(val):
        return max(0, min(2, val))

    @staticmethod
    def volchange(amount):
        with pulsectl.Pulse() as p:
            sink = Sound._get_sink(p)
            amount = 0.01 * float(amount)
            new_values = [Sound._normalize(i + amount) for i in sink.volume.values]
            p.volume_set(sink, pulsectl.PulseVolumeInfo(new_values))
            return new_values

    @staticmethod
    def _get_sink(p):
        sinks = p.sink_list()
        if not sinks:
            raise UserError('No available sink. Maybe restart pulseaudio?')
        if len(sinks) == 1:
            return sinks[0]
        backup = None
        for sink in sinks:
            if sink.name.startswith('bluez_sink.'):
                return sink
            if sink.name.startswith('alsa_output.'):
                backup = sink
        return backup or sinks[0]


def runtests():
    pass


if __name__ == '__main__':
    if len(sys.argv) == 2 and sys.argv[1] == 'test'):
        runtests()
    else:
        Log()('Starting')
        assert len(sys.argv) < 3
        loc = sys.argv[1] if len(sys.argv) == 2 else None
        loop = asyncio.get_event_loop()
        try:
            loop.run_until_complete(Server().start(loc))
        except KeyboardInterrupt:
            pass
        finally:
            loop.stop()
