# Cavity

Global delay feedback after the lenses. The Path node pulses at the delay period; its size follows Amount.

- **Amount** — how much loop comes back. Default 0.
- **Time / Note** — seconds, or `1 / Hz` when you want it pitched.
- **Mode** — Time, Sync, Note, and the rest of the spec list.
- **Safe** — on by default. Clamps amount and peak-limits the write so the loop does not eat the session.

Same compressor amount as Comp also sits in this loop, so the cavity breathes with the output.
