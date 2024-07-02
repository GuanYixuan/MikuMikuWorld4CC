#include "Sonolus_json.h"

#include "IO.h"
#include "Math.h"
#include "Constants.h"

#include <iostream>
#include <cmath>
#include <cstdio>
#include "json.hpp"
#include <unordered_map>
#include <stdexcept>
#include <optional>

using namespace MikuMikuWorld;
using namespace Sonolus_json;
using namespace nlohmann;

const std::unordered_map<std::string, Entity_type::__Inner_type> Entity_type::map_from_str {
	// Initialization-related
	{"Initialization", Entity_type::Initialization},
	{"InputManager", Entity_type::InputManager},
	{"Stage", Entity_type::Stage},

	// Time scale and BPM changes
	{"TimeScaleGroup", Entity_type::TimeScaleGroup},
	{"TimeScaleChange", Entity_type::TimeScaleChange},
	{"#TIMESCALE_CHANGE", Entity_type::TimeScaleChange}, // Presents in official charts
	{"#BPM_CHANGE", Entity_type::BPMChange},

	// Single note
	{"NormalTapNote", Entity_type::NormalTap},
	{"NormalFlickNote", Entity_type::NormalFlick},
	{"CriticalTapNote", Entity_type::CriticalTap},
	{"CriticalFlickNote", Entity_type::CriticalFlick},
	{"NormalTraceNote", Entity_type::NormalTrace},
	{"NormalTraceFlickNote", Entity_type::NormalTraceFlick},
	{"CriticalTraceNote", Entity_type::CriticalTrace},
	{"CriticalTraceFlickNote", Entity_type::CriticalTraceFlick},

	// Slide start
	{"NormalSlideStartNote", Entity_type::NormalSlideStart},
	{"CriticalSlideStartNote", Entity_type::CriticalSlideStart},
	{"HiddenSlideStartNote", Entity_type::HiddenSlideStart},
	{"NormalTraceSlideStartNote", Entity_type::NormalTraceSlideStart},
	{"CriticalTraceSlideStartNote", Entity_type::CriticalTraceSlideStart},

	// Slide tick
	{"NormalSlideTickNote", Entity_type::NormalSlideTick},
	{"CriticalSlideTickNote", Entity_type::CriticalSlideTick},
	{"NormalAttachedSlideTickNote", Entity_type::NormalAttachedSlideTick},
	{"CriticalAttachedSlideTickNote", Entity_type::CriticalAttachedSlideTick},
	{"HiddenSlideTickNote", Entity_type::HiddenSlideTick},
	{"IgnoredSlideTickNote", Entity_type::IgnoredSlideTick},

	// Slide end
	{"NormalSlideEndNote", Entity_type::NormalSlideEnd},
	{"NormalSlideEndFlickNote", Entity_type::NormalSlideEndFlick},
	{"CriticalSlideEndNote", Entity_type::CriticalSlideEnd},
	{"CriticalSlideEndFlickNote", Entity_type::CriticalSlideEndFlick},
	{"NormalTraceSlideEndNote", Entity_type::NormalTraceSlideEnd},
	{"CriticalTraceSlideEndNote", Entity_type::CriticalTraceSlideEnd},

	// Slide connector (slide bar)
	{"NormalSlideConnector", Entity_type::NormalSlideConnector},
	{"CriticalSlideConnector", Entity_type::CriticalSlideConnector},

	// Guides
	{"Guide", Entity_type::Guide},

	// Others
	{"SimLine", Entity_type::SimLine},
	{"DamageNote", Entity_type::DamageNote}
};
Entity_type::Entity_type(const std::string& str) {
	if (!map_from_str.count(str)) {
		printf("Unexpected entity type %s", str.c_str());
		throw std::invalid_argument("Unexpected entity type");
	}
	__value = map_from_str.at(str);
}
inline bool Entity_type::is_note() const noexcept { return 0x10 <= (int)__value && (int)__value < 0x70; }
inline Note_category Entity_type::get_category() const noexcept {
	int val = (int)__value;
	if (val < 0x10) return val <= 2 ? Note_category::init : Note_category::timing;
	return (Note_category)((val >> 4) + 1);
}
inline bool Entity_type::critical() const {
	if (!is_note()) throw std::invalid_argument("Not a note");
	if (__value == Entity_type::IgnoredSlideTick || (int)__value >= 0x60) return false;
	return bool((int)__value & 1);
}
inline bool Entity_type::friction() const {
	if (!is_note()) throw std::invalid_argument("Not a note");
	int upper = (int)__value & 0xF0;
	if (upper != 0x10 && upper != 0x20 && upper != 0x40) return false;
	return (int)__value & 2;
}

// Helper function that extracts directions for flicks
inline FlickType get_flick_dir(const std::unordered_map<std::string, json>& data) {
	if (!data.count("direction")) return FlickType::None;

	int dir = data.at("direction");
	if (dir == 1) return FlickType::Right;
	else if (dir == -1) return FlickType::Left;
	return FlickType::Default;
}

Score Sonolus_json::load_file(const std::string& file_name) {
	Score ret;
	ret.tempoChanges.pop_back();

	json js = json::parse(_wfopen(IO::mbToWideStr(file_name).c_str(), L"r"));

	// Extract music offset
	assert(js["bgmOffset"].is_number());
	ret.metadata.musicOffset = -1000 * double(js["bgmOffset"]);

	int current_slide_id = -1;
	bool current_guide_slide = false; // Whether we are processing a guide slide now, set on encountering DummySlides
	std::unordered_map<std::string, int> ref_to_id; // Mapping from "ref" in .json file to ID in editor
	assert(js["entities"].is_array());
	for (const json& entity : js["entities"]) {
		 std::cout << nextID << ": " << entity << std::endl;

		// Extract entity["archetype"]
		assert(entity["archetype"].is_string());
		std::string archetype(entity["archetype"]);
		Entity_type type(archetype);
		Note_category category = type.get_category();

		// Skipping various types
		if (type == Entity_type::Initialization || type == Entity_type::InputManager || type == Entity_type::Stage) continue; // No need to handle
		if (type == Entity_type::IgnoredSlideTick || type == Entity_type::SimLine) continue; // No need to handle
		if (type == Entity_type::TimeScaleGroup || type == Entity_type::DamageNote) continue; // Not supported yet

		// Extract entity["data"] as an unordered_map
		std::unordered_map<std::string, json> data;
		assert(entity["data"].is_array());
		for (const json& data_item : entity["data"]) data[data_item["name"]] = data_item.count("value") ? data_item["value"] : data_item["ref"];

		// Extract some of the widely-used attributes
		std::optional<int> tick = data.count("#BEAT") ? std::optional<int>(std::round(double(data["#BEAT"]) * TICKS_PER_BEAT)) : std::nullopt;
		std::optional<int> width = data.count("size") ? std::optional<int>(std::round(double(data["size"]) * 2)) : std::nullopt;
		std::optional<int> lane = (data.count("lane") && data.count("size")) ? std::optional<int>(std::round(double(data["lane"]) - double(data["size"]) + 6)) : std::nullopt;

		// Convert timings
		bool converted = true;
		if (type == Entity_type::TimeScaleChange) {
			ret.hiSpeedChanges.emplace(
				nextHiSpeedID,
				HiSpeedChange{nextHiSpeedID, tick.value(), data.count("timeScale") ? data["timeScale"] : data["#TIMESCALE"]}
				);
			nextHiSpeedID++;
		} else if (type == Entity_type::BPMChange) ret.tempoChanges.emplace_back(tick.value(), data["#BPM"]);
		else converted = false;
		if (converted) continue;

		// Convert single notes
		if (category == Note_category::single) {
			ret.notes.emplace(nextID, Note(NoteType::Tap, nextID, tick.value(), lane.value(), width.value(), type.critical(), type.friction(), get_flick_dir(data)));
			nextID++;
			continue;
		}

		// Convert slides!
		// Start/Create a slide
		// Assumption: All notes within a slide are presented continuously in the file
		if (category == Note_category::slide_start) {
			current_guide_slide = false;
			// Construct the start note
			ret.notes.emplace(nextID, Note(NoteType::Hold, nextID, tick.value(), lane.value(), width.value(), type.critical(), type.friction()));
			// Create a new HoldNote instance, note that its end note, ease types and slide type (e.g. guide) are not determined
			HoldNote new_hold;
			new_hold.end = -1;
			new_hold.start.ID = nextID;
			if (type == Entity_type::HiddenSlideStart) new_hold.startType = HoldNoteType::Hidden;
			ret.holdNotes.emplace(nextID, new_hold);
			// Remember "ref" to find it later on
			ref_to_id.emplace(std::string(entity["name"]), nextID);
			current_slide_id = nextID++;
		}
		// Add slide ticks
		else if (category == Note_category::slide_tick) {
			// Create slide tick and append it, use EaseType::EaseTypeCount as undetermined (but needed) ease type
			bool attached = (type == Entity_type::NormalAttachedSlideTick) || (type == Entity_type::CriticalAttachedSlideTick);
			HoldStepType step_type = attached ? HoldStepType::Skip : (type == Entity_type::HiddenSlideTick ? HoldStepType::Hidden : HoldStepType::Normal);
			ret.holdNotes[current_slide_id].steps.push_back(HoldStep{nextID, step_type, attached ? EaseType::Linear : EaseType::EaseTypeCount});
			// The lane and width of "AttachedSlideTick" are determined in the second loop
			ret.notes.emplace(nextID, Note(NoteType::HoldMid, nextID, tick.value(), attached ? 0 : lane.value(), attached ? 2 : width.value(), type.critical(), false, FlickType::None, current_slide_id));
			// Remember "ref" to add curve control information for them later on
			if (!attached) ref_to_id.emplace(std::string(entity["name"]), nextID);
			nextID++;
		}
		// Determine slide end
		else if (category == Note_category::slide_end) {
			assert(!current_guide_slide && "Encountered slide end before slide start!");
			ret.notes.emplace(nextID, Note(NoteType::HoldEnd, nextID, tick.value(), lane.value(), width.value(), type.critical(), type.friction(), get_flick_dir(data), current_slide_id));
			ret.holdNotes[current_slide_id].end = nextID++;
			sortHoldSteps(ret, ret.holdNotes[current_slide_id]);
		}
		// Process connectors to provide ease information
		else if (category == Note_category::connector || category == Note_category::guide_slide) {
			// Change hold type to guide on finding special connectors
			if (!current_guide_slide && category == Note_category::guide_slide) {
				current_guide_slide = true;
				ret.holdNotes[current_slide_id].startType = HoldNoteType::Guide;
				ret.holdNotes[current_slide_id].endType = HoldNoteType::Guide;
			} else if (current_guide_slide && category == Note_category::connector) {
				printf("Warning: Probably mixing different types of connectors in the same slide (%d)!", current_slide_id);
			}
			// Extract ease type
			EaseType ease_type = EaseType::Linear;
			if (data["ease"] == 1) ease_type = EaseType::EaseIn;
			else if (data["ease"] == -1) ease_type = EaseType::EaseOut;
			// Find corresponding HoldStep to assign EaseType
			int target_id = ref_to_id[data["head"]];
			int target_index_in_slide = findHoldStep(ret.holdNotes[current_slide_id], target_id);
			ret.holdNotes[current_slide_id][target_index_in_slide].ease = ease_type;
			// Update critical status also
			ret.notes[target_id].critical = type.critical();
		} else {
			std::cout << "Unhandled: " << entity << std::endl;
		}
	}

	// Process HoldSteps with undetermined ease type as they might be the actuall silde end
	for (auto& [id, hold] : ret.holdNotes) {
		for (int i = 0; i < hold.steps.size(); i++) {
			if (hold[i].ease == EaseType::EaseTypeCount) {
				if (i == hold.steps.size()-1 && hold.end == -1) { // Indicates "HiddenSlideTick" as slide end
					hold.end = hold[i].ID;
					if (hold.endType == HoldNoteType::Normal) hold.endType = HoldNoteType::Hidden;
					hold.steps.pop_back();
				} else assert(false); // Otherwise invaild
			}
		}
	}

	// Second pass to determine lane and width for "AttachedSlideTick" according to curve information
	for (auto& hold_pair : ret.holdNotes) {
		const HoldNote& curr_hold = hold_pair.second; // Current hold note
		Note* curr_note = nullptr; // Current note
		const Note* start_note = nullptr; // Start note of the attached connector
		const Note* end_note = nullptr; // End note of the attached connector
		EaseType connector_type = EaseType::Linear;
		for (int i = curr_hold.steps.size(); i >= 0; i--) {
			curr_note = (i == curr_hold.steps.size()) ? &ret.notes[curr_hold.end] : &ret.notes[curr_hold[i].ID];
			if (i == curr_hold.steps.size() || curr_hold.steps[i].type != HoldStepType::Skip) {
				// Update connector endpoints
				end_note = curr_note;
				start_note = curr_note;
				for (int j = i - 1; j >= -1; j--) if (curr_hold[j].type != HoldStepType::Skip) {
					start_note = &ret.notes[curr_hold[j].ID];
					connector_type = curr_hold[j].ease;
					break;
				}
				assert(end_note != start_note && end_note && start_note);
				continue;
			}
			// Calculate x-interval for the slide at the attached moment
			float t = (float)(curr_note->tick - start_note->tick) / (end_note->tick - start_note->tick);
			float pct = connector_type.ease(t);
			float left = lerp(start_note->lane, end_note->lane, pct);
			float right = lerp(start_note->lane+start_note->width, end_note->lane+end_note->width, pct);
			// Move the attached tick to right place
			curr_note->lane = round(left);
			curr_note->width = std::max(lround(right - left), 1l);
		}
	}

	// Sort speed changes and BPM changes to prevent some strange things
	if (ret.tempoChanges.empty()) ret.tempoChanges.push_back(Tempo());
	std::stable_sort(ret.tempoChanges.begin(), ret.tempoChanges.end(), [](const Tempo& a, const Tempo& b) {return a.tick < b.tick; });
	return ret;
}
