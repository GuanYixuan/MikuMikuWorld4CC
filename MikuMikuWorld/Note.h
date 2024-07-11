#pragma once
#include "NoteTypes.h"
#include <string>
#include <vector>

namespace MikuMikuWorld
{
	constexpr int MIN_NOTE_WIDTH = 1;
	constexpr int MAX_NOTE_WIDTH = 12;
	constexpr int MIN_LANE = 0;
	constexpr int MAX_LANE = 11;
	constexpr int NUM_LANES = 12;

	constexpr const char* SE_PERFECT = "perfect";
	constexpr const char* SE_FLICK = "flick";
	constexpr const char* SE_TICK = "tick";
	constexpr const char* SE_FRICTION = "friction";
	constexpr const char* SE_CONNECT = "connect";
	constexpr const char* SE_CRITICAL_TAP = "critical_tap";
	constexpr const char* SE_CRITICAL_FLICK = "critical_flick";
	constexpr const char* SE_CRITICAL_TICK = "critical_tick";
	constexpr const char* SE_CRITICAL_FRICTION = "critical_friction";
	constexpr const char* SE_CRITICAL_CONNECT = "critical_connect";

	constexpr const char* SE_NAMES[] = { SE_PERFECT,         SE_FLICK,         SE_TICK,
		                                 SE_FRICTION,        SE_CONNECT,       SE_CRITICAL_TAP,
		                                 SE_CRITICAL_FLICK,  SE_CRITICAL_TICK, SE_CRITICAL_FRICTION,
		                                 SE_CRITICAL_CONNECT };

	constexpr float flickArrowWidths[] = { 0.95f, 1.25f, 1.8f, 2.3f, 2.6f, 3.2f };

	constexpr float flickArrowHeights[] = { 1, 1.05f, 1.2f, 1.4f, 1.5f, 1.6f };

	enum class ZIndex : int32_t
	{
		HoldLine,
		Guide,
		HoldTick,
		Note,
		FrictionTick,
		zCount
	};

	struct NoteTextures
	{
		int notes;
		int holdPath;
		int touchLine;
		int ccNotes;
		int guideColors;
	};

	extern NoteTextures noteTextures;
	extern int nextID;

	class Note
	{
	  private:
		NoteType type;

	  public:
		// The note's ID whose uniqueness is assured by the global incremental variable `nextID`
		int ID;
		// Start note's ID of the slide that this note belongs to, -1 if this note is slide start or isn't part of a slide
		int parentID;
		// The note's time, where scale is defined by `TICKS_PER_BEAT` in `Constants.h`
		int tick;
		// The position of the note's leftmost point, which should lie in [0, 12) in most cases
		float lane;
		// The note's width, where the full width is 12
		float width;
		// Whether the note is a critical note
		bool critical{ false };
		// Whether the note is a trace note
		bool friction{ false };
		// The flick note's direction, not valid where a flick is inappropriate
		FlickType flick{ FlickType::None };
		// The layer that this note belongs to
		int layer{ 0 };

		explicit Note(NoteType _type);
		explicit Note(NoteType _type, int tick, float lane, float width);
		Note(NoteType _type, int _ID, int _tick, float _lane, float _width, int _layer = 0, bool _critical = false, bool _friction = false, FlickType _flick = FlickType::None, int _parentID = -1) noexcept;
		Note();

		constexpr NoteType getType() const noexcept { return type; }
		constexpr bool operator==(NoteType _type) const noexcept { return type == _type; }
		constexpr bool operator!=(NoteType _type) const noexcept { return type != _type; }

		// Returns whether this note is part of a hold note
		constexpr bool isHold() const { return type == NoteType::Hold || type == NoteType::HoldMid || type == NoteType::HoldEnd; }
		// Returns whether this note is a (single) flick note
		constexpr bool isFlick() const noexcept { return flick != FlickType::None && type != NoteType::Hold && type != NoteType::HoldMid; }
		/**
		 * @brief Returns whether this note controls a segment of slide curve
		 * @note This method doesn't check whether this note links to a `HoldStepType::Skip` though
		 */
		constexpr bool hasEase() const noexcept { return type == NoteType::Hold || type == NoteType::HoldMid; }

		bool canFlick() const;
		bool canTrace() const;
	};

	// Provide additional curve control information for notes in slides
	class HoldStep
	{
	  public:
		// ID of the note that this piece of information belongs to
		int ID;
		HoldStepType type;
		EaseType ease;

		HoldStep() : ID(0), type(HoldStepType::Normal), ease(EaseType::Linear) {}
		HoldStep(int _ID, HoldStepType _type, EaseType _ease) : ID(_ID), type(_type), ease(_ease) {}

		constexpr bool operator==(HoldStepType _type) const noexcept { return type == _type; }
		constexpr bool operator!=(HoldStepType _type) const noexcept { return type != _type; }
	};

	class HoldNote
	{
	  public:
		// Additional curve control information for slide start
		HoldStep start;
		// Additional curve control information for each slide tick
		std::vector<HoldStep> steps;
		// ID of the end note
		int end;

		// Whether the slide start is *hidden*, but if any of `startType` and `endType` is HoldNoteType::Guide, then the slide is considered a guide slide
		HoldNoteType startType{};
		// Whether the slide end is *hidden*, but if any of `startType` and `endType` is HoldNoteType::Guide, then the slide is considered a guide slide
		HoldNoteType endType{};

		FadeType fadeType{ FadeType::Out };
		GuideColor guideColor{ GuideColor::Green };

		HoldNote() = default;
		// Construct a guide slide
		HoldNote(const HoldStep& _start, int _end, FadeType _fadeType, GuideColor _guideColor)
			: start(_start), steps(), end(_end), startType(HoldNoteType::Guide), endType(HoldNoteType::Guide), fadeType(_fadeType), guideColor(_guideColor) {}
		// Full constructor
		HoldNote(const HoldStep& _start, const std::vector<HoldStep>& _steps, int _end, HoldNoteType _startType, HoldNoteType _endType, FadeType _fadeType, GuideColor _guideColor)
			: start(_start), steps(_steps), end(_end), startType(_startType), endType(_endType), fadeType(_fadeType), guideColor(_guideColor) {}

		// Returns whether this is a guide slide
		constexpr bool isGuide() const
		{
			return startType == HoldNoteType::Guide || endType == HoldNoteType::Guide;
		}

		/**
		 * @brief Retrieve HoldStep according to given `index` within `[-1, steps.size()-1]`
		 * @throw `std::out_of_range` if `index` is invalid
		 * @warning Reference returned by this method can be invalidated by vector reallocation
		 */
		HoldStep& operator[](int index) {
			if (index < -1 || index >= (int)steps.size()) throw std::out_of_range("Index out of range in HoldNote[]");
			return index == -1 ? start : steps[index];
		}
		/**
		 * @brief Retrieve HoldStep according to given `index` within `[-1, steps.size()-1]`
		 * @throw `std::out_of_range` if `index` is invalid
		 * @warning Reference returned by this method can be invalidated by vector reallocation
		 */
		const HoldStep& operator[](int index) const {
			if (index < -1 || index >= (int)steps.size()) throw std::out_of_range("Index out of range in HoldNote[]");
			return index == -1 ? start : steps[index];
		}
		/**
		 * @brief Retrieve note ID according to given `index` within `[-1, steps.size()]`
		 * @throw `std::out_of_range` if `index` is invalid
		 */
		int id_at(int index) const {
			if (index < -1 || index > (int)steps.size()) throw std::out_of_range("Index out of range in HoldNote::id_at");
			return (index == steps.size()) ? end : (index == -1 ? start.ID : steps[index].ID);
		}
	};

	struct Score;

	void resetNextID();

	void cycleFlick(Note& note);
	void cycleStepEase(HoldStep& note);
	void cycleStepType(HoldStep& note);
	// Sort HoldSteps contained in current HoldNote according to `score` context
	void sortHoldSteps(const Score& score, HoldNote& note);
	/**
	 * @brief Find the index of a HoldStep according to given `stepID`
	 * @return `int` - The index of first HoldStep with `stepID` in `steps`, returns -1 if given `stepID` cannot be found
	 */
	int findHoldStep(const HoldNote& note, int stepID);

	int getFlickArrowSpriteIndex(const Note& note);
	int getNoteSpriteIndex(const Note& note);
	int getCcNoteSpriteIndex(const Note& note);
	int getFrictionSpriteIndex(const Note& note);
	std::string_view getNoteSE(const Note& note, const Score& score);
}
