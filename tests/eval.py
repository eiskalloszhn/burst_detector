
"""Compare the detector output with the ground truth of the synthetic data

Prints, per injected phrase: weeks reported, detection delay, error of the Bottomk author
estimate and the score per week. Then counts false positives and summarises the cost statistics. 
Plots are saved if matplotlib exists.
"""
import csv
import os
import sys
from collections import Counter, defaultdict

BASE_TS = 1_500_000_000      # must match synthetic_data_gerator.py
WEEK = 604_800


def week_of_bucket(last_ts):
    # last_timestamp is the record that sealed the bucket: the first record of the next week.
    return (last_ts - BASE_TS) // WEEK - 1


def load_truth(path):
    phrase_of = {}
    active = defaultdict(dict)                      # name -> {week: (occurrences, authors)}
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            phrase_of[r["name"]] = r["phrase"]
            active[r["name"]][int(r["week"])] = (int(r["occurrences"]), int(r["distinct_authors"]))
    return phrase_of, active


def load_rows(path):
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            rows.append(dict(week=week_of_bucket(int(r["last_timestamp"])),
                             phrase=r["phrase"], score=float(r["score"]),
                             authors=float(r["authors_est"]), ratio=float(r["ratio"])))
    return rows


def detection_report(phrase_of, active, rows):
    by_phrase = defaultdict(list)
    for r in rows:
        by_phrase[r["phrase"]].append(r)

    print("\n=== injected phrases ===")
    print(f"{'name':10} {'active':>9} {'reported weeks':>22} {'delay':>6} "
          f"{'authors err (mean/max)':>24}")
    for name, phrase in phrase_of.items():
        weeks_true = sorted(active[name])
        got = sorted(by_phrase.get(phrase, []), key=lambda r: r["week"])
        weeks_rep = [r["week"] for r in got]
        delay = (weeks_rep[0] - weeks_true[0]) if weeks_rep else None

        errs = []
        for r in got:
            if r["week"] in active[name]:
                true = active[name][r["week"]][1]
                errs.append((r["authors"] - true) / true)
        err = (f"{sum(map(abs, errs)) / len(errs):.2f} / {max(map(abs, errs)):.2f}"
               if errs else "-")
        act = f"{weeks_true[0]}-{weeks_true[-1]}"
        rep = ",".join(map(str, weeks_rep)) if weeks_rep else "none"
        print(f"{name:10} {act:>9} {rep:>22} {str(delay):>6} {err:>24}")

    print("\n=== score per reported week ===")
    for name, phrase in phrase_of.items():
        got = sorted(by_phrase.get(phrase, []), key=lambda r: r["week"])
        if got:
            print(f"{name:10} " + "  ".join(f"w{r['week']}:{r['score']:.1f}" for r in got))


def overlaps(row_phrase, inj_phrase):
    """True if a 5-gram is a shifted window of an injected phrase"""
    a, b = row_phrase.split(), inj_phrase.split()
    for k in range(1, 5):
        if a[-k:] == b[:k] or a[:k] == b[-k:]:
            return True
    return False


def false_positive_report(phrase_of, active, rows):
    injected = set(phrase_of.values())
    not_injected = [r for r in rows if r["phrase"] not in injected]
    fragments = [r for r in not_injected
                 if any(overlaps(r["phrase"], p) for p in injected)]
    frag_ids = {id(r) for r in fragments}
    fp = [r for r in not_injected if id(r) not in frag_ids]
    late = []
    name_of = {p: n for n, p in phrase_of.items()}
    for r in rows:
        if r["phrase"] in injected and r["week"] not in active[name_of[r["phrase"]]]:
            late.append(r)

    per_week = Counter(r["week"] for r in fp)
    print("\n=== false positives ===")
    print(f"rows total: {len(rows)}   exact injected phrases: {len(rows) - len(not_injected)}   "
          f"shifted windows of an injected phrase: {len(fragments)}   "
          f"unrelated (true false positives): {len(fp)}   "
          f"injected phrase outside its active weeks: {len(late)}")
    if fp:
        print("weeks with false positives:", dict(sorted(per_week.items())))
        for r in fp[:10]:
            print(f"  w{r['week']} score {r['score']:.1f} authors {r['authors']:.0f} : {r['phrase']}")
    return per_week


def stats_report(path):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        return None
    last = rows[-1]
    n = len(rows)
    print("\n=== cost statistics ===")
    total = float(last["elapsed_s"])
    recs = int(float(last["records"]))
    print(f"records {recs}   time {total:.1f}s   mean {recs / total:.0f} records/s")
    idx = lambda p: max(0, min(n - 1, int(round(n * p)) - 1))
    print("peak RSS (MB) at 25/50/75/100% of the run: " +
          " / ".join(f"{float(rows[idx(p)]['peak_rss_mb']):.0f}" for p in (0.25, 0.5, 0.75, 1.0)))
    print(f"max buckets {max(int(float(r['buckets'])) for r in rows)}   "
          f"max reverse_map {max(int(float(r['reverse_map'])) for r in rows)}   "
          f"max mg_counters {max(int(float(r['mg_counters'])) for r in rows)}")
    try:
        t = [float(last[k]) for k in ("t_read_s", "t_tok_s", "t_hash_s")]
        s = sum(t) or 1.0
        print(f"time split  read/parse/merge {100 * t[0] / s:.0f}%   tokenize {100 * t[1] / s:.0f}%   "
              f"hash+window {100 * t[2] / s:.0f}%")
    except (ValueError, TypeError):
        print("time split unavailable: a timing column in the stats file is malformed")
    return rows


def plots(outdir, phrase_of, rows, fp_per_week, stats_rows):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from matplotlib.ticker import MaxNLocator
    except ImportError:
        print("\n(matplotlib not installed: no plots)")
        return
    os.makedirs(outdir, exist_ok=True)

    plt.figure(figsize=(7, 4))
    for name, phrase in phrase_of.items():
        pts = sorted((r["week"], r["score"]) for r in rows if r["phrase"] == phrase)
        if pts:
            plt.plot(*zip(*pts), marker="o", label=name)
    plt.xlabel("week")
    plt.ylabel("burst score")
    plt.yscale("log")
    plt.legend(fontsize=7)
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.tight_layout()
    plt.savefig(os.path.join(outdir, "scores.png"), dpi=150)
    plt.close()

    if fp_per_week:
        plt.figure(figsize=(7, 3))
        plt.bar(list(fp_per_week), list(fp_per_week.values()))
        plt.xlabel("week")
        plt.ylabel("false-positive rows")
        plt.tight_layout()
        plt.savefig(os.path.join(outdir, "false_positives.png"), dpi=150)
        plt.close()

    if stats_rows:
        plt.figure(figsize=(7, 3))
        plt.plot([int(float(r["records"])) for r in stats_rows],
                 [float(r["peak_rss_mb"]) for r in stats_rows])
        plt.xlabel("records processed")
        plt.ylabel("peak RSS (MB)")
        plt.tight_layout()
        plt.savefig(os.path.join(outdir, "memory.png"), dpi=150)
        plt.close()
    print(f"\nplots written to {outdir}/")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    truth_path, bursts_path = sys.argv[1], sys.argv[2]
    stats_path = sys.argv[3] if len(sys.argv) > 3 else None
    outdir = sys.argv[4] if len(sys.argv) > 4 else "eval_out"

    phrase_of, active = load_truth(truth_path)
    rows = load_rows(bursts_path)
    detection_report(phrase_of, active, rows)
    fp_per_week = false_positive_report(phrase_of, active, rows)
    stats_rows = stats_report(stats_path) if stats_path else None
    plots(outdir, phrase_of, rows, fp_per_week, stats_rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
