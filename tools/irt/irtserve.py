#!/usr/bin/env python3
"""
IRT FLEXLEVELS SERVER  —  serves FlexLevelsExport.csv over HTTP so IRT will re-read it.

WHY THIS EXISTS (measured 2026-08-27, not guessed)
--------------------------------------------------
The panel writes the CSV IN PLACE and correctly (v14.52: inPlace:true, file identity preserved,
mtime advancing every 180s) — and IRT still never re-reads it. Proof: at 13:33:45 the file said
"SPXW KING 100% ~ = 7717.00" with one row while IRT drew 7748.25 and 7731.50 from minutes earlier.
31 points and a missing row.

**IRT does not poll a LOCAL file.** Its "Check For Updates Every: 1 Minute" governs Remote File mode
only. No write strategy in the browser can fix that, so the file is served over HTTP instead.

⚠ USE 127.0.0.1, NEVER localhost. On Windows `localhost` usually resolves to IPv6 ::1 first, and a
server bound to IPv4 refuses it — you get ERR_CONNECTION_REFUSED while the server is running fine.
This binds BOTH stacks where the OS allows it, but the printed URL uses 127.0.0.1 regardless.

⚠ NO-STORE IS THE POINT. Plain http.server answers a conditional request with 304 Not Modified, so a
polling client can sit on a cached copy forever — exactly the bug we are escaping. Cache-Control is
no-store AND Last-Modified/ETag are stripped, so every poll is a real read.
"""
import http.server, os, socket, socketserver, sys, time, traceback

# ⚖ HAND-SET: the panel exports every 180s, so anything past ~8 minutes means it has
# stopped writing — not that the market is quiet. Generous enough to survive a couple of
# missed exports without crying wolf.
STALE_WARN_SEC = 480

PORTS = [8000, 8765, 8181, 8899]          # first one that binds wins


class NoCache(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        http.server.SimpleHTTPRequestHandler.end_headers(self)

    def send_header(self, key, value):
        if key.lower() in ('last-modified', 'etag'):
            return                                  # no validators -> no 304 -> no stale poll
        http.server.SimpleHTTPRequestHandler.send_header(self, key, value)

    def log_message(self, fmt, *args):
        """Log the request AND the age of the file being served.

        ⚠⚠ THE FAILURE THIS EXISTS TO CATCH. If the panel stops writing — Chrome drops the folder
        permission on every page load, so this happens most mornings — the CSV simply stays on disk
        and this server keeps serving it happily. IRT polls, gets 200, and draws YESTERDAY'S LEVELS,
        while the log fills with healthy-looking 200s. That is worse than a blank chart: nothing
        anywhere says the numbers are stale, and a 200 reads as proof that everything works.
        A green light on a dead feed is the exact class of mistake that cost two days on this
        problem already. So every line carries the file's age, and a stale file shouts.
        """
        age = ''
        try:
            secs = time.time() - os.path.getmtime('FlexLevelsExport.csv')
            mins = int(secs // 60)
            if secs > STALE_WARN_SEC:
                age = ('   *** CSV IS %d MIN OLD — THE PANEL IS NOT WRITING. '
                       'Check Chrome: Atlas open? folder permission granted? ***' % mins)
            else:
                age = '   (csv %dm%02ds old)' % (mins, int(secs % 60))
        except Exception:
            age = '   *** FlexLevelsExport.csv IS MISSING ***'
        sys.stdout.write("  %s  %s%s\n" % (self.log_date_time_string(), fmt % args, age))
        sys.stdout.flush()


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True
    address_family = socket.AF_INET6 if socket.has_ipv6 else socket.AF_INET

    def server_bind(self):
        # dual-stack where the OS allows it, so BOTH 127.0.0.1 and ::1 answer
        if self.address_family == socket.AF_INET6:
            try:
                self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
            except Exception:
                pass
        socketserver.ThreadingTCPServer.server_bind(self)


def bind_any():
    """Bind loopback, IPv4 FIRST, and return the host that actually bound.

    ⚠⚠ THE BUG THIS FIXES, MEASURED 2026-08-27: the first cut tried '::1' first and returned on
    success, so the server listened on IPv6 ONLY — while printing a 127.0.0.1 URL that could never
    connect. Chrome confirmed it: 127.0.0.1 refused, [::1] served 200. IRT was pointed at a dead
    address and polled nothing, and the empty log looked like "IRT is not fetching" when the truth
    was "nothing was listening where we told it to look".

    ⚠ BINDING '::1' DOES NOT ACCEPT IPv4. IPV6_V6ONLY=0 only gives you dual-stack when you bind the
    WILDCARD '::' — never a specific loopback literal. Setting the sockopt on a '::1' bind is a
    no-op that reads like a fix.

    IPv4 goes first because it is what both Chrome and IRT reach for on a numeric loopback URL, and
    because the URL we print must be the one that actually bound. The printed URL is derived from
    the bind result, never assumed — that mismatch is the whole defect.
    """
    last = None
    for port in PORTS:
        for fam, host, shown in ((socket.AF_INET, '127.0.0.1', '127.0.0.1'),
                                 (socket.AF_INET6, '::1', '[::1]')):
            try:
                Server.address_family = fam
                return Server((host, port), NoCache), port, shown
            except OSError as e:
                last = e
                continue
    raise SystemExit('could not bind any of %s - something else is using them.\n%s' % (PORTS, last))


if __name__ == '__main__':
    try:
        folder = sys.argv[1] if len(sys.argv) > 1 else os.getcwd()
        if not os.path.isdir(folder):
            raise SystemExit('not a folder: %s' % folder)
        os.chdir(folder)

        httpd, port, shown = bind_any()
        url = 'http://%s:%d/FlexLevelsExport.csv' % (shown, port)

        print('')
        print('  serving : %s' % folder)
        print('  csv     : %s' % ('FOUND' if os.path.exists('FlexLevelsExport.csv')
                                  else '*** NOT HERE YET — check the panel gear picked THIS folder ***'))
        print('')
        print('  ' + '=' * 62)
        print('  PASTE THIS INTO IRT  ->  Remote File')
        print('')
        print('     %s' % url)
        print('')
        print('  Check For Updates Every: 1 Minute,  then OK')
        print('  ' + '=' * 62)
        print('')
        print('  LISTENING. Every line below is IRT actually polling —')
        print('  if nothing appears after you press OK, IRT is not fetching.')
        print('')
        httpd.serve_forever()
    except KeyboardInterrupt:
        print('\n  stopped by Ctrl+C')
    except SystemExit as e:
        print('\n  *** %s' % e)
    except Exception:
        print('\n  *** the server crashed:')
        traceback.print_exc()
    finally:
        # ⚠ never let the window vanish — a closed window is indistinguishable from a crash,
        # which is exactly how the first attempt was lost.
        try:
            input('\n  press Enter to close this window...')
        except Exception:
            pass
