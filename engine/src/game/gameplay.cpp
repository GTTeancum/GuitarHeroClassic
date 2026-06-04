// engine/src/game/gameplay.cpp

#include "game/gameplay.h"
#include "render/window_d3d9.h"

#include "ark_v3.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace ghogx::game {

// ---------------------------------------------------------------------------
// load_song
// ---------------------------------------------------------------------------

bool Gameplay::load_song(const std::string& hdr_path, const std::string& ark_path,
                          const std::string& shortname, int difficulty) {
    chart_loaded_ = false;
    song_time_    = 0.0;
    next_note_idx_= 0;
    score_        = 0;
    streak_       = 0;
    multiplier_   = 1;
    hit_flash_mask_  = 0;
    miss_flash_mask_ = 0;
    prev_fret_mask_  = 0;
    difficulty_   = std::clamp(difficulty, 0, 3);
    hdr_path_     = hdr_path;
    ark_path_     = ark_path;

    if (hdr_path.empty() || ark_path.empty()) {
        std::fprintf(stderr, "[gameplay] no ARK paths; cannot load song\n");
        return false;
    }

    // --- MIDI chart ---
    const std::string mid_path = "songs/" + shortname + "/" + shortname + ".mid";
    std::fprintf(stderr, "[gameplay] loading chart: %s\n", mid_path.c_str());

    std::vector<uint8_t> mid_bytes;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(mid_path);
        if (!entry) {
            std::fprintf(stderr, "[gameplay] MIDI not found in ARK: %s\n", mid_path.c_str());
            return false;
        }
        mid_bytes = ark.read_entry(*entry, {ark_path});
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[gameplay] ARK error: %s\n", ex.what());
        return false;
    }

    chart_ = ghogx::chart::parse_midi(mid_bytes);
    chart_loaded_ = true;

    std::fprintf(stderr, "[gameplay] chart loaded: diff=%d notes=%zu dur=%.1fs\n",
                 difficulty_,
                 chart_.notes[difficulty_].size(),
                 chart_.duration_sec());

    // --- Audio ---
    const std::string vgs_path = "songs/" + shortname + "/" + shortname + ".vgs";
    audio_.load_vgs(hdr_path, ark_path, vgs_path);  // non-fatal on failure

    return true;
}

// ---------------------------------------------------------------------------
// tick
// ---------------------------------------------------------------------------

bool Gameplay::is_finished() const {
    if (!chart_loaded_) return false;
    return song_time_ >= chart_.duration_sec() + 2.0;  // 2s grace after last note
}

void Gameplay::tick(float dt, uint32_t fret_mask) {
    if (!chart_loaded_) return;

    // On the first tick, start the audio.
    const bool first_tick = (song_time_ == 0.0 && dt > 0.0f);
    if (first_tick) {
        audio_.play();
        std::fprintf(stderr, "[gameplay] song started\n");
    }

    // Master clock: the audio playback position when playing (so note timing
    // stays locked to the sound), else wall-clock accumulation.
    if (audio_.is_playing())
        song_time_ = audio_.position_sec();
    else
        song_time_ += static_cast<double>(dt);

    // Decay per-lane hit flames (~0.22 s lifetime).
    for (int i = 0; i < 5; ++i)
        lane_flash_[i] = std::max(0.0f, lane_flash_[i] - dt * 4.5f);

    // Clear per-frame feedback.
    hit_flash_mask_  = 0;
    miss_flash_mask_ = 0;
    std::memset(lane_hit_, 0, sizeof(lane_hit_));

    const bool strummed =
        ((fret_mask & (1u << 5)) != 0) &&
        ((prev_fret_mask_ & (1u << 5)) == 0);  // rising edge on strum bit

    const auto& notes = chart_.notes[difficulty_];

    // Advance next_note_idx_ past notes that are permanently missed or hit.
    while (next_note_idx_ < notes.size()) {
        const auto& n = notes[next_note_idx_];
        const double note_sec = chart_.tick_to_sec(n.tick_on);
        if (note_sec < song_time_ - kHitWindowSec) {
            // Note passed without being hit.
            if (!lane_hit_[n.lane]) {
                miss_flash_mask_ |= (1u << n.lane);
                streak_ = 0;
                multiplier_ = 1;
                std::fprintf(stderr, "[gameplay] miss lane=%d streak reset\n", n.lane);
            }
            ++next_note_idx_;
        } else {
            break;
        }
    }

    // Check upcoming notes for hits.
    for (size_t i = next_note_idx_; i < notes.size(); ++i) {
        const auto& n = notes[i];
        const double note_sec = chart_.tick_to_sec(n.tick_on);

        // Past the lookahead window — stop processing.
        if (note_sec > song_time_ + kHitWindowSec) break;

        // Already hit this lane this frame.
        if (lane_hit_[n.lane]) continue;

        // Within hit window: note_sec ∈ [song_time - kHitWindowSec, song_time + kHitWindowSec].
        if (std::abs(note_sec - song_time_) > kHitWindowSec) continue;

        const bool lane_pressed = (fret_mask >> n.lane) & 1;
        const bool is_hopo_candidate = n.is_hopo && (streak_ > 0);

        bool can_hit = false;
        if (is_hopo_candidate && lane_pressed) {
            // HOPO: just pressing (not strumming) counts.
            // Make sure this is a new press (edge).
            const bool was_pressed = (prev_fret_mask_ >> n.lane) & 1;
            can_hit = lane_pressed && !was_pressed;
        }
        if (!can_hit && strummed && lane_pressed) {
            can_hit = true;
        }

        if (can_hit) {
            lane_hit_[n.lane] = true;
            hit_flash_mask_ |= (1u << n.lane);
            lane_flash_[n.lane] = 1.0f;  // light the strikeline flame
            ++streak_;
            // Multiplier: 1→2 at 10, 2→3 at 20, 3→4 at 30, cap at 4.
            if      (streak_ >= 30) multiplier_ = 4;
            else if (streak_ >= 20) multiplier_ = 3;
            else if (streak_ >= 10) multiplier_ = 2;
            else                    multiplier_ = 1;

            const int pts = 50 * multiplier_;
            score_ += pts;

            std::fprintf(stderr,
                "[gameplay] HIT lane=%d tick=%u pts=%d streak=%d mult=%d score=%d\n",
                n.lane, n.tick_on, pts, streak_, multiplier_, score_);
        }
    }

    prev_fret_mask_ = fret_mask;

    // Print score summary once per second.
    static double last_print = 0.0;
    if (song_time_ - last_print >= 1.0) {
        last_print = song_time_;
        std::fprintf(stderr, "[gameplay] t=%.1f score=%d streak=%d mult=%d\n",
                     song_time_, score_, streak_, multiplier_);
    }
}

// ---------------------------------------------------------------------------
// draw
// ---------------------------------------------------------------------------

void Gameplay::draw(ghogx::render::Window& win) {
    if (!chart_loaded_) return;
    if (!highway_) {
        highway_ = std::make_unique<HighwayRenderer>(win);
        // Load the GH2 track texture set natively from the ARK (once).
        highway_->load_textures(hdr_path_, ark_path_);
    }
    // song_time_ is the audio-synced master clock (set in tick()).
    highway_->draw(song_time_, chart_, difficulty_,
                   prev_fret_mask_ & 0x1F /* held frets */, lane_flash_, 1.5f);
}

}  // namespace ghogx::game
