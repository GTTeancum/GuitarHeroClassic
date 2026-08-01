# Performer animation lifecycle to-do

## Stock singer clip exhaustion

Status: open; intentionally deferred from the converted
Clive/Fender/GH1-Arena qualification.

During the complete visible `Shout at the Devil` run on 2026-07-31, the stock
GH2 `metal_singer` visibly froze and later recovered when a new BAND SINGER
event selected another clip.

The trace shows two distinct lifecycle failures:

- `singer_idle` remained at `clip_t=-0.129/7.533` from song time 108.08
  through 114.08 seconds, then resumed.
- `singer_active_fast` advanced past its 9.233-second duration, including
  `clip_t=14.914/9.233` at song time 156.57, and recovered after a later
  performer event.

These are direct `main.drv` `play`/`play_if_safe` requests with
`savedNodeContinuation=0`; they do not use the converted Clive
`play_group` compatibility path.

Evidence:

```text
proofs/full-conversion-stack-visible-final/stderr.log
```

Before changing behavior:

1. recover the retail CharDriver lifecycle rule for direct stock performer
   requests and the relevant play flags;
2. determine whether the negative-time hold is an authored transition wait or
   a compatibility-runtime scheduling error;
3. apply a role-independent rule only where supported by the recovered
   lifecycle contract;
4. preserve one-shot intro, win, lose, and band-jump clips;
5. add focused contracts for singer, bassist, and drummer direct-play paths;
6. run a complete-song matrix with all stock band roles; and
7. require zero active/idle duration overruns or unexplained multi-second
   constant-frame holds.

Do not solve this by applying the converted guitarist group-loop exception to
all performers. Direct stock clips and group-selected converted clips have
different continuation semantics.
