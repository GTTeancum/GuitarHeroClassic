// trace_recorder.h - bounded event tracer for the gameplay-loop capture phase.
//
// Streams structured events to a single jsonl file per run (one event per
// line). The intent is to capture, during a window of sustained gameplay,
// enough about (1) what files the engine opens, (2) what properties /
// handlers it looks up, (3) what audio it submits, and (4) which guest
// functions are active when, that we can reconstruct the per-frame
// gameplay loop from the trace offline.
//
// All event-logging functions are no-ops when capture isn't active. The
// gate is a single atomic bool flipped by SetCapturing(). Recording is
// thread-safe; writes are serialized through a small mutex.
//
// See 360_TRACE_PLAN.md Phase 1.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace trace360 {

// Open the jsonl output file. Path is absolute. Idempotent (re-init is a
// no-op). Capture starts disabled; call SetCapturing(true) to begin.
bool Init(const std::string& output_path);

// Flush and close. Safe to call multiple times.
void Shutdown();

// Toggle the capture gate. Logging functions below are no-ops when off.
void SetCapturing(bool on);
bool IsCapturing();

// Force a flush to disk (also happens periodically and on Shutdown).
void Flush();

// --- Structured events -----------------------------------------------------
//
// Each LogXxx writes one jsonl line: `{"t":<ns>,"kind":"...",...fields...}`.
// `t` is monotonic nanoseconds since Init.

void LogFileOpen(std::string_view path, uint64_t offset, uint32_t size, bool found);
void LogPropertyLookup(std::string_view class_name, std::string_view prop_name,
                       uint32_t returned_ptr);
void LogHandlerLookup(std::string_view name, uint32_t returned_ptr);
void LogClassLookup(std::string_view class_name, uint32_t returned_ptr);
void LogAudioSubmit(uint32_t stream_id, uint32_t buffer_len);
void LogFrame(uint64_t frame_index);

// Generic catch-all for ad-hoc events while we figure out what matters.
// kind is short (e.g. "audio.start"); detail can be any JSON-safe string.
void LogEvent(std::string_view kind, std::string_view detail);

// Sample-style stack capture: writes the host call stack at the moment of
// the call as an array of resolved symbol names. Used to find which guest
// functions are active when we periodically poll. Bounded depth.
void LogStackSample(std::string_view tag);

}  // namespace trace360
