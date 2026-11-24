
"""Synthetic data generator for testing the burst detector

Writes, into the output directory (tests/):
  synthetic_submissions.jsonl   one JSON object per line, sorted by created_utc
  synthetic_comments.jsonl      same, in the comment format (created_utc is a STRING to test edge case handling)
  truth.csv                     ground truth: what was injected, per phrase and week

Compress before running the program:
  zstd -f tests/synthetic_submissions.jsonl -o tests/synthetic_submissions.zst
  zstd -f tests/synthetic_comments.jsonl    -o tests/synthetic_comments.zst

Generatopmis random but seeded
"""
import csv
import itertools
import json
import os
import random
import sys

SEED = 42
WEEKS = 40
RECORDS_PER_WEEK = 5000            # background records per week (half submissions, half comments)
BASE_TS = 1_500_000_000
WEEK = 604_800
VOCAB = 20_000                     # background vocabulary size (Zipf distributed)
AUTHORS = 20_000                   # background author pool

OUT = sys.argv[1] if len(sys.argv) > 1 else "data"
rng = random.Random(SEED)

# Bckground
words = [f"wd{i:05d}" for i in range(VOCAB)]
cum_weights = list(itertools.accumulate(1.0 / (i + 1) for i in range(VOCAB)))


def filler(n):
    return rng.choices(words, cum_weights=cum_weights, k=n)


def background_text():
    return " ".join(filler(rng.randint(12, 40)))


def injected_text(phrase):
    before = filler(rng.randint(3, 8))
    after = filler(rng.randint(3, 8))
    return " ".join(before + phrase.split() + after)


background_authors = [f"user{i:05d}" for i in range(AUTHORS)]


def background_author():
    if rng.random() < 0.03:
        return "[deleted]"
    return rng.choice(background_authors)


# Injected phrases
# name, 5-word phrase, weeks it is active, occurrences per week, author pool (None = background pool)
def pool(prefix, n):
    return [f"{prefix}{i:05d}" for i in range(n)]


INJECT = [
    # constant rate all along, never reported
    dict(name="evergreen", phrase="evergreen phrase constant rate here",
         weeks=range(0, 40), per_week=60, authors=None),
    # a single week (days 2-4 only): flagged, but never confirmed
    dict(name="short", phrase="short lived blip phrase here",
         weeks=[8], per_week=150, authors=pool("short", 300), days=(2, 4)),
    # a real burst, ~150 distinct authors per week (below the Bottom-k size k=200: exact)
    dict(name="burst", phrase="burst phrase real trend now",
         weeks=range(15, 19), per_week=200, authors=pool("burst", 400)),
    # one or two accounts repeating a phrase: high score, must be rejected by the author filter
    dict(name="spam", phrase="spam phrase single account repeat",
         weeks=range(20, 24), per_week=300, authors=pool("spam", 2)),
    # a burst with ~1300 distinct authors per week (above k=200: estimated, +-35%)
    dict(name="burst_big", phrase="burst big phrase many authors",
         weeks=range(30, 34), per_week=1500, authors=pool("big", 5000)),
    # same shape at four different intensities
    dict(name="sweep20", phrase="sweep twenty phrase rate low",
         weeks=range(25, 28), per_week=20, authors=pool("sw20_", 200)),
    dict(name="sweep40", phrase="sweep forty phrase rate mid",
         weeks=range(25, 28), per_week=40, authors=pool("sw40_", 200)),
    dict(name="sweep80", phrase="sweep eighty phrase rate high",
         weeks=range(25, 28), per_week=80, authors=pool("sw80_", 200)),
    dict(name="sweep160", phrase="sweep onesixty phrase rate top",
         weeks=range(25, 28), per_week=160, authors=pool("sw160", 200)),
]

# Record builders
counter = itertools.count(1)


def make_record(kind, ts, author, text):
    n = next(counter)
    if kind == "sub":
        pid = f"s{n:07d}"
        return {"author": author, "created_utc": ts, "id": pid,
                "permalink": f"/r/memes/comments/{pid}/post/",
                "title": text, "selftext": ""}
    cid = f"c{n:07d}"
    rec = {"author": author, "created_utc": str(ts), "id": cid,   # string
           "link_id": f"t3_s{rng.randint(1, 99999):07d}", "body": text}
    if rng.random() < 0.1:                                        # some comments carry a permalink
        rec["permalink"] = f"/r/memes/comments/s0000001/post/_/{cid}/"
    return rec


items = {"sub": [], "com": []}          # (timestamp, line)
five_grams = 0

# background
for w in range(WEEKS):
    for _ in range(RECORDS_PER_WEEK):
        ts = BASE_TS + w * WEEK + rng.randint(0, WEEK - 1)
        kind = "sub" if rng.random() < 0.5 else "com"
        text = background_text()
        r = rng.random()
        if kind == "com" and r < 0.005:
            text = "[removed]"
        elif kind == "com" and r < 0.010:
            text = "https://i.redd.it/abc.png"
        five_grams += max(0, len(text.split()) - 4)
        rec = make_record(kind, ts, background_author(), text)
        items[kind].append((ts, json.dumps(rec)))

# injected phrases, with ground truth
truth = {}
for spec in INJECT:
    for w in spec["weeks"]:
        occ = 0
        who = set()
        for _ in range(spec["per_week"]):
            lo, hi = spec.get("days", (1, 7))
            ts = BASE_TS + w * WEEK + rng.randint(lo * 86400, hi * 86400 - 1)
            kind = "sub" if rng.random() < 0.5 else "com"
            author = rng.choice(spec["authors"] or background_authors)
            text = injected_text(spec["phrase"])
            five_grams += len(text.split()) - 4
            items[kind].append((ts, json.dumps(make_record(kind, ts, author, text))))
            occ += 1
            who.add(author)
        truth[(spec["name"], w)] = (spec["phrase"], occ, len(who))

# a few garbage lines
for _ in range(5):
    ts = BASE_TS + rng.randint(0, WEEKS * WEEK - 1)
    items["com"].append((ts, "this line is not json {"))

# Write
os.makedirs(OUT, exist_ok=True)
for kind, fname in (("sub", "synthetic_submissions.jsonl"), ("com", "synthetic_comments.jsonl")):
    items[kind].sort(key=lambda x: x[0])
    with open(os.path.join(OUT, fname), "w") as f:
        for _, line in items[kind]:
            f.write(line + "\n")

with open(os.path.join(OUT, "truth.csv"), "w", newline="") as f:
    wr = csv.writer(f)
    wr.writerow(["name", "phrase", "week", "occurrences", "distinct_authors"])
    for (name, w), (phrase, occ, authors) in sorted(truth.items(), key=lambda kv: (kv[0][0], kv[0][1])):
        wr.writerow([name, phrase, w, occ, authors])

print(f"submissions: {len(items['sub'])}  comments: {len(items['com'])}")
print(f"5-grams per week (approx): {five_grams / WEEKS:.0f}")
print(f"files written to {OUT}/")
