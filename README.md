# r/memes Burst Detector

Streaming burst detector for Reddit (`r/memes` submissions and comments), developed as the final project for **[CM0622] Algorithms for Massive Data** (Ca' Foscari University of Venice).

## How it works

The program processes time-sorted `.zst` dumps in a single pass using sublinear space:

1. **Streaming I/O & Merge (`reader`, `parser`, `merge`):** Decompresses submissions and comments chunk-by-chunk via `libzstd`, strips punctuation and emojis, and merges the two streams chronologically by `created_utc`.
2. **Two-level Karp–Rabin hashing (`tokenizer`, `rolling_hash`):** Tokenizes text into 5-grams and hashes them modulo the Mersenne prime q = 2⁶¹ - 1. Uses two independent random bases (`p_word` for characters within a token, `p_gram` for sliding across 5 tokens) to avoid structural collisions across word boundaries.
3. **Sliding Window & Misra–Gries (`slidingwindow`, `misragries`):** Maintains a 3-month sliding window divided into 1-week buckets. Old buckets are merged logarithmically following a DGIM-style scheme (at most B + 1 buckets per level). Each bucket holds a Misra–Gries sketch with s counters.
4. **Two-phase burst confirmation & Bottom-k (`bottomk`, `main`):**
   * When a weekly bucket is sealed, heavy hitters in the latest bucket are scored against the historical baseline (`recent_freq / base_freq`, regularized by the m/s Misra–Gries error bound).
   * Phrases exceeding `MIN_SCORE` are monitored during the following week using a **Bottom-k** sketch to estimate distinct authors (F₀).
   * If a flagged phrase is still bursting at the next bucket boundary and passes `MIN_AUTHORS` and `MIN_RATIO` (distinct authors / occurrences, to filter single-account spam), it is written to the output CSV.

## Complexity

With N ≈ 13 weeks in the sliding window, B = 3, s = 70,000, and k = 200:
* **Space:** $O(s \cdot B \log N)$ words for the bucketed Misra–Gries sketches and $O(s \cdot k)$ words for the active Bottom-k sketches and reverse map (pruned at each bucket seal). I/O buffers are O(1).
* **Time:** O(1) amortized per token for rolling hash and Misra–Gries updates; $O(\log k)$ per occurrence of a flagged phrase for Bottom-k; $O(s \cdot B \log N)$ at each weekly bucket seal for merging and burst scoring.

### To-Do

A parameter sweep to find the best correctness-space/time tradeoff is yet to be done: the code will be updated with the best combination with the next version.

## Build & Run

Dependencies: C++17 compiler, CMake $(\ge 3.20)$, `libzstd`, `nlohmann/json`.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

./build/proj <submissions.zst> <comments.zst> [bursts.csv] [stats.csv]
```

## Synthetic Tests

`tests/synthetic_data_generator.py` generates a 40-week Zipf-distributed stream with 9 injected test cases (`evergreen`, `short`, `burst`, `burst_big`, `spam`, and intensity sweeps `sweep20`–`sweep160`) alongside `truth.csv`.

```bash
python3 tests/synthetic_data_generator.py tests
zstd -f tests/synthetic_submissions.jsonl -o tests/synthetic_submissions.zst
zstd -f tests/synthetic_comments.jsonl    -o tests/synthetic_comments.zst

./build/proj tests/synthetic_submissions.zst tests/synthetic_comments.zst tests/bursts.csv tests/stats.csv
python3 tests/evaluate.py tests/truth.csv tests/bursts.csv tests/stats.csv tests/eval_out
```
