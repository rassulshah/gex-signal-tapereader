#!/usr/bin/env python3
"""
CHAT HISTORY GENERATOR  —  session-state/CHAT-HISTORY.md

Operator-mandated 2026-08-27: "everytime you make me a build i want this file updated with the chat
history of the context. then when i start a new context you will feed the last context history to
the new context also as a part of the load gex procedure."

WHY IT IS GENERATED AND NOT WRITTEN BY HAND
-------------------------------------------
A hand-written history drops whatever the writer forgets — which is precisely how ITEM 18 was lost
from the resume note and cost a whole session. This reads the ACTUAL session transcript, so it
cannot forget. Run it as part of every build (see tools/BUILD-CHECKLIST.md).

TWO TIERS, MAINTAINED AUTOMATICALLY
-----------------------------------
  CURRENT CONTEXT   full detail — every operator prompt verbatim, every substantive reply
  EARLIER CONTEXTS  compressed — operator prompts VERBATIM (always kept, they are small and they
                    are the highest-value record) plus the decisions; replies dropped

Each run demotes the previous CURRENT to compressed form and writes the new one on top, so the file
stays bounded on its own and a new context can read the whole thing.

⚠ OPERATOR PROMPTS ARE NEVER PARAPHRASED AND NEVER TRIMMED. This project already treats his exact
words as data (the standing business requirement is quoted verbatim in the resume note for the same
reason). Compression removes MY output, never his.

USAGE
-----
    python3 tools/chat-history.py                      # newest transcript, auto-detect version
    python3 tools/chat-history.py --version 14.53
    python3 tools/chat-history.py --title "item 18: the Yahoo courier"
    python3 tools/chat-history.py --transcript <path>  # explicit session .jsonl
    python3 tools/chat-history.py --dry-run            # print, do not write
"""

import argparse, glob, json, os, re, subprocess, sys, datetime

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(REPO, 'session-state', 'CHAT-HISTORY.md')
TRANSCRIPT_GLOB = os.path.expanduser('~/.claude/projects/*/*.jsonl')

REPLY_CAP = 1800          # per assistant turn, in the CURRENT tier only
MARK_CUR = '<!-- CURRENT-CONTEXT -->'
MARK_OLD = '<!-- EARLIER-CONTEXTS -->'

# Scaffolding that is not conversation. Anything matching is dropped from the operator's side.
NOISE = [
    re.compile(r'^\[Request interrupted', re.I),
    re.compile(r'^<local-command', re.I),
    re.compile(r'^<command-(name|message|args)>', re.I),
    re.compile(r'^Base directory for this skill:', re.I),
    re.compile(r'^This session is being continued from a previous conversation', re.I),
    re.compile(r'^Caveat: The messages below were generated', re.I),
]


def text_of(content):
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        return '\n'.join(b.get('text', '') for b in content
                         if isinstance(b, dict) and b.get('type') == 'text')
    return ''


def is_tool_result(content):
    return isinstance(content, list) and any(
        isinstance(b, dict) and b.get('type') == 'tool_result' for b in content)


def scrub(t):
    """Strip harness furniture. Keeps the operator's own words untouched."""
    t = re.sub(r'<system-reminder>.*?</system-reminder>', '', t, flags=re.S)
    t = re.sub(r'<uploaded_files>.*?</uploaded_files>', '[files attached]', t, flags=re.S)
    t = re.sub(r'<function_calls>.*?</function_calls>', '', t, flags=re.S)
    return t.strip()


def newest_transcript():
    files = glob.glob(TRANSCRIPT_GLOB)
    if not files:
        sys.exit('no transcript found under ~/.claude/projects — pass --transcript')
    return max(files, key=os.path.getmtime)


def parse(path):
    """Return an ordered list of ('user'|'assistant', timestamp, text)."""
    turns = []
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            try:
                d = json.loads(line)
            except Exception:
                continue
            m = d.get('message')
            if not isinstance(m, dict):
                continue
            role, content = m.get('role'), m.get('content')
            ts = (d.get('timestamp') or '')[11:16]
            if role == 'user' and not is_tool_result(content):
                t = scrub(text_of(content))
                if t and not any(p.match(t) for p in NOISE):
                    turns.append(('user', ts, t))
            elif role == 'assistant':
                t = scrub(text_of(content))
                if t:
                    turns.append(('assistant', ts, t))
    return turns


def merge_adjacent(turns):
    """Collapse consecutive same-role turns (streamed replies arrive in pieces)."""
    out = []
    for role, ts, t in turns:
        if out and out[-1][0] == role:
            out[-1][2] += '\n' + t
        else:
            out.append([role, ts, t])
    return out


def git(*args, default=''):
    try:
        return subprocess.check_output(['git', '-C', REPO] + list(args),
                                       stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return default


def detect_version():
    p = os.path.join(REPO, 'current', 'gex-signal-tapereader.user.js')
    try:
        with open(p, encoding='utf-8', errors='replace') as fh:
            for line in fh:
                m = re.search(r"GPTS_VERSION\s*=\s*'([^']+)'", line)
                if m:
                    return m.group(1)
    except Exception:
        pass
    return 'unknown'


def compress(block):
    """Demote a CURRENT entry to the compressed tier: keep the header, the operator's prompts
    verbatim, and the DECISIONS section. Drop the assistant replies."""
    lines = block.split('\n')
    keep, in_decisions = [], False
    for ln in lines:
        if ln.startswith('## '):
            keep.append(ln)
        elif ln.startswith('**OPERATOR:**') or ln.startswith('> '):
            keep.append(ln)
        elif ln.startswith('### DECISIONS') or ln.startswith('### SHIPPED') or ln.startswith('### OPEN AT CLOSE'):
            in_decisions = True
            keep.append('')
            keep.append(ln)
        elif ln.startswith('### '):
            in_decisions = False
        elif in_decisions:
            keep.append(ln)
    keep.append('')
    keep.append('_(compressed — operator prompts verbatim; replies dropped. Full detail is in git '
                'history for this file.)_')
    keep.append('')
    return '\n'.join(keep)


# ── COMPACTION RECOVERY ────────────────────────────────────────────────────────
# When a context is compacted, the harness REWRITES the .jsonl: the turns before the compaction are
# replaced by a single summary message, so this generator — reading the transcript, as it must —
# finds a context with ZERO operator prompts and silently writes an entry with none. That is the
# ITEM 18 failure the file exists to prevent, in a new costume.
#
# The summary itself carries the operator's messages verbatim, under a numbered "All user messages"
# heading. They are recovered from there, deduped against what CHAT-HISTORY.md already quotes (an
# earlier entry usually holds most of them), and marked RECOVERED so nobody mistakes a reconstructed
# ordering for the transcript's own.
COMPACT_HEAD = re.compile(r'^This session is being continued from a previous conversation', re.I)
ALLMSGS = re.compile(r'^\s*\d+\.\s*\*\*All user messages:?\*\*\s*$', re.M)


def compaction_summary(path):
    """The text of a compaction-continuation message, or '' when the transcript is intact."""
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            try:
                d = json.loads(line)
            except Exception:
                continue
            m = d.get('message')
            if not isinstance(m, dict) or m.get('role') != 'user':
                continue
            if is_tool_result(m.get('content')):
                continue
            t = text_of(m.get('content'))
            if COMPACT_HEAD.match(t.strip()):
                return t
            return ''          # a real prompt came first: nothing was compacted away
    return ''


def recovered_prompts(summary, already):
    """Verbatim operator prompts from a compaction summary that CHAT-HISTORY.md does not yet hold."""
    # ⚠ Prompts are stored one blockquote line per source line, so a MULTI-LINE prompt is '> ' broken
    # in the file and a raw substring test never matches it — every build would recover it again.
    already = re.sub(r'\s+', ' ', re.sub(r'^\s*>\s?', '', already or '', flags=re.M))
    m = ALLMSGS.search(summary or '')
    if not m:
        return []
    tail = summary[m.end():]
    stop = re.search(r'^\s*\d+\.\s*\*\*', tail, re.M)
    if stop:
        tail = tail[:stop.start()]
    out = []
    for ln in tail.split('\n'):
        ln = ln.strip()
        if not ln.startswith('-'):
            continue
        ln = ln.lstrip('- ').strip()
        q = re.findall(r'"([^"]+)"', ln)
        if not q:
            continue
        for txt in q:
            key = re.sub(r'\s+', ' ', txt).strip()
            if len(key) < 8:
                continue
            if already.find(key) >= 0:
                continue          # an earlier entry already quotes it
            if any(re.sub(r'\s+', ' ', o) == key for o in out):
                continue
            out.append(txt)
    return out


def carry_sections(prev, sid, place):
    """Hand-written sections from the CURRENT entry, when it belongs to the SAME session."""
    out = {}
    if not prev or MARK_CUR not in prev:
        return out
    cur = prev.split(MARK_CUR, 1)[1].split(MARK_OLD, 1)[0]
    if ('session `%s`' % sid) not in cur:
        return out                        # a different context: its notes go to the earlier tier
    for name in place:
        m = re.search(r'^### ' + re.escape(name) + r'\s*\n(.*?)(?=^### |\Z)', cur, re.S | re.M)
        if not m:
            continue
        txt = m.group(1).strip()
        if txt and txt != place[name]:
            out[name] = txt
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--transcript')
    ap.add_argument('--version')
    ap.add_argument('--title', default='')
    ap.add_argument('--dry-run', action='store_true')
    a = ap.parse_args()

    path = a.transcript or newest_transcript()
    turns = merge_adjacent(parse(path))
    if not turns:
        sys.exit('transcript parsed but held no conversation turns')

    prev_md = ''
    if os.path.exists(OUT):
        with open(OUT, encoding='utf-8') as fh:
            prev_md = fh.read()
    # ⚠ Dedupe against the EARLIER tier only. The CURRENT block is the one this run REPLACES, so a
    # prompt recovered by the previous run of this same context must be recovered again — otherwise
    # every build after the first writes an entry with no prompts at all.
    earlier_md = prev_md.split(MARK_OLD, 1)[1] if MARK_OLD in prev_md else prev_md
    rec = recovered_prompts(compaction_summary(path), earlier_md)
    if rec:
        turns = [['user', '', t + '\n\n_[RECOVERED from the compaction summary — this context was '
                  'compacted and the transcript no longer holds the turn itself]_'] for t in rec] + turns

    sid = os.path.basename(path).replace('.jsonl', '')[:8]
    ver = a.version or detect_version()
    day = datetime.date.today().isoformat()
    prompts = [t for t in turns if t[0] == 'user']

    head = f"## {day} · v{ver} · session `{sid}`"
    if a.title:
        head += f" — {a.title}"

    body = [MARK_CUR, '', head, '',
            f"_{len(prompts)} operator prompts · transcript `{os.path.basename(path)}`_", '',
            '### THE EXCHANGE', '']

    for role, ts, t in turns:
        if role == 'user':
            body.append(f"**OPERATOR:**")
            for ln in t.split('\n'):
                body.append(f"> {ln}")
            body.append('')
        else:
            cut = t if len(t) <= REPLY_CAP else t[:REPLY_CAP].rstrip() + ' …[trimmed]'
            body.append(f"**me ({ts}):** {cut}")
            body.append('')

    # ⚠ THE THREE SECTIONS BELOW ARE WRITTEN BY HAND AND THIS GENERATOR MUST NOT EAT THEM. A context
    # ships several builds, and until v15.18 every rebuild replaced DECISIONS / SHIPPED / OPEN AT
    # CLOSE with their placeholders — so the notes survived only if someone noticed and retyped them,
    # and `test_chat_history` went red for what looked like a process failure instead of a tool bug.
    PLACE = {
        'DECISIONS': '_Fill in before committing: what was settled, what was corrected, what was refused._',
        'SHIPPED': '_Version + what actually changed, or "no code shipped"._',
        'OPEN AT CLOSE': '_What the next context must pick up. Cross-check `LOCKED-ITEMS.md`._',
    }
    kept = carry_sections(prev_md, sid, PLACE) if 'prev_md' in dir() else {}
    for name in ('DECISIONS', 'SHIPPED', 'OPEN AT CLOSE'):
        body += ['### ' + name, '', kept.get(name) or PLACE[name], '']

    recent = git('log', '--oneline', '-12')
    if recent:
        body += ['### COMMITS THIS CONTEXT', '', '```', recent, '```', '']

    new_entry = '\n'.join(body)

    header = (
        "# CHAT HISTORY — what was actually said, context by context\n"
        "\n"
        "**Operator-mandated 2026-08-27.** Generated by `tools/chat-history.py` from the real session\n"
        "transcript, never written from memory — a remembered history drops things, which is exactly how\n"
        "ITEM 18 was lost. **Regenerate on every build.**\n"
        "\n"
        "**`load gex` MUST READ THE CURRENT-CONTEXT ENTRY IN FULL** before doing anything else. It is the\n"
        "one file that carries what was *said* rather than what was concluded — the corrections, the\n"
        "rejected approaches, and the operator's exact words.\n"
        "\n"
        "⚠ Operator prompts are verbatim and are never trimmed, in either tier. Compression removes the\n"
        "assistant's replies only.\n"
    )

    prev = ''
    if os.path.exists(OUT):
        with open(OUT, encoding='utf-8') as fh:
            prev = fh.read()

    old_block = ''
    if MARK_CUR in prev:
        after = prev.split(MARK_CUR, 1)[1]
        cur_block, rest = (after.split(MARK_OLD, 1) + [''])[:2]
        # ⚠ Same context rebuilt (several builds per session is normal): REPLACE its entry. Demoting
        # it would file a compressed copy of the context alongside the live one on every build.
        same = ('session `%s`' % sid) in cur_block
        kept = '' if same else compress(cur_block).strip()
        old_block = (kept + ('\n\n' if kept and rest.strip() else '') + rest.strip()).strip()
        # `rest` carries the tier's own heading; the writer emits one too. Without this a rebuilt
        # context stacks a fresh '# EARLIER CONTEXTS' line on the file every single run.
        old_block = re.sub(r'^#\s*EARLIER CONTEXTS\s*\n+', '', old_block)
    elif prev.strip():
        old_block = prev.strip()

    out = header + '\n---\n\n' + new_entry + '\n---\n\n' + MARK_OLD + '\n\n# EARLIER CONTEXTS\n\n' + old_block + '\n'

    if a.dry_run:
        print(out[:4000])
        print(f"\n... [{len(out)} bytes total, {len(prompts)} prompts]")
        return

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w', encoding='utf-8') as fh:
        fh.write(out)
    print(f"wrote {OUT} — {len(prompts)} operator prompts, {len(out)} bytes, v{ver}")


if __name__ == '__main__':
    main()
